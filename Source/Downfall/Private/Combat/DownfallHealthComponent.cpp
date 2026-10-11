#include "Combat/DownfallHealthComponent.h"
#include "Combat/DownfallCombatRules.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UDownfallHealthComponent::UDownfallHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UDownfallHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority())
    {
        MaxHealth = FMath::IsFinite(MaxHealth) ? FMath::Max(1.f, MaxHealth) : 100.f;
        CurrentHealth = MaxHealth;
        GetOwner()->OnTakeAnyDamage.AddDynamic(this, &ThisClass::HandleAnyDamage);
    }
}

void UDownfallHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDownfallHealthComponent, CurrentHealth);
    DOREPLIFETIME(UDownfallHealthComponent, MaxHealth);
    DOREPLIFETIME(UDownfallHealthComponent, TeamId);
}

void UDownfallHealthComponent::SetTeamId(const uint8 InTeamId)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    TeamId = InTeamId;
    GetOwner()->ForceNetUpdate();
}

void UDownfallHealthComponent::InitializeHealth(const float InMaxHealth)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !FMath::IsFinite(InMaxHealth)) return;
    const float Previous = CurrentHealth;
    MaxHealth = FMath::Clamp(InMaxHealth, 1.f, 100000.f);
    CurrentHealth = MaxHealth;
    BroadcastHealthChange(Previous);
    GetOwner()->ForceNetUpdate();
}

void UDownfallHealthComponent::HandleAnyDamage(AActor* DamagedActor, const float Damage,
    const UDamageType* DamageType, AController* EventInstigator, AActor* DamageCauser)
{
    ApplyDamage(Damage, EventInstigator, DamageCauser);
}

float UDownfallHealthComponent::ApplyDamage(const float Damage, AController* EventInstigator, AActor* DamageCauser)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !IsAlive() || !FMath::IsFinite(Damage) || Damage <= 0.f) return 0.f;

    // Prefer the projectile's original pawn over a controller that may have changed possession.
    AActor* Source = DamageCauser ? DamageCauser->GetInstigator() : nullptr;
    if (!Source && EventInstigator) Source = EventInstigator->GetPawn();
    if (!Source) Source = DamageCauser;
    const UDownfallHealthComponent* SourceHealth = Source ? Source->FindComponentByClass<UDownfallHealthComponent>() : nullptr;
    const uint8 SourceTeam = SourceHealth ? SourceHealth->GetTeamId() : DownfallCombatRules::NeutralTeam;
    if (!DownfallCombatRules::CanDamage(SourceTeam, TeamId, Source == Owner, bAllowSelfDamage, bAllowFriendlyFire)) return 0.f;

    const float Previous = CurrentHealth;
    CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);
    BroadcastHealthChange(Previous);
    Owner->ForceNetUpdate();
    return Previous - CurrentHealth;
}

void UDownfallHealthComponent::OnRep_Health(const float PreviousHealth)
{
    BroadcastHealthChange(PreviousHealth);
}

void UDownfallHealthComponent::BroadcastHealthChange(const float PreviousHealth)
{
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    if (PreviousHealth > 0.f && CurrentHealth <= 0.f) OnDeath.Broadcast();
}
