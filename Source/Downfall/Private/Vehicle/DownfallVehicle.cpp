#include "Vehicle/DownfallVehicle.h"
#include "Character/DownfallCharacter.h"
#include "Combat/DownfallCombatComponent.h"
#include "Combat/DownfallHealthComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADownfallVehicle::ADownfallVehicle()
{
    bReplicates = true;
    // Built-in actor movement excludes autonomous proxies in common configurations.
    // This prototype uses explicit authoritative snapshots for every connection.
    SetReplicateMovement(false);
    PrimaryActorTick.bCanEverTick = true;
    NetUpdateFrequency = 30.f;
    MinNetUpdateFrequency = 10.f;
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("ChassisCollision"));
    SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(220.f, 95.f, 55.f));
    Collision->SetCollisionProfileName(TEXT("Pawn"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeChassis"));
    Chassis->SetupAttachment(Collision);
    Chassis->SetStaticMesh(Cube.Object);
    Chassis->SetRelativeScale3D(FVector(4.4f, 1.8f, 0.8f));
    Chassis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeCabin"));
    Cabin->SetupAttachment(Collision);
    Cabin->SetStaticMesh(Cube.Object);
    Cabin->SetRelativeLocation(FVector(-25.f, 0.f, 65.f));
    Cabin->SetRelativeScale3D(FVector(2.2f, 1.6f, 0.6f));
    Cabin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FName WheelName(*FString::Printf(TEXT("PrototypeWheel%d"), Index));
        auto* Wheel = CreateDefaultSubobject<UStaticMeshComponent>(WheelName);
        Wheel->SetupAttachment(Collision);
        Wheel->SetStaticMesh(Cylinder.Object);
        Wheel->SetRelativeLocation(FVector(Index < 2 ? 140.f : -140.f, Index % 2 ? 95.f : -95.f, -25.f));
        Wheel->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
        Wheel->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.3f));
        Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(Collision);
    CameraArm->TargetArmLength = 650.f;
    CameraArm->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
    CameraArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
}

void ADownfallVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallVehicle, Driver);
    DOREPLIFETIME(ADownfallVehicle, SpeedCmPerSecond);
    DOREPLIFETIME(ADownfallVehicle, ServerPose);
}

void ADownfallVehicle::OnRep_ServerPose()
{
    if (!HasAuthority()) SetActorTransform(ServerPose, false, nullptr, ETeleportType::TeleportPhysics);
}

void ADownfallVehicle::UnPossessed()
{
    Super::UnPossessed();
    RemoveInputContext();
    if (!HasAuthority() || !Driver) return;
    // Normal TryExit clears Driver before possession changes. This branch handles
    // disconnects or an external possession change, never a normal dismount.
    ADownfallCharacter* AbandonedDriver = Driver;
    Driver = nullptr;
    AbandonedDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    AbandonedDriver->SetSeated(false);
    if (!AbandonedDriver->GetController()) AbandonedDriver->Destroy();
    ServerThrottle = ServerSteering = SpeedCmPerSecond = 0.f;
    bServerBrake = true;
    ForceNetUpdate();
}

bool ADownfallVehicle::TryEnter(ADownfallCharacter* Character)
{
    if (!HasAuthority() || !IsValid(Character) || Driver || !Character->GetController()
        || !Character->GetHealthComponent()->IsAlive()
        || FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) > FMath::Square(350.f)
        || FMath::Abs(SpeedCmPerSecond) > 100.f)
    {
        return false;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleEntry), false, Character);
    Params.AddIgnoredActor(this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), GetActorLocation(), ECC_Visibility, Params))
    {
        return false;
    }
    AController* EnteringController = Character->GetController();
    Character->GetCombatComponent()->StopFire();
    Driver = Character;
    Character->SetSeated(true);
    // Keep the hidden body with the seat so cleanup and relevancy follow the vehicle.
    Character->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    EnteringController->Possess(this);
    ServerThrottle = 0.f;
    ServerSteering = 0.f;
    bServerBrake = true;
    ForceNetUpdate();
    return true;
}

