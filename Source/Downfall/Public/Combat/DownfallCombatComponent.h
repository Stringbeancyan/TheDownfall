#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "TimerManager.h"
#include "Engine/NetSerialization.h"
#include "Combat/DownfallWeaponDefinition.h"
#include "DownfallCombatComponent.generated.h"

class APawn;
class ADownfallProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDownfallShotEvent, FVector, MuzzleLocation, FVector, Direction, FName, WeaponId);

USTRUCT()
struct FDownfallMagazineState
{
    GENERATED_BODY()
    UPROPERTY() int32 Magazine = 0;
    UPROPERTY() int32 Reserve = 0;
};

/** Input requests contain intent only. The server chooses origin, aim, damage and cadence. */
UCLASS(ClassGroup=(Downfall), meta=(BlueprintSpawnableComponent))
class DOWNFALL_API UDownfallCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDownfallCombatComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintCallable, Category="Combat") void StartFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void StopFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void Reload();
    UFUNCTION(BlueprintCallable, Category="Combat") void EquipNextWeapon();
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetAmmo() const;
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetReserveAmmo() const;
    UFUNCTION(BlueprintPure, Category="Combat") FText GetWeaponName() const;
    UFUNCTION(BlueprintPure, Category="Combat") bool IsReloading() const { return bReloading; }
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetWeaponIndex() const { return EquippedWeaponIndex; }
    /** Bind from a character Blueprint without replacing its native combat component. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Presentation") FDownfallShotEvent OnShot;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditDefaultsOnly, Category="Combat") TArray<FDownfallWeaponSpec> Weapons;
    UPROPERTY(EditDefaultsOnly, Category="Combat") TSubclassOf<ADownfallProjectile> ProjectileClass;
    UPROPERTY(Replicated) int32 EquippedWeaponIndex = 0;
    UPROPERTY(Replicated) TArray<FDownfallMagazineState> Magazines;
    UPROPERTY(Replicated) bool bReloading = false;
    UFUNCTION(BlueprintImplementableEvent, Category="Combat|Presentation")
    void OnShotPresentation(FVector MuzzleLocation, FVector Direction, FName WeaponId);

private:
    UFUNCTION(Server, Reliable) void ServerStartFire();
    UFUNCTION(Server, Reliable) void ServerStopFire();
    UFUNCTION(Server, Reliable) void ServerReload();
    UFUNCTION(Server, Reliable) void ServerEquipNextWeapon();
    UFUNCTION(NetMulticast, Unreliable) void MulticastShot(FVector_NetQuantize10 Origin,
        FVector_NetQuantizeNormal Direction, FName WeaponId);
    UFUNCTION() void HandleOwnerDeath();
    void FireOrSchedule();
    void StopFiringAuthority();
    void CompleteReload();
    bool ConsumeRequest();
    bool CanOperate() const;
    const FDownfallWeaponSpec* GetWeapon() const;
    bool BuildShot(const APawn& Pawn, FVector& OutMuzzle, FVector& OutDirection) const;

    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    bool bTriggerHeld = false;
    bool bLocalTriggerHeld = false;
    int32 ReloadWeaponIndex = INDEX_NONE;
    // This deadline deliberately survives release, equip and reload. Input spam cannot reset it.
    double NextAllowedShotTime = 0.0;
    double LastRequestTime = 0.0;
    float RequestTokens = 12.f;
};
