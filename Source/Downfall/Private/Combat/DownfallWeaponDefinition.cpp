#include "Combat/DownfallWeaponDefinition.h"

bool FDownfallWeaponSpec::IsValid() const
{
    return MagazineCapacity > 0 && MagazineCapacity <= 200 && InitialReserveAmmo >= 0 && InitialReserveAmmo <= 2000
        && FMath::IsFinite(SecondsBetweenShots) && SecondsBetweenShots >= 0.05f && SecondsBetweenShots <= 10.f
        && FMath::IsFinite(ReloadSeconds) && ReloadSeconds >= 0.25f && ReloadSeconds <= 30.f
        && FMath::IsFinite(Damage) && Damage > 0.f && Damage <= 1000.f
        && FMath::IsFinite(MuzzleSpeed) && MuzzleSpeed >= 100.f && MuzzleSpeed <= 100000.f
        && FMath::IsFinite(GravityScale) && GravityScale >= 0.f && GravityScale <= 4.f
        && FMath::IsFinite(ProjectileLifetime) && ProjectileLifetime >= 0.1f && ProjectileLifetime <= 5.f
        && FMath::IsFinite(SpreadDegrees) && SpreadDegrees >= 0.f && SpreadDegrees <= 10.f;
}

FDownfallWeaponSpec FDownfallWeaponSpec::MakeRifle()
{
    FDownfallWeaponSpec Spec;
    Spec.Id = TEXT("Kestrel");
    Spec.DisplayName = NSLOCTEXT("Downfall", "KestrelRifle", "KESTREL C6 / COIL RIFLE");
    return Spec;
}

FDownfallWeaponSpec FDownfallWeaponSpec::MakeSidearm()
{
    FDownfallWeaponSpec Spec;
    Spec.Id = TEXT("Vesper");
    Spec.DisplayName = NSLOCTEXT("Downfall", "VesperSidearm", "VESPER P12 / SIDEARM");
    Spec.MagazineCapacity = 12;
    Spec.InitialReserveAmmo = 60;
    Spec.SecondsBetweenShots = 0.28f;
    Spec.ReloadSeconds = 1.55f;
    Spec.Damage = 34.f;
    Spec.MuzzleSpeed = 34000.f;
    Spec.SpreadDegrees = 0.3f;
    Spec.bAutomatic = false;
    return Spec;
}
