#include "AI/DownfallRobot.h"
#include "Combat/DownfallHealthComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Presentation/DownfallModelComponent.h"

ADownfallRobot::ADownfallRobot()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetNetUpdateFrequency(15.f);
    SetMinNetUpdateFrequency(5.f);
    SetNetCullDistanceSquared(FMath::Square(15000.f));
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
    bUseControllerRotationYaw = false;
    GetCapsuleComponent()->InitCapsuleSize(42.f, 88.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetHiddenInGame(true);
    GetCharacterMovement()->MaxWalkSpeed = 220.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
    Health = CreateDefaultSubobject<UDownfallHealthComponent>(TEXT("Health"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeTorso"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeLocation(FVector(0,0,0));
    Body->SetRelativeScale3D(FVector(.7,.6,.85));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    Sensor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeSensor"));
    Sensor->SetupAttachment(RootComponent);
    Sensor->SetRelativeLocation(FVector(12,0,63));
    Sensor->SetRelativeScale3D(FVector(.5,.46,.38));
    Sensor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Sphere.Succeeded()) Sensor->SetStaticMesh(Sphere.Object);
    for (int32 Index=0; Index<4; ++Index)
    {
        const FName Name(*FString::Printf(TEXT("PrototypeLimb%d"), Index));
        UStaticMeshComponent* Limb = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Limb->SetupAttachment(RootComponent);
        const bool bLeg = Index < 2;
        const float Side = Index % 2 == 0 ? -1.f : 1.f;
        Limb->SetRelativeLocation(FVector(0, Side * (bLeg ? 19.f : 45.f), bLeg ? -62.f : 0.f));
        Limb->SetRelativeScale3D(bLeg ? FVector(.23,.23,.48) : FVector(.2,.2,.7));
        Limb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (Cube.Succeeded()) Limb->SetStaticMesh(Cube.Object);
    }
    SensorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SensorLight"));
    SensorLight->SetupAttachment(RootComponent);
    SensorLight->SetRelativeLocation(FVector(42,0,63));
    SensorLight->SetLightColor(FLinearColor(1.f,.12f,.025f));
    SensorLight->SetIntensity(30.f);
    SensorLight->SetAttenuationRadius(110.f);
    SensorLight->SetCastShadows(false);
    UDownfallModelComponent* Model = CreateDefaultSubobject<UDownfallModelComponent>(TEXT("SentinelModel"));
    Model->SetupAttachment(RootComponent);
    Model->ModelKind = EDownfallModelKind::Sentinel;
}

void ADownfallRobot::BeginPlay()
{
    Super::BeginPlay();
    Health->OnDeath.AddDynamic(this, &ThisClass::OnDeath);
    if (HasAuthority())
    {
        Health->InitializeHealth(StartingHealth);
        Health->SetTeamId(2);
        PatrolOrigin = GetActorLocation();
        PatrolAxis = GetActorForwardVector();
        MoveGoal = PatrolOrigin + PatrolAxis * PatrolDistance;
        GetWorldTimerManager().SetTimer(ThinkTimer, this, &ThisClass::Think, .2f, true);
    }
    else SetActorTickEnabled(false);
    OnRep_BrainState();
}

void ADownfallRobot::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ThinkTimer);
    Super::EndPlay(Reason);
}

void ADownfallRobot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallRobot, BrainState);
}

bool ADownfallRobot::IsEligibleTarget(const APawn* Pawn) const
{
    if (!IsValid(Pawn) || Pawn == this || Pawn->IsHidden() || !Pawn->IsPlayerControlled()) return false;
    const UDownfallHealthComponent* OtherHealth = Pawn->FindComponentByClass<UDownfallHealthComponent>();
    return OtherHealth && OtherHealth->IsAlive() && OtherHealth->GetTeamId() != Health->GetTeamId();
}

bool ADownfallRobot::CanSee(const APawn* Pawn, FHitResult* OutHit) const
{
    if (!IsEligibleTarget(Pawn)) return false;
    FVector Eyes;
    FRotator ViewRotation;
    Pawn->GetActorEyesViewPoint(Eyes, ViewRotation);
    const FVector Origin = GetActorLocation() + FVector(0,0,63);
    const FVector ToTarget = Eyes - Origin;
    if (ToTarget.SizeSquared() > FMath::Square(SightRange)) return false;
    // A real field of view: no tracking through walls or sensing behind the robot.
    if (FVector::DotProduct(GetActorForwardVector(), ToTarget.GetSafeNormal2D()) < .258819f) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DownfallRobotVision), false, this);
    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Origin, Eyes, ECC_Visibility, Query);
    if (OutHit) *OutHit = Hit;
    return !bHit || Hit.GetActor() == Pawn;
}

