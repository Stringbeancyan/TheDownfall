#include "Presentation/DownfallRainComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "World/DownfallWorldDirector.h"

UDownfallRainComponent::UDownfallRainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.f / 30.f;
}

void UDownfallRainComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer)
    {
        SetComponentTickEnabled(false);
        return;
    }
    Streaks = NewObject<UInstancedStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
    Streaks->SetupAttachment(GetOwner()->GetRootComponent());
    Streaks->SetMobility(EComponentMobility::Movable);
    Streaks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    if (UMaterialInterface* RainMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Downfall/Generated/Materials/M_RainStreak.M_RainStreak")))
        Streaks->SetMaterial(0, RainMaterial);
    Streaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Streaks->SetCastShadow(false);
    Streaks->SetCanEverAffectNavigation(false);
    Streaks->RegisterComponent();
    for (int32 Index = 0; Index < 256; ++Index)
    {
        Offsets.Add(FVector(Random.FRandRange(-1000,1000), Random.FRandRange(-1000,1000), Random.FRandRange(-300,700)));
        Exposed.Add(false);
        Streaks->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
    }
}

void UDownfallRainComponent::TickComponent(float DeltaSeconds, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    const ADownfallWorldDirector* Director = Cast<ADownfallWorldDirector>(GetOwner());
    APlayerController* LocalPlayer = GetWorld()->GetFirstPlayerController();
    if (!Streaks || !Director || !LocalPlayer || !LocalPlayer->IsLocalController() || !LocalPlayer->PlayerCameraManager) return;
    const FDownfallWeatherSample Weather = Director->GetWeatherSample();
    Streaks->SetVisibility(Weather.Rain > .01f);
    if (Weather.Rain <= .01f) return;
    const FVector View = LocalPlayer->PlayerCameraManager->GetCameraLocation();
    const float Step = FMath::Min(DeltaSeconds, .1f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DownfallRainRoof), false, GetOwner());
    if (LocalPlayer->GetPawn()) Query.AddIgnoredActor(LocalPlayer->GetPawn());
    // Eight roof tests per presentation frame, including outside viewpoints.
    // Avoid one trace per particle per frame. Indoor/roof-edge response is coarse.
    for (int32 Test = 0; Test < 8; ++Test)
    {
        const int32 Index = RoofCursor++ % Offsets.Num();
        const FVector From = View + Offsets[Index];
        FHitResult Roof;
        Exposed[Index] = !GetWorld()->LineTraceSingleByChannel(Roof, From,
            From + FVector(0,0,20000), ECC_Visibility, Query);
    }
    RoofCursor %= Offsets.Num();
    const int32 VisibleCount = FMath::RoundToInt(Weather.Rain * Offsets.Num());
    for (int32 Index = 0; Index < Offsets.Num(); ++Index)
    {
        FVector& Offset = Offsets[Index];
        Offset.Z -= 1600.f * Step;
        Offset.X += Weather.WindSpeed * 35.f * Step;
        if (Offset.Z < -300) { Offset.Z += 1000; Exposed[Index] = false; }
        if (Offset.X > 1000) { Offset.X -= 2000; Exposed[Index] = false; }
        const bool bVisible = Index < VisibleCount && Exposed[Index];
        const FVector Scale = bVisible ? FVector(.004f,.004f,.32f) : FVector::ZeroVector;
        Streaks->UpdateInstanceTransform(Index,
            FTransform(FRotator(Weather.WindSpeed * -1.2f,0,0), View + Offset, Scale), true, false, true);
    }
    Streaks->MarkRenderStateDirty();
}

void UDownfallRainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Streaks) Streaks->DestroyComponent();
    Super::EndPlay(Reason);
}
