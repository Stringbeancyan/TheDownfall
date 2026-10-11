#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "DownfallCaptureObjective.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

USTRUCT(BlueprintType)
struct FDownfallCaptureState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) uint8 OwnerTeam = 255;
    UPROPERTY(BlueprintReadOnly) uint8 CapturingTeam = 255;
    UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bContested = false;
};

/** Server polls overlap membership; clients cannot submit team/progress claims. */
UCLASS()
class DOWNFALL_API ADownfallCaptureObjective : public AActor
{
    GENERATED_BODY()
public:
    ADownfallCaptureObjective();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure) uint8 GetOwnerTeam() const { return State.OwnerTeam; }
    UFUNCTION(BlueprintPure) float GetCaptureProgress() const { return State.Progress; }
    UFUNCTION(BlueprintPure) bool IsContested() const { return State.bContested; }
    UFUNCTION(BlueprintPure) FDownfallCaptureState GetCaptureState() const { return State; }
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> CaptureVolume;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Marker;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, Category="Objective", meta=(ClampMin="1")) float CaptureSeconds = 15.f;
    UPROPERTY(EditAnywhere, Category="Objective", meta=(ClampMin="0")) float DecayPerSecond = .05f;
private:
    UPROPERTY(ReplicatedUsing=OnRep_CaptureState) FDownfallCaptureState State;
    FTimerHandle CaptureTimer;
    double LastCaptureServerTime = 0.0;
    void EvaluateCapture();
    UFUNCTION() void OnRep_CaptureState();
};