void ADownfallRobot::Think()
{
    if (!HasAuthority() || !Health->IsAlive()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    TArray<APawn*, TInlineAllocator<64>> Players;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APlayerController* CandidateController = It->Get())
            if (IsEligibleTarget(CandidateController->GetPawn())) Players.Add(CandidateController->GetPawn());
    }
    APawn* VisibleTarget = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    int32 RemainingTests = 4;
    if (Target.IsValid() && CanSee(Target.Get()))
    {
        VisibleTarget = Target.Get();
        BestDistance = FVector::DistSquared(GetActorLocation(), VisibleTarget->GetActorLocation());
    }
    if (Target.IsValid()) --RemainingTests;
    const int32 Attempts = FMath::Min(RemainingTests, Players.Num());
    for (int32 Index=0; Index<Attempts; ++Index)
    {
        ScanCursor = ScanCursor % Players.Num();
        APawn* Candidate = Players[ScanCursor++];
        if (Candidate == Target.Get()) continue;
        const float Distance = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
        if (Distance < BestDistance && CanSee(Candidate)) { VisibleTarget = Candidate; BestDistance = Distance; }
    }
    if (VisibleTarget)
    {
        Target = VisibleTarget;
        LastSeenLocation = VisibleTarget->GetActorLocation();
        LastSeenTime = Now;
        MoveGoal = LastSeenLocation;
        BrainState = BestDistance <= FMath::Square(AttackRange) ? EDownfallRobotState::Attack : EDownfallRobotState::Pursue;
        if (BrainState == EDownfallRobotState::Attack && Now >= NextShotTime) FireAt(VisibleTarget);
    }
    else if (Now - LastSeenTime < 3.0)
    {
        BrainState = EDownfallRobotState::Search;
        MoveGoal = LastSeenLocation; // Never refresh this from an occluded target's transform.
    }
    else
    {
        Target.Reset();
        BrainState = EDownfallRobotState::Patrol;
        if (FVector::DistSquared2D(GetActorLocation(), MoveGoal) < FMath::Square(100.f)) bPatrolForward = !bPatrolForward;
        MoveGoal = PatrolOrigin + PatrolAxis * (bPatrolForward ? PatrolDistance : -PatrolDistance);
    }
    OnRep_BrainState();
}

void ADownfallRobot::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !Health->IsAlive()) return;
    FVector Direction = MoveGoal - GetActorLocation();
    Direction.Z = 0.f;
    if (!Direction.IsNearlyZero())
    {
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 5.f));
        if (BrainState != EDownfallRobotState::Attack && Direction.SizeSquared() > FMath::Square(80.f))
            AddMovementInput(Direction.GetSafeNormal());
    }
}

void ADownfallRobot::FireAt(APawn* Pawn)
{
    if (!HasAuthority() || !Health->IsAlive() || !IsEligibleTarget(Pawn)) return;
    NextShotTime = GetWorld()->GetTimeSeconds() + FMath::Max(.2f, FireInterval);
    FVector Eyes;
    FRotator Rotation;
    Pawn->GetActorEyesViewPoint(Eyes, Rotation);
    const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 55.f + FVector(0,0,35);
    if (FVector::DistSquared(Muzzle, Eyes) > FMath::Square(AttackRange)) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DownfallRobotShot), false, this);
    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, Eyes, ECC_Visibility, Query);
    if (bHit && Hit.GetActor() == Pawn)
        UGameplayStatics::ApplyPointDamage(Pawn, ShotDamage, (Eyes-Muzzle).GetSafeNormal(), Hit, GetController(), this, UDamageType::StaticClass());
    MulticastShotCosmetics(Muzzle, bHit ? Hit.ImpactPoint : Eyes);
}

float ADownfallRobot::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || !Health->IsAlive() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f) return 0.f;
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& Point = static_cast<const FPointDamageEvent&>(DamageEvent);
        // Prototype sensor weakpoint: upper capsule region. Authored per-part armor is a later system.
        if (GetActorTransform().InverseTransformPosition(Point.HitInfo.ImpactPoint).Z > 45.f) DamageAmount *= 1.5f;
    }
    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ADownfallRobot::OnDeath()
{
    if (HasAuthority())
    {
        BrainState = EDownfallRobotState::Disabled;
        GetWorldTimerManager().ClearTimer(ThinkTimer);
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        ForceNetUpdate();
    }
    OnRep_BrainState();
}

void ADownfallRobot::OnRep_BrainState()
{
    const bool bDisabled = BrainState == EDownfallRobotState::Disabled;
    if (bDisabled)
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        SetActorTickEnabled(false);
    }
    SensorLight->SetVisibility(!bDisabled);
    Body->SetRelativeRotation(bDisabled ? FRotator(20,0,12) : FRotator::ZeroRotator);
    SensorLight->SetLightColor(BrainState == EDownfallRobotState::Attack ? FLinearColor::Red : FLinearColor(1.f,.3f,.03f));
}

void ADownfallRobot::MulticastShotCosmetics_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
    // Explicit prototype feedback. Shipping presentation needs authored Niagara/MetaSound cues.
    if (GetNetMode() != NM_DedicatedServer) DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, .10f, 0, 1.f);
}
