#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DownfallModelComponent.generated.h"

class UStaticMeshComponent;

UENUM()
enum class EDownfallModelKind : uint8 { Responder, Sentinel, UtilityVehicle };

/** Original articulated kit models. Cosmetic only; capsule/chassis own gameplay collision.
 * Replaceable with skeletal production assets without changing replication.
 */
UCLASS(ClassGroup=(Downfall))
class DOWNFALL_API UDownfallModelComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UDownfallModelComponent();
    UPROPERTY(EditAnywhere, Category="Presentation") EDownfallModelKind ModelKind = EDownfallModelKind::Responder;
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> MovingParts;
    TArray<FVector> RestLocations;
    FVector PreviousLocation = FVector::ZeroVector;
    float MotionPhase = 0.f;
    void BuildModel();
    UStaticMeshComponent* Part(const TCHAR* Name, const TCHAR* Shape, FVector Position,
        FVector Dimensions, FLinearColor Color, float Metallic = 0.f,
        FRotator Rotation = FRotator::ZeroRotator);
};
