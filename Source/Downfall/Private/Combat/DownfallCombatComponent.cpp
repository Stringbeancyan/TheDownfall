#include "Combat/DownfallCombatComponent.h"
#include "Combat/DownfallCombatRules.h"
#include "Combat/DownfallHealthComponent.h"
#include "Combat/DownfallProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UDownfallCombatComponent::UDownfallCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    Weapons = { FDownfallWeaponSpec::MakeRifle(), FDownfallWeaponSpec::MakeSidearm() };
    ProjectileClass = ADownfallProjectile::StaticClass();
}

void UDownfallCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!GetOwner()->HasAuthority()) return;
    if (Weapons.Num() != 2 || !Weapons[0].IsValid() || !Weapons[1].IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid weapon defaults on %s; using validated slice loadout."), *GetOwner()->GetName());
        Weapons = { FDownfallWeaponSpec::MakeRifle(), FDownfallWeaponSpec::MakeSidearm() };
    }
    Magazines.SetNum(Weapons.Num());
    for (int32 Index = 0; Index < Weapons.Num(); ++Index)
    {
        Magazines[Index].Magazine = Weapons[Index].MagazineCapacity;
        Magazines[Index].Reserve = Weapons[Index].InitialReserveAmmo;
    }
    LastRequestTime = GetWorld()->GetTimeSeconds();
    if (UDownfallHealthComponent* Health = GetOwner()->FindComponentByClass<UDownfallHealthComponent>())
        Health->OnDeath.AddDynamic(this, &ThisClass::HandleOwnerDeath);
}

void UDownfallCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void UDownfallCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDownfallCombatComponent, EquippedWeaponIndex);
    DOREPLIFETIME_CONDITION(UDownfallCombatComponent, Magazines, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDownfallCombatComponent, bReloading, COND_OwnerOnly);
}

void UDownfallCombatComponent::StartFire()
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || (!Pawn->IsLocallyControlled() && !Pawn->HasAuthority()) || bLocalTriggerHeld) return;
    bLocalTriggerHeld = true;
    ServerStartFire();
}

void UDownfallCombatComponent::StopFire()
{
    // Input context removal may arrive after the owner loses its controller.
    // Always release local intent, even when this copy may no longer send an RPC.
    bLocalTriggerHeld = false;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || (!Pawn->IsLocallyControlled() && !Pawn->HasAuthority())) return;
    ServerStopFire();
}

void UDownfallCombatComponent::Reload() { ServerReload(); }
void UDownfallCombatComponent::EquipNextWeapon() { ServerEquipNextWeapon(); }

const FDownfallWeaponSpec* UDownfallCombatComponent::GetWeapon() const
{
    return Weapons.IsValidIndex(EquippedWeaponIndex) ? &Weapons[EquippedWeaponIndex] : nullptr;
}

int32 UDownfallCombatComponent::GetAmmo() const
{
    return Magazines.IsValidIndex(EquippedWeaponIndex) ? Magazines[EquippedWeaponIndex].Magazine : 0;
}

int32 UDownfallCombatComponent::GetReserveAmmo() const
{
    return Magazines.IsValidIndex(EquippedWeaponIndex) ? Magazines[EquippedWeaponIndex].Reserve : 0;
}

FText UDownfallCombatComponent::GetWeaponName() const
{
    const FDownfallWeaponSpec* Weapon = GetWeapon();
    return Weapon ? Weapon->DisplayName : FText::GetEmpty();
}

bool UDownfallCombatComponent::CanOperate() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const UDownfallHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UDownfallHealthComponent>() : nullptr;
    return Pawn && Pawn->HasAuthority() && Pawn->GetController() && Health && Health->IsAlive()
        && GetWeapon() && Magazines.IsValidIndex(EquippedWeaponIndex);
}

bool UDownfallCombatComponent::ConsumeRequest()
{
    return GetWorld() && DownfallCombatRules::ConsumeRequestToken(GetWorld()->GetTimeSeconds(), LastRequestTime, RequestTokens);
}

void UDownfallCombatComponent::ServerStartFire_Implementation()
{
    if (!ConsumeRequest() || !CanOperate() || bReloading || bTriggerHeld) return;
    bTriggerHeld = true;
    FireOrSchedule();
}

void UDownfallCombatComponent::ServerStopFire_Implementation()
{
    // Release always succeeds, including when the request budget is depleted.
    StopFiringAuthority();
}

void UDownfallCombatComponent::StopFiringAuthority()
{
    bTriggerHeld = false;
    GetWorld()->GetTimerManager().ClearTimer(FireTimer);
}

void UDownfallCombatComponent::ServerEquipNextWeapon_Implementation()
{
    if (!ConsumeRequest() || !CanOperate() || Weapons.Num() < 2) return;
    StopFiringAuthority();
    GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
    bReloading = false;
    ReloadWeaponIndex = INDEX_NONE;
    EquippedWeaponIndex = (EquippedWeaponIndex + 1) % Weapons.Num();
    GetOwner()->ForceNetUpdate();
}

