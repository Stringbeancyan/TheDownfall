#include "Character/DownfallCharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/DownfallCharacterMovementComponent.h"
#include "Combat/DownfallCombatComponent.h"
#include "Combat/DownfallHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DownfallGameMode.h"
#include "Core/DownfallPlayerState.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Net/UnrealNetwork.h"
#include "Presentation/DownfallCinematicDirector.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicle/DownfallVehicle.h"

ADownfallCharacter::ADownfallCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDownfallCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(30.0f);
	SetMinNetUpdateFrequency(10.0f);
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 92.0f);
	// Placeholder body meshes have no collision; AI and aiming trace the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BaseEyeHeight = 68.0f;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	HealthComponent = CreateDefaultSubobject<UDownfallHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UDownfallCombatComponent>(TEXT("CombatComponent"));
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 60.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->ProbeSize = 12.0f;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 85.0f;

	PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	PrototypeBody->SetupAttachment(GetRootComponent());
	PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, -14.0f));
	PrototypeBody->SetRelativeScale3D(FVector(0.48f, 0.4f, 1.25f));
	PrototypeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeHead"));
	PrototypeHead->SetupAttachment(GetRootComponent());
	PrototypeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 66.0f));
	PrototypeHead->SetRelativeScale3D(FVector(0.34f));
	PrototypeWeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeWeaponBody"));
	PrototypeWeaponBody->SetupAttachment(GetRootComponent());
	PrototypeWeaponBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeWeaponBody->SetRelativeLocation(FVector(15.0f, 15.0f, 36.0f));
	PrototypeWeaponBody->SetRelativeScale3D(FVector(0.35f, 0.09f, 0.14f));
	PrototypeWeaponBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeWeaponBarrel"));
	PrototypeWeaponBarrel->SetupAttachment(GetRootComponent());
	PrototypeWeaponBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeWeaponBarrel->SetRelativeLocation(FVector(40.0f, 15.0f, 45.0f));
	PrototypeWeaponBarrel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PrototypeWeaponBarrel->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WeaponMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BodyMesh.Succeeded())
	{
		PrototypeBody->SetStaticMesh(BodyMesh.Object);
		PrototypeWeaponBarrel->SetStaticMesh(BodyMesh.Object);
	}
	if (HeadMesh.Succeeded()) PrototypeHead->SetStaticMesh(HeadMesh.Object);
	if (WeaponMesh.Succeeded()) PrototypeWeaponBody->SetStaticMesh(WeaponMesh.Object);
}

void ADownfallCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &ADownfallCharacter::HandleDeath);
	if (HasAuthority())
	{
		const ADownfallPlayerState* State = GetPlayerState<ADownfallPlayerState>();
		HealthComponent->SetTeamId(State ? State->GetTeamId() : 1);
	}
	// Initial replicated health can arrive before this actor binds its death event.
	if (!HealthComponent->IsAlive()) HandleDeath();
}

void ADownfallCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveNativeInputMapping();
	HealthComponent->OnDeath.RemoveDynamic(this, &ADownfallCharacter::HandleDeath);
	Super::EndPlay(EndPlayReason);
}

void ADownfallCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADownfallCharacter, bSeated);
}

void ADownfallCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (const ADownfallPlayerState* State = GetPlayerState<ADownfallPlayerState>())
	{
		HealthComponent->SetTeamId(State->GetTeamId());
	}
}

void ADownfallCharacter::UnPossessed()
{
	StopFire();
	StopSprint();
	RemoveNativeInputMapping();
	Super::UnPossessed();
}

void ADownfallCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	if (!IsLocallyControlled())
	{
		CombatComponent->StopFire();
		StopSprint();
		RemoveNativeInputMapping();
	}
}

FVector ADownfallCharacter::GetPawnViewLocation() const
{
	// Server combat uses the same head origin, independent of client camera obstruction or lag.
	return GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight);
}

bool ADownfallCharacter::CanAct() const
{
	return HealthComponent->IsAlive() && !bSeated &&
		(!Controller || !Controller->IsMoveInputIgnored());
}

