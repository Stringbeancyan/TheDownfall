#include "Combat/DownfallProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADownfallProjectile::ADownfallProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);
    NetUpdateFrequency = 30.f;
    MinNetUpdateFrequency = 15.f;
    NetCullDistanceSquared = FMath::Square(20000.f);

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(1.5f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_GameTraceChannel1); // DownfallProjectile in DefaultEngine.ini.
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);
    Collision->CanCharacterStepUpOn = ECB_No;
    Collision->SetCanEverAffectNavigation(false);
    Collision->OnComponentHit.AddDynamic(this, &ThisClass::HandleHit);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderTracer"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCastShadow(false);
    Visual->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded()) Visual->SetStaticMesh(SphereMesh.Object);
    Visual->SetRelativeScale3D(FVector(0.035f));

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("BallisticMovement"));
    Movement->SetUpdatedComponent(Collision);
    Movement->bAutoActivate = false;
    Movement->bRotationFollowsVelocity = true;
    Movement->bInitialVelocityInLocalSpace = false;
    Movement->bForceSubStepping = true;
    Movement->bSweepCollision = true;
    Movement->MaxSimulationTimeStep = 1.f / 120.f;
    Movement->MaxSimulationIterations = 16;
    Movement->bShouldBounce = false;
}

void ADownfallProjectile::InitializeShot(const FDownfallWeaponSpec& Spec, const FVector& Direction)
{
    if (!HasAuthority() || !Spec.IsValid() || Direction.ContainsNaN() || Direction.IsNearlyZero()) return;
    Damage = Spec.Damage;
    FlightLifetime = Spec.ProjectileLifetime;
    Movement->InitialSpeed = Spec.MuzzleSpeed;
    Movement->MaxSpeed = Spec.MuzzleSpeed * 1.1f;
    Movement->ProjectileGravityScale = Spec.GravityScale;
    Movement->Velocity = Direction.GetSafeNormal() * Spec.MuzzleSpeed;
    bConfigured = true;
}

void ADownfallProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (!bConfigured) { Destroy(); return; }
        if (GetOwner()) Collision->IgnoreActorWhenMoving(GetOwner(), true);
        if (GetInstigator()) Collision->IgnoreActorWhenMoving(GetInstigator(), true);
        Movement->Activate(true);
        SetLifeSpan(FlightLifetime);
    }
    else
    {
        // Client transforms arrive through actor replication. Clients never run damaging sweeps.
        Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Movement->Deactivate();
        Movement->SetComponentTickEnabled(false);
        if (Impact.bOccurred) OnRep_Impact();
    }
}

void ADownfallProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallProjectile, Impact);
}

void ADownfallProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!HasAuthority() || Impact.bOccurred || OtherActor == GetOwner() || OtherActor == GetInstigator()) return;
    const FVector ShotDirection = Movement->Velocity.GetSafeNormal();
    Impact.bOccurred = true;
    Impact.Location = Hit.ImpactPoint;
    Impact.Normal = Hit.ImpactNormal;
    Movement->StopMovementImmediately();
    Movement->Deactivate();
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (IsValid(OtherActor))
    {
        UGameplayStatics::ApplyPointDamage(OtherActor, Damage, ShotDirection, Hit,
            GetInstigatorController(), this, UDamageType::StaticClass());
    }
    OnRep_Impact();
    ForceNetUpdate();
    // Keep authoritative impact state alive briefly so a newly relevant client can receive it.
    SetLifeSpan(0.5f);
}

void ADownfallProjectile::OnRep_Impact()
{
    if (!Impact.bOccurred || bPresentedImpact) return;
    bPresentedImpact = true;
    Visual->SetVisibility(false);
    if (GetNetMode() != NM_DedicatedServer) OnImpactPresentation(Impact.Location, Impact.Normal);
}