void UDownfallCombatComponent::ServerReload_Implementation()
{
    if (!ConsumeRequest() || !CanOperate() || bReloading) return;
    const FDownfallWeaponSpec& Weapon = *GetWeapon();
    if (DownfallCombatRules::RoundsToReload(GetAmmo(), Weapon.MagazineCapacity, GetReserveAmmo()) <= 0) return;
    StopFiringAuthority();
    bReloading = true;
    ReloadWeaponIndex = EquippedWeaponIndex;
    GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &ThisClass::CompleteReload, Weapon.ReloadSeconds, false);
    GetOwner()->ForceNetUpdate();
}

void UDownfallCombatComponent::CompleteReload()
{
    if (CanOperate() && bReloading && ReloadWeaponIndex == EquippedWeaponIndex)
    {
        FDownfallMagazineState& Magazine = Magazines[EquippedWeaponIndex];
        const int32 Transfer = DownfallCombatRules::RoundsToReload(Magazine.Magazine, GetWeapon()->MagazineCapacity, Magazine.Reserve);
        Magazine.Magazine += Transfer;
        Magazine.Reserve -= Transfer;
    }
    bReloading = false;
    ReloadWeaponIndex = INDEX_NONE;
    GetOwner()->ForceNetUpdate();
}

void UDownfallCombatComponent::HandleOwnerDeath()
{
    StopFiringAuthority();
    GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
    bReloading = false;
    ReloadWeaponIndex = INDEX_NONE;
}

bool UDownfallCombatComponent::BuildShot(const APawn& Pawn, FVector& OutMuzzle, FVector& OutDirection) const
{
    const FVector PawnPosition = Pawn.GetActorLocation();
    FVector EyePosition = Pawn.GetPawnViewLocation();
    const FRotator AimRotation = Pawn.GetBaseAimRotation();
    if (PawnPosition.ContainsNaN() || EyePosition.ContainsNaN() || AimRotation.ContainsNaN()) return false;
    if (FVector::DistSquared(EyePosition, PawnPosition) > FMath::Square(200.f)) return false;
    const FVector Aim = AimRotation.Vector().GetSafeNormal();
    const FVector ChestPosition = PawnPosition + FVector(0.f, 0.f, 45.f);
    const FRotator BodyYaw(0.f, Pawn.GetActorRotation().Yaw, 0.f);
    OutMuzzle = ChestPosition + BodyYaw.RotateVector(FVector(50.f, 15.f, 0.f));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DownfallShot), true, &Pawn);
    FHitResult Obstruction;
    // Reject muzzle placement through walls. No client position, target, damage or hit is accepted.
    if (GetWorld()->SweepSingleByChannel(Obstruction, ChestPosition, OutMuzzle, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(2.f), Query)) return false;

    const FVector AimEnd = EyePosition + Aim * 150000.f;
    FHitResult AimHit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(AimHit, EyePosition, AimEnd, ECC_Visibility, Query);
    const FVector Target = bHit ? AimHit.ImpactPoint : AimEnd;
    OutDirection = (Target - OutMuzzle).GetSafeNormal();
    // A camera hit behind or directly alongside the barrel should never reverse the projectile.
    if (OutDirection.IsNearlyZero() || FVector::DotProduct(OutDirection, Aim) < 0.5f) OutDirection = Aim;
    return true;
}

void UDownfallCombatComponent::FireOrSchedule()
{
    if (!bTriggerHeld || !CanOperate() || bReloading || GetAmmo() <= 0) { StopFiringAuthority(); return; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (!DownfallCombatRules::CanFireAtTime(Now, NextAllowedShotTime))
    {
        const float Delay = static_cast<float>(FMath::Max(0.001, NextAllowedShotTime - Now));
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &ThisClass::FireOrSchedule, Delay, false);
        return;
    }
    const FDownfallWeaponSpec& Weapon = *GetWeapon();
    // Advance even if obstructed/spawn fails: repeated invalid shots cannot schedule an unbounded loop.
    NextAllowedShotTime = Now + Weapon.SecondsBetweenShots;
    APawn* Pawn = CastChecked<APawn>(GetOwner());
    FVector Muzzle, Direction;
    if (ProjectileClass && BuildShot(*Pawn, Muzzle, Direction))
    {
        Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(Weapon.SpreadDegrees));
        const FTransform SpawnTransform(Direction.Rotation(), Muzzle);
        ADownfallProjectile* Projectile = GetWorld()->SpawnActorDeferred<ADownfallProjectile>(ProjectileClass,
            SpawnTransform, Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Projectile)
        {
            Projectile->InitializeShot(Weapon, Direction);
            UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
            Magazines[EquippedWeaponIndex].Magazine -= 1;
            MulticastShot(Muzzle, Direction, Weapon.Id);
            GetOwner()->ForceNetUpdate();
        }
    }
    if (Weapon.bAutomatic && bTriggerHeld)
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &ThisClass::FireOrSchedule, Weapon.SecondsBetweenShots, false);
}

void UDownfallCombatComponent::MulticastShot_Implementation(const FVector_NetQuantize10 Origin,
    const FVector_NetQuantizeNormal Direction, const FName WeaponId)
{
    if (GetWorld()->GetNetMode() != NM_DedicatedServer)
    {
        OnShot.Broadcast(Origin, Direction, WeaponId);
        OnShotPresentation(Origin, Direction, WeaponId);
    }
}