bool ADownfallVehicle::FindExitLocation(FVector& OutLocation) const
{
    if (!Driver) return false;
    const UCapsuleComponent* Capsule = Driver->GetCapsuleComponent();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float Height = Capsule->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleExit), false, this);
    Params.AddIgnoredActor(Driver);
    const FVector Directions[] = {GetActorRightVector(), -GetActorRightVector(), -GetActorForwardVector()};
    for (const FVector& Direction : Directions)
    {
        const FVector Candidate = GetActorLocation() + Direction * 320.f;
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, Candidate + FVector(0,0,180), Candidate - FVector(0,0,400), ECC_Visibility, Params)
            || Floor.ImpactNormal.Z < 0.7f) continue;
        const FVector Position = Floor.ImpactPoint + FVector(0,0,Height + 5.f);
        FHitResult ExitObstruction;
        const FVector SweepStart(GetActorLocation().X, GetActorLocation().Y, Position.Z);
        const FCollisionShape ExitCapsule = FCollisionShape::MakeCapsule(Radius, Height);
        if (!GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn, ExitCapsule, Params)
            && !GetWorld()->SweepSingleByChannel(ExitObstruction, SweepStart, Position,
                FQuat::Identity, ECC_Pawn, ExitCapsule, Params))
        {
            OutLocation = Position;
            return true;
        }
    }
    return false;
}

bool ADownfallVehicle::TryExit()
{
    if (!HasAuthority() || !Driver || FMath::Abs(SpeedCmPerSecond) > 150.f) return false;
    FVector ExitLocation;
    if (!FindExitLocation(ExitLocation)) return false;
    ADownfallCharacter* ExitingDriver = Driver;
    AController* ExitingController = Controller;
    Driver = nullptr;
    ExitingDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    ExitingDriver->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
    ExitingDriver->SetSeated(false);
    if (ExitingController) ExitingController->Possess(ExitingDriver);
    ServerThrottle = 0.f;
    ServerSteering = 0.f;
    bServerBrake = true;
    ForceNetUpdate();
    return true;
}

void ADownfallVehicle::ServerExit_Implementation()
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastExitRequestTime < 0.25) return;
    LastExitRequestTime = Now;
    TryExit();
}

void ADownfallVehicle::ServerDrive_Implementation(float Throttle, float Steering, bool bBrake)
{
    if (!Driver || !Controller || !FMath::IsFinite(Throttle) || !FMath::IsFinite(Steering)) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastAcceptedInputTime < 0.025) return;
    LastAcceptedInputTime = Now;
    LastServerInputTime = Now;
    ServerThrottle = FMath::Clamp(Throttle, -1.f, 1.f);
    ServerSteering = FMath::Clamp(Steering, -1.f, 1.f);
    bServerBrake = bBrake;
}

void ADownfallVehicle::SimulateAuthority(float DeltaSeconds)
{
    const float Step = FMath::Min(DeltaSeconds, 0.05f);
    if (!Driver || !Controller || GetWorld()->GetTimeSeconds() - LastServerInputTime > 0.35)
    {
        ServerThrottle = 0.f;
        ServerSteering = 0.f;
        bServerBrake = true;
    }
    SpeedCmPerSecond = FMath::Clamp(SpeedCmPerSecond + ServerThrottle * 650.f * Step, -450.f, 1800.f);
    if (bServerBrake || FMath::IsNearlyZero(ServerThrottle))
        SpeedCmPerSecond = FMath::FInterpConstantTo(SpeedCmPerSecond, 0.f, Step, bServerBrake ? 1400.f : 220.f);
    const float YawStep = ServerSteering * (SpeedCmPerSecond / 1800.f) * 65.f * Step;
    // UE sweeps translation, not rotation: verify the rotated box before turning.
    FRotator DesiredRotation = GetActorRotation();
    DesiredRotation.Yaw += YawStep;
    FCollisionQueryParams RotationParams(SCENE_QUERY_STAT(VehicleSteering), false, this);
    if (Driver) RotationParams.AddIgnoredActor(Driver);
    if (!GetWorld()->OverlapBlockingTestByChannel(GetActorLocation(), DesiredRotation.Quaternion(), ECC_Pawn,
        FCollisionShape::MakeBox(Collision->GetScaledBoxExtent() * 0.98f), RotationParams))
        SetActorRotation(DesiredRotation);
    FHitResult MoveHit;
    AddActorWorldOffset(GetActorForwardVector() * SpeedCmPerSecond * Step, true, &MoveHit);
    if (MoveHit.bBlockingHit) SpeedCmPerSecond = 0.f;

    FHitResult Floor;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleGround), false, this);
    if (Driver) Params.AddIgnoredActor(Driver);
    if (GetWorld()->LineTraceSingleByChannel(Floor, GetActorLocation(), GetActorLocation() - FVector(0,0,300), ECC_Visibility, Params))
    {
        const float FloorZ = Floor.ImpactPoint.Z + Collision->GetScaledBoxExtent().Z + 2.f;
        const float Correction = FMath::Clamp(FloorZ - GetActorLocation().Z, -800.f * Step, 400.f * Step);
        AddActorWorldOffset(FVector(0,0,Correction), true);
    }
    else AddActorWorldOffset(FVector(0,0,-800.f * Step), true);
}

void ADownfallVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetNetMode() != NM_DedicatedServer)
    {
        UpdateInputContext();
        if (IsLocallyControlled())
        {
            InputSendElapsed += DeltaSeconds;
            if (InputSendElapsed >= 1.f / 15.f)
            {
                InputSendElapsed = 0.f;
                const auto* LocalController = Cast<APlayerController>(Controller);
                const bool bCanDrive = LocalController && !LocalController->IsMoveInputIgnored();
                ServerDrive(bCanDrive ? LocalThrottle : 0.f, bCanDrive ? LocalSteering : 0.f,
                    !bCanDrive || bLocalBrake);
            }
        }
    }
    if (HasAuthority())
    {
        SimulateAuthority(DeltaSeconds);
        ServerPose = GetActorTransform();
    }
}

void ADownfallVehicle::BuildInputActions()
{
    if (DrivingContext) return;
    DrivingContext = NewObject<UInputMappingContext>(this);
    ThrottleAction = NewObject<UInputAction>(this);
    ThrottleAction->ValueType = EInputActionValueType::Axis1D;
    SteeringAction = NewObject<UInputAction>(this);
    SteeringAction->ValueType = EInputActionValueType::Axis1D;
    ExitAction = NewObject<UInputAction>(this);
    BrakeAction = NewObject<UInputAction>(this);
    DrivingContext->MapKey(ThrottleAction, EKeys::W);
    DrivingContext->MapKey(ThrottleAction, EKeys::S).Modifiers.Add(NewObject<UInputModifierNegate>(DrivingContext));
    DrivingContext->MapKey(SteeringAction, EKeys::D);
    DrivingContext->MapKey(SteeringAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(DrivingContext));
    DrivingContext->MapKey(ExitAction, EKeys::E);
    DrivingContext->MapKey(BrakeAction, EKeys::SpaceBar);
}

void ADownfallVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    BuildInputActions();
    if (auto* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Enhanced->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ThisClass::InputThrottle);
        Enhanced->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ThisClass::InputThrottle);
        Enhanced->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ThisClass::InputSteering);
        Enhanced->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ThisClass::InputSteering);
        Enhanced->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ThisClass::InputBrake);
        Enhanced->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ThisClass::InputBrake);
        Enhanced->BindAction(ExitAction, ETriggerEvent::Started, this, &ThisClass::InputExit);
    }
}

void ADownfallVehicle::UpdateInputContext()
{
    auto* PC = Cast<APlayerController>(Controller);
    if (!IsLocallyControlled() || !PC) { RemoveInputContext(); return; }
    if (bInputContextInstalled && InputController == PC) return;
    RemoveInputContext();
    BuildInputActions();
    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->AddMappingContext(DrivingContext, 10);
            InputController = PC;
            bInputContextInstalled = true;
        }
    }
}

void ADownfallVehicle::RemoveInputContext()
{
    if (InputController.IsValid())
        if (ULocalPlayer* LP = InputController->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                Subsystem->RemoveMappingContext(DrivingContext);
    bInputContextInstalled = false;
    InputController.Reset();
    LocalThrottle = LocalSteering = 0.f;
    bLocalBrake = false;
}

void ADownfallVehicle::InputThrottle(const FInputActionValue& Value) { LocalThrottle = Value.Get<float>(); }
void ADownfallVehicle::InputSteering(const FInputActionValue& Value) { LocalSteering = Value.Get<float>(); }
void ADownfallVehicle::InputBrake(const FInputActionValue& Value) { bLocalBrake = Value.Get<bool>(); }
void ADownfallVehicle::InputExit() { ServerExit(); }

void ADownfallVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveInputContext();
    if (HasAuthority() && Driver && EndPlayReason == EEndPlayReason::Destroyed)
    {
        SpeedCmPerSecond = 0.f;
        if (!TryExit())
        {
            // Restore the body even if all normal exits are obstructed; GameMode handles death.
            ADownfallCharacter* RecoveringDriver = Driver;
            Driver = nullptr;
            RecoveringDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            RecoveringDriver->SetSeated(false);
            if (Controller && !Controller->IsActorBeingDestroyed()) Controller->Possess(RecoveringDriver);
        }
    }
    Super::EndPlay(EndPlayReason);
}
