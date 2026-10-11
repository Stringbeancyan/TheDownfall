#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DownfallWeaponDefinition.generated.h"

/** Designer-facing ballistic parameters; only server values determine gameplay. */
USTRUCT(BlueprintType)
struct DOWNFALL_API FDownfallWeaponSpec
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FName Id = TEXT("Kestrel");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="200")) int32 MagazineCapacity = 30;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="2000")) int32 InitialReserveAmmo = 120;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.05")) float SecondsBetweenShots = 0.12f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.25")) float ReloadSeconds = 2.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.01")) float Damage = 24.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="100")) float MuzzleSpeed = 56000.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="4")) float GravityScale = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1", ClampMax="5")) float ProjectileLifetime = 3.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10")) float SpreadDegrees = 0.15f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bAutomatic = true;

    /** Runtime validation protects native spawners from malformed edited assets. */
    bool IsValid() const;
    static FDownfallWeaponSpec MakeRifle();
    static FDownfallWeaponSpec MakeSidearm();
};

UCLASS(BlueprintType)
class DOWNFALL_API UDownfallWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon") FDownfallWeaponSpec Weapon;
};
