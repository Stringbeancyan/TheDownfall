#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DownfallHealthComponent.generated.h"

class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDownfallDeathEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDownfallHealthChangedEvent, float, Health, float, MaximumHealth);

/** Server-owned health. Binding to OnTakeAnyDamage also supports native point/radial damage. */
UCLASS(ClassGroup=(Downfall), meta=(BlueprintSpawnableComponent))
class DOWNFALL_API UDownfallHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDownfallHealthComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="Health") float GetHealth() const { return CurrentHealth; }
    UFUNCTION(BlueprintPure, Category="Health") float GetMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintPure, Category="Health") bool IsAlive() const { return CurrentHealth > 0.f; }
    UFUNCTION(BlueprintPure, Category="Health") uint8 GetTeamId() const { return TeamId; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Health") void SetTeamId(uint8 InTeamId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Health") void InitializeHealth(float InMaxHealth);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Health")
    float ApplyDamage(float Damage, AController* EventInstigator, AActor* DamageCauser);

    UPROPERTY(BlueprintAssignable, Category="Health") FDownfallDeathEvent OnDeath;
    UPROPERTY(BlueprintAssignable, Category="Health") FDownfallHealthChangedEvent OnHealthChanged;

protected:
    virtual void BeginPlay() override;
    UPROPERTY(EditDefaultsOnly, Replicated, Category="Health", meta=(ClampMin="1")) float MaxHealth = 100.f;
    UPROPERTY(ReplicatedUsing=OnRep_Health) float CurrentHealth = 100.f;
    UPROPERTY(EditDefaultsOnly, Replicated, Category="Team") uint8 TeamId = 255;
    UPROPERTY(EditDefaultsOnly, Category="Team") bool bAllowFriendlyFire = false;
    UPROPERTY(EditDefaultsOnly, Category="Team") bool bAllowSelfDamage = false;

private:
    UFUNCTION() void OnRep_Health(float PreviousHealth);
    UFUNCTION() void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* EventInstigator, AActor* DamageCauser);
    void BroadcastHealthChange(float PreviousHealth);
};
