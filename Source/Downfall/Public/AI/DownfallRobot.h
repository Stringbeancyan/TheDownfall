#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/NetSerialization.h"
#include "TimerManager.h"
#include "DownfallRobot.generated.h"

class UDownfallHealthComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDownfallRobotState : uint8 { Patrol, Pursue, Search, Attack, Disabled };

/** Server-only prototype brain. Straight-line CharacterMovement patrol/chase; no navmesh tactics.
 * LOS is capped at four player tests per decision, plus one authoritative shot confirmation.
 */
UCLASS()
class DOWNFALL_API ADownfallRobot : public ACharacter
{
    GENERATED_BODY()
public:
    ADownfallRobot();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    UFUNCTION(BlueprintPure) UDownfallHealthComponent* GetHealthComponent() const { return Health; }
    UFUNCTION(BlueprintPure) EDownfallRobotState GetRobotState() const { return BrainState; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDownfallHealthComponent> Health;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sensor;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> SensorLight;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin="100")) float SightRange = 3200.f;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin="100")) float AttackRange = 1700.f;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin=".2")) float FireInterval = 1.2f;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin="0")) float ShotDamage = 12.f;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin="0")) float PatrolDistance = 500.f;
    UPROPERTY(EditAnywhere, Category="AI", meta=(ClampMin="1")) float StartingHealth = 180.f;

private:
    UPROPERTY(ReplicatedUsing=OnRep_BrainState) EDownfallRobotState BrainState = EDownfallRobotState::Patrol;
    TWeakObjectPtr<APawn> Target;
    FVector PatrolOrigin = FVector::ZeroVector;
    FVector LastSeenLocation = FVector::ZeroVector;
    FVector MoveGoal = FVector::ZeroVector;
    FVector PatrolAxis = FVector::ForwardVector;
    double LastSeenTime = -100.0;
    double NextShotTime = 0.0;
    int32 ScanCursor = 0;
    bool bPatrolForward = true;
    FTimerHandle ThinkTimer;
    void Think();
    bool IsEligibleTarget(const APawn* Pawn) const;
    bool CanSee(const APawn* Pawn, FHitResult* OutHit = nullptr) const;
    void FireAt(APawn* Pawn);
    UFUNCTION() void OnDeath();
    UFUNCTION() void OnRep_BrainState();
    UFUNCTION(NetMulticast, Unreliable) void MulticastShotCosmetics(FVector_NetQuantize Start, FVector_NetQuantize End);
};