void ADownfallCharacter::BuildNativeInputMapping()
{
	if (NativeMappingContext) return;
	NativeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_DownfallNative"));
	auto MakeAction = [this](const FName Name, const EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	ReloadAction = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	NextWeaponAction = MakeAction(TEXT("IA_NextWeapon"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	ShoulderAction = MakeAction(TEXT("IA_Shoulder"), EInputActionValueType::Boolean);
	CinematicAction = MakeAction(TEXT("IA_Cinematic"), EInputActionValueType::Boolean);
	SkipCinematicAction = MakeAction(TEXT("IA_SkipCinematic"), EInputActionValueType::Boolean);

	auto MapAxis = [this](UInputAction* Action, FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = NativeMappingContext->MapKey(Action, Key);
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(NativeMappingContext));
		}
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(NativeMappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
	};
	MapAxis(MoveAction, EKeys::W, true, false);
	MapAxis(MoveAction, EKeys::S, true, true);
	MapAxis(MoveAction, EKeys::A, false, true);
	MapAxis(MoveAction, EKeys::D, false, false);
	MapAxis(LookAction, EKeys::MouseX, false, false);
	MapAxis(LookAction, EKeys::MouseY, true, true);
	NativeMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	NativeMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	NativeMappingContext->MapKey(CrouchAction, EKeys::LeftControl);
	NativeMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	NativeMappingContext->MapKey(ReloadAction, EKeys::R);
	NativeMappingContext->MapKey(NextWeaponAction, EKeys::Q);
	NativeMappingContext->MapKey(NextWeaponAction, EKeys::MouseScrollUp);
	NativeMappingContext->MapKey(InteractAction, EKeys::E);
	NativeMappingContext->MapKey(ShoulderAction, EKeys::V);
	NativeMappingContext->MapKey(CinematicAction, EKeys::F8);
	NativeMappingContext->MapKey(SkipCinematicAction, EKeys::F9);
}

void ADownfallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	APlayerController* Player = Cast<APlayerController>(Controller);
	if (!ensure(EnhancedInput) || !Player || !Player->GetLocalPlayer()) return;
	BuildNativeInputMapping();
	BoundInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
	if (BoundInputSubsystem) BoundInputSubsystem->AddMappingContext(NativeMappingContext, 0);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADownfallCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADownfallCharacter::Look);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ADownfallCharacter::StartJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADownfallCharacter::StopJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ADownfallCharacter::StopJump);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ADownfallCharacter::StartSprint);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &ADownfallCharacter::StopSprint);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ADownfallCharacter::StopSprint);
	EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ADownfallCharacter::ToggleCrouch);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ADownfallCharacter::StartFire);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &ADownfallCharacter::StopFire);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Canceled, this, &ADownfallCharacter::StopFire);
	EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &ADownfallCharacter::Reload);
	EnhancedInput->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &ADownfallCharacter::NextWeapon);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ADownfallCharacter::Interact);
	EnhancedInput->BindAction(ShoulderAction, ETriggerEvent::Started, this, &ADownfallCharacter::SwitchShoulder);
	EnhancedInput->BindAction(CinematicAction, ETriggerEvent::Started, this, &ADownfallCharacter::StartCinematic);
	EnhancedInput->BindAction(SkipCinematicAction, ETriggerEvent::Started, this, &ADownfallCharacter::SkipCinematic);
}

void ADownfallCharacter::RemoveNativeInputMapping()
{
	if (BoundInputSubsystem && NativeMappingContext)
	{
		BoundInputSubsystem->RemoveMappingContext(NativeMappingContext);
	}
	BoundInputSubsystem = nullptr;
}

void ADownfallCharacter::Move(const FInputActionValue& Value)
{
	if (!CanAct() || !Controller) return;
	FVector2D Input = Value.Get<FVector2D>();
	if (Input.SizeSquared() > 1.0f) Input.Normalize();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Input.X);
}

void ADownfallCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void ADownfallCharacter::StartJump() { if (CanAct()) Jump(); }
void ADownfallCharacter::StopJump() { StopJumping(); }
void ADownfallCharacter::StartSprint()
{
	if (CanAct()) CastChecked<UDownfallCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSprint(true);
}
void ADownfallCharacter::StopSprint()
{
	CastChecked<UDownfallCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSprint(false);
}
void ADownfallCharacter::ToggleCrouch()
{
	if (!CanAct()) return;
	if (bIsCrouched) UnCrouch(); else Crouch();
}
void ADownfallCharacter::StartFire() { if (CanAct()) CombatComponent->StartFire(); }
void ADownfallCharacter::StopFire() { CombatComponent->StopFire(); }
void ADownfallCharacter::Reload() { if (CanAct()) CombatComponent->Reload(); }
void ADownfallCharacter::NextWeapon() { if (CanAct()) CombatComponent->EquipNextWeapon(); }
void ADownfallCharacter::Interact() { if (CanAct()) ServerInteract(); }
void ADownfallCharacter::SwitchShoulder() { CameraBoom->SocketOffset.Y *= -1.0f; }

void ADownfallCharacter::ServerInteract_Implementation()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanAct() || !Controller || Now - LastServerInteractionTime < 0.25) return;
	LastServerInteractionTime = Now;
	const FVector ViewOrigin = GetPawnViewLocation();
	const FVector ViewDirection = GetBaseAimRotation().Vector();
	ADownfallVehicle* BestVehicle = nullptr;
	float BestDistanceSquared = FMath::Square(350.0f);
	for (TActorIterator<ADownfallVehicle> It(GetWorld()); It; ++It)
	{
		ADownfallVehicle* Vehicle = *It;
		const FVector ToVehicle = Vehicle->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = ToVehicle.SizeSquared();
		if (DistanceSquared > BestDistanceSquared ||
			FVector::DotProduct(ViewDirection.GetSafeNormal2D(), ToVehicle.GetSafeNormal2D()) < 0.2f)
		{
			continue;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DownfallInteraction), false, this);
		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewOrigin,
			Vehicle->GetActorLocation(), ECC_Visibility, Params);
		if (!bHit || Hit.GetActor() == Vehicle)
		{
			BestVehicle = Vehicle;
			BestDistanceSquared = DistanceSquared;
		}
	}
	if (BestVehicle) BestVehicle->TryEnter(this);
}

void ADownfallCharacter::SetSeated(const bool bNewSeated)
{
	if (!HasAuthority()) return;
	bSeated = bNewSeated;
	StopFire();
	StopSprint();
	OnRep_Seated();
	ForceNetUpdate();
}

void ADownfallCharacter::OnRep_Seated()
{
	SetActorHiddenInGame(bSeated);
	GetCapsuleComponent()->SetCollisionEnabled(bSeated || !HealthComponent->IsAlive()
		? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	if (bSeated || !HealthComponent->IsAlive())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void ADownfallCharacter::HandleDeath()
{
	if (bDeathHandled) return;
	bDeathHandled = true;
	StopFire();
	StopSprint();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Placeholder death pose. No claim of ragdoll, mocap, or skeletal animation.
	PrototypeBody->SetRelativeRotation(FRotator(0.0f, 0.0f, 75.0f));
	PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, -62.0f));
	PrototypeHead->SetRelativeLocation(FVector(0.0f, 55.0f, -62.0f));
	PrototypeWeaponBody->SetVisibility(false);
	PrototypeWeaponBarrel->SetVisibility(false);
	if (HasAuthority())
	{
		if (ADownfallGameMode* Mode = GetWorld()->GetAuthGameMode<ADownfallGameMode>())
		{
			Mode->HandleCharacterDeath(this);
		}
	}
}

void ADownfallCharacter::StartCinematic()
{
	APlayerController* Player = Cast<APlayerController>(Controller);
	if (!Player || !Player->IsLocalController()) return;
	for (TActorIterator<ADownfallCinematicDirector> It(GetWorld()); It; ++It)
	{
		It->StartForPlayer(Player);
		break;
	}
}

void ADownfallCharacter::SkipCinematic()
{
	APlayerController* Player = Cast<APlayerController>(Controller);
	if (!Player || !Player->IsLocalController()) return;
	for (TActorIterator<ADownfallCinematicDirector> It(GetWorld()); It; ++It)
	{
		It->SkipForPlayer(Player);
		break;
	}
}
