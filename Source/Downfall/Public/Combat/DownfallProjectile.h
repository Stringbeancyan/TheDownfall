#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "Combat/DownfallWeaponDefinition.h"
#include "DownfallProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

USTRUCT()
struct FDownfallProjectileImpact
{
    GENERATED_BODY()
    UPROPERTY() bool bOccurred = false;
    UPROPERTY() FVector_NetQuantize10 Location = FVector::ZeroVector;
    UPROPERTY() FVector_NetQuantizeNormal Normal = FVector::UpVector;
};

/** Server-only swept ballistic simulation; replicated transform/impact drives client presentation. */
UCLASS()
class DOWNFALL_API ADownfallProjectile : public AActor
{
    GENERATED_BODY()
public:
    ADownfallProjectile();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Call between SpawnActorDeferred and FinishSpawningActor, on authority only. */
    void InitializeShot(const FDownfallWeaponSpec& Spec, const FVector& Direction);

protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;
    UPROPERTY(ReplicatedUsing=OnRep_Impact) FDownfallProjectileImpact Impact;
    UFUNCTION(BlueprintImplementableEvent, Category="Combat|Presentation")
    void OnImpactPresentation(FVector Position, FVector Normal);

private:
    UFUNCTION() void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION() void OnRep_Impact();
    float Damage = 0.f;
    float FlightLifetime = 3.f;
    bool bConfigured = false;
    bool bPresentedImpact = false;
};
