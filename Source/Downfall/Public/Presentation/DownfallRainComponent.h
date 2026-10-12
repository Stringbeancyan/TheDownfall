#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DownfallRainComponent.generated.h"

class UInstancedStaticMeshComponent;

/** Bounded camera-local rain rehearsal. No replicated particles or server work. */
UCLASS()
class DOWNFALL_API UDownfallRainComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDownfallRainComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Streaks;
    TArray<FVector> Offsets;
    TArray<bool> Exposed;
    FRandomStream Random{7241};
    int32 RoofCursor = 0;
};
