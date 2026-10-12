#include "World/DownfallDestructibleCover.h"
#include "Combat/DownfallHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADownfallDestructibleCover::ADownfallDestructibleCover()
{
    bReplicates = true;
    SetNetUpdateFrequency(5.f);
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    StructuralCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("StructuralCollision"));
    StructuralCollision->SetupAttachment(RootComponent);
    StructuralCollision->SetRelativeLocation(FVector(0,0,90));
    StructuralCollision->SetBoxExtent(FVector(160,35,90));
    StructuralCollision->SetCollisionProfileName(TEXT("BlockAll"));
    IntactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactMesh"));
    IntactMesh->SetupAttachment(RootComponent);
    IntactMesh->SetRelativeLocation(FVector(0,0,90));
    IntactMesh->SetRelativeScale3D(FVector(3.2,.7,1.8));
    IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BrokenBaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrokenBase"));
    BrokenBaseMesh->SetupAttachment(RootComponent);
    BrokenBaseMesh->SetRelativeLocation(FVector(0,0,12));
    BrokenBaseMesh->SetRelativeScale3D(FVector(3.2,.85,.24));
    BrokenBaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BrokenBaseMesh->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) { IntactMesh->SetStaticMesh(Cube.Object); BrokenBaseMesh->SetStaticMesh(Cube.Object); }
    Health = CreateDefaultSubobject<UDownfallHealthComponent>(TEXT("Health"));
}

void ADownfallDestructibleCover::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        Health->InitializeHealth(StructuralHealth);
        Health->SetTeamId(255);
        Health->OnDeath.AddDynamic(this, &ThisClass::OnDestroyedByDamage);
        Health->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
    }
    ApplyStructuralState();
}

void ADownfallDestructibleCover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallDestructibleCover, StructuralState);
}

void ADownfallDestructibleCover::OnHealthChanged(float CurrentHealth, float MaxHealth)
{
    if (!HasAuthority() || CurrentHealth <= 0.f || StructuralState == EDownfallCoverState::Destroyed) return;
    if (CurrentHealth <= MaxHealth * .5f)
    {
        StructuralState = EDownfallCoverState::Damaged;
        ApplyStructuralState();
        ForceNetUpdate();
    }
}

void ADownfallDestructibleCover::OnDestroyedByDamage()
{
    if (!HasAuthority() || StructuralState == EDownfallCoverState::Destroyed) return;
    StructuralState = EDownfallCoverState::Destroyed;
    ApplyStructuralState();
    ForceNetUpdate();
    MulticastBreakCosmetics();
}

void ADownfallDestructibleCover::OnRep_StructuralState() { ApplyStructuralState(); }

void ADownfallDestructibleCover::ApplyStructuralState()
{
    const bool bBroken = StructuralState == EDownfallCoverState::Destroyed;
    StructuralCollision->SetCollisionEnabled(bBroken ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    IntactMesh->SetVisibility(!bBroken);
    BrokenBaseMesh->SetVisibility(bBroken);
    // Damaged silhouette is cosmetic; authoritative collision remains intact until destruction.
    IntactMesh->SetRelativeScale3D(StructuralState == EDownfallCoverState::Damaged ? FVector(3.2,.68,1.7) : FVector(3.2,.7,1.8));
}

void ADownfallDestructibleCover::MulticastBreakCosmetics_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer) return;
    UStaticMesh* Mesh = IntactMesh->GetStaticMesh();
    if (!Mesh) return;
    // Bounded, non-blocking primitive fragments. Replace with Niagara or authored fracture visuals.
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const FVector LocalOffset(-125.f + Index * 50.f, 0, 90.f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AStaticMeshActor* Debris = GetWorld()->SpawnActor<AStaticMeshActor>(
            GetActorTransform().TransformPosition(LocalOffset), GetActorRotation(), Params);
        if (!Debris) continue;
        Debris->SetReplicates(false);
        UStaticMeshComponent* Piece = Debris->GetStaticMeshComponent();
        Piece->SetMobility(EComponentMobility::Movable);
        Piece->SetStaticMesh(Mesh);
        Piece->SetWorldScale3D(FVector(.35,.45,.4));
        Piece->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        Piece->SetCollisionResponseToAllChannels(ECR_Ignore);
        Piece->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        Piece->SetSimulatePhysics(true);
        Piece->SetMassOverrideInKg(NAME_None, 5.f);
        Piece->AddImpulse(GetActorRotation().RotateVector(FVector((Index-2.5f)*60.f, Index%2 ? 250.f : -250.f, 250.f)), NAME_None, true);
        Debris->SetLifeSpan(4.f);
    }
}
