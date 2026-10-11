#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DownfallDestructibleCover.generated.h"

class UBoxComponent;
class UDownfallHealthComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDownfallCoverState : uint8 { Intact, Damaged, Destroyed };

/** Structural gameplay state is replicated; debris is cosmetic and never blocks players. */
UCLASS()
class DOWNFALL_API ADownfallDestructibleCover : public AActor
{
    GENERATED_BODY()
public:
    ADownfallDestructibleCover();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure) EDownfallCoverState GetStructuralState() const { return StructuralState; }
    UFUNCTION(BlueprintPure) UDownfallHealthComponent* GetHealthComponent() const { return Health; }
protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> StructuralCollision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> IntactMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BrokenBaseMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDownfallHealthComponent> Health;
    UPROPERTY(EditAnywhere, Category="Destruction", meta=(ClampMin="1")) float StructuralHealth = 180.f;
private:
    UPROPERTY(ReplicatedUsing=OnRep_StructuralState) EDownfallCoverState StructuralState = EDownfallCoverState::Intact;
    UFUNCTION() void OnHealthChanged(float CurrentHealth, float MaxHealth);
    UFUNCTION() void OnDestroyedByDamage();
    UFUNCTION() void OnRep_StructuralState();
    UFUNCTION(NetMulticast, Unreliable) void MulticastBreakCosmetics();
    void ApplyStructuralState();
};
