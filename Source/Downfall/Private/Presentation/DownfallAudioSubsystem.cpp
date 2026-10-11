#include "Presentation/DownfallAudioSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundSubmix.h"

ADownfallAcousticZone::ADownfallAcousticZone()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("AcousticBounds"));
    SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(800.0, 800.0, 300.0));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetGenerateOverlapEvents(false);
    Bounds->SetHiddenInGame(true);
    Profile.Environment = EDownfallAcousticEnvironment::Room;
    Profile.ReverbSend = 0.35f;
    Profile.LowPassFrequencyHz = 18000.0f;
}

bool ADownfallAcousticZone::ContainsPoint(const FVector& WorldLocation) const
{
    if (!Bounds)
    {
        return false;
    }
    const FVector LocalPoint = Bounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    return FMath::Abs(LocalPoint.X) <= Extent.X
        && FMath::Abs(LocalPoint.Y) <= Extent.Y
        && FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

double ADownfallAcousticZone::GetVolume() const
{
    const FVector Extent = Bounds ? Bounds->GetScaledBoxExtent() : FVector::ZeroVector;
    return 8.0 * FMath::Abs(Extent.X * Extent.Y * Extent.Z);
}

bool UDownfallAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return Super::ShouldCreateSubsystem(Outer) && World
        && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
        && World->GetNetMode() != NM_DedicatedServer;
}

void UDownfallAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    DefaultAttenuation = NewObject<USoundAttenuation>(this, NAME_None, RF_Transient);
    DefaultAttenuation->Attenuation.bAttenuate = true;
    DefaultAttenuation->Attenuation.bSpatialize = true;
    DefaultAttenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
    DefaultAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
    DefaultAttenuation->Attenuation.AttenuationShapeExtents = FVector(150.0, 0.0, 0.0);
    DefaultAttenuation->Attenuation.FalloffDistance = MaximumAudibleDistanceCm - 150.0f;
}

void UDownfallAudioSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        for (TPair<int32, FDownfallPendingSpatialSound>& Event : PendingEvents)
        {
            World->GetTimerManager().ClearTimer(Event.Value.TimerHandle);
        }
    }
    PendingEvents.Reset();
    for (const TPair<int32, TObjectPtr<UAudioComponent>>& Event : ActiveEvents)
    {
        if (IsValid(Event.Value))
        {
            Event.Value->OnAudioFinishedNative.RemoveAll(this);
            Event.Value->Stop();
        }
    }
    ActiveEvents.Reset();
    Super::Deinitialize();
}

bool UDownfallAudioSubsystem::GetListenerLocation(int32 LocalPlayerIndex, FVector& OutLocation) const
{
    if (LocalPlayerIndex < 0)
    {
        return false;
    }
    const APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), LocalPlayerIndex);
    if (!Controller || !Controller->IsLocalController() || !Controller->PlayerCameraManager)
    {
        return false;
    }
    OutLocation = Controller->PlayerCameraManager->GetCameraLocation();
    return true;
}

FDownfallAcousticProfile UDownfallAudioSubsystem::GetAcousticProfileAt(FVector Location) const
{
    FDownfallAcousticProfile Result;
    if (!GetWorld() || Location.ContainsNaN())
    {
        return Result;
    }

    const ADownfallAcousticZone* Best = nullptr;
    for (TActorIterator<ADownfallAcousticZone> It(GetWorld()); It; ++It)
    {
        const ADownfallAcousticZone* Zone = *It;
        if (!IsValid(Zone) || !Zone->ContainsPoint(Location))
        {
            continue;
        }
        if (!Best || Zone->Priority > Best->Priority
            || (Zone->Priority == Best->Priority && Zone->GetVolume() < Best->GetVolume())
            || (Zone->Priority == Best->Priority && Zone->GetVolume() == Best->GetVolume()
                && Zone->GetPathName() < Best->GetPathName()))
        {
            Best = Zone;
        }
    }
    if (Best)
    {
        Result = Best->Profile;
    }
    return Result;
}

int32 UDownfallAudioSubsystem::PlaySpatialEvent(USoundBase* Sound, FVector Location,
    const FDownfallSpatialSoundParameters& Parameters)
{
    UWorld* World = GetWorld();
    FVector ListenerLocation;
    if (!World || World->GetNetMode() == NM_DedicatedServer || !IsValid(Sound)
        || Location.ContainsNaN() || !FMath::IsFinite(Parameters.VolumeMultiplier)
        || !FMath::IsFinite(Parameters.PitchMultiplier) || !FMath::IsFinite(Parameters.EventAgeSeconds)
        || !GetListenerLocation(Parameters.LocalPlayerIndex, ListenerLocation)
        || PendingEvents.Num() + ActiveEvents.Num() >= MaximumLocalEvents)
    {
        return INDEX_NONE;
    }
    const double DistanceCm = FVector::Distance(Location, ListenerLocation);
    if (DistanceCm > MaximumAudibleDistanceCm)
    {
        return INDEX_NONE;
    }

    // Wrap safely without signed overflow; live event IDs must remain unique.
    do
    {
        NextEventId = NextEventId == MAX_int32 ? 0 : NextEventId + 1;
    }
    while (PendingEvents.Contains(NextEventId) || ActiveEvents.Contains(NextEventId));
    const int32 EventId = NextEventId;

    FDownfallPendingSpatialSound Pending;
    Pending.Sound = Sound;
    Pending.Location = Location;
    Pending.Parameters = Parameters;
    const float Delay = Parameters.bUsePropagationDelay
        ? FMath::Max(0.0f, static_cast<float>(DistanceCm / SpeedOfSoundCmPerSecond)
            - FMath::Max(Parameters.EventAgeSeconds, 0.0f)) : 0.0f;
    PendingEvents.Add(EventId, Pending);
    if (Delay > 0.001f)
    {
        FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UDownfallAudioSubsystem::StartQueuedEvent, EventId);
        World->GetTimerManager().SetTimer(PendingEvents[EventId].TimerHandle, Delegate, Delay, false);
    }
    else
    {
        StartQueuedEvent(EventId);
    }
    return EventId;
}

void UDownfallAudioSubsystem::StartQueuedEvent(int32 EventId)
{
    FDownfallPendingSpatialSound Pending;
    if (!PendingEvents.RemoveAndCopyValue(EventId, Pending))
    {
        return;
    }
    UWorld* World = GetWorld();
    FVector ListenerLocation;
    if (!World || World->GetNetMode() == NM_DedicatedServer || !IsValid(Pending.Sound)
        || !GetListenerLocation(Pending.Parameters.LocalPlayerIndex, ListenerLocation)
        || FVector::DistSquared(Pending.Location, ListenerLocation)
            > FMath::Square(static_cast<double>(MaximumAudibleDistanceCm)))
    {
        return;
    }

    const FDownfallAcousticProfile Profile = GetAcousticProfileAt(ListenerLocation);
    float Volume = FMath::Clamp(Pending.Parameters.VolumeMultiplier, 0.0f, 4.0f);
    float LowPassHz = FMath::Clamp(Profile.LowPassFrequencyHz, 20.0f, 20000.0f);
    if (Pending.Parameters.bTraceOcclusion)
    {
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DownfallAudioOcclusion), false);
        if (const APlayerController* Controller = UGameplayStatics::GetPlayerController(World, Pending.Parameters.LocalPlayerIndex))
        {
            Query.AddIgnoredActor(Controller->GetPawn());
        }
        if (World->LineTraceTestByChannel(ListenerLocation, Pending.Location, ECC_Visibility, Query))
        {
            Volume *= 0.45f;
            LowPassHz = FMath::Min(LowPassHz, 1800.0f);
        }
    }

    USoundAttenuation* Attenuation = Pending.Parameters.Attenuation
        ? Pending.Parameters.Attenuation.Get() : DefaultAttenuation.Get();
    UAudioComponent* Component = UGameplayStatics::SpawnSoundAtLocation(World, Pending.Sound,
        Pending.Location, FRotator::ZeroRotator, Volume,
        FMath::Clamp(Pending.Parameters.PitchMultiplier, 0.25f, 4.0f), 0.0f,
        Attenuation, Pending.Parameters.Concurrency, true);
    if (!Component)
    {
        return;
    }
    Component->SetLowPassFilterEnabled(LowPassHz < 19999.0f);
    Component->SetLowPassFilterFrequency(LowPassHz);
    if (Profile.ReverbSubmix)
    {
        Component->SetSubmixSend(Profile.ReverbSubmix, FMath::Clamp(Profile.ReverbSend, 0.0f, 1.0f));
    }
    Component->OnAudioFinishedNative.AddUObject(this, &UDownfallAudioSubsystem::HandleAudioFinished);
    ActiveEvents.Add(EventId, Component);
}

void UDownfallAudioSubsystem::CancelSpatialEvent(int32 EventId)
{
    FDownfallPendingSpatialSound Pending;
    if (PendingEvents.RemoveAndCopyValue(EventId, Pending) && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(Pending.TimerHandle);
    }
    TObjectPtr<UAudioComponent> Component = nullptr;
    if (ActiveEvents.RemoveAndCopyValue(EventId, Component) && IsValid(Component))
    {
        Component->OnAudioFinishedNative.RemoveAll(this);
        Component->Stop();
    }
}

void UDownfallAudioSubsystem::HandleAudioFinished(UAudioComponent* Component)
{
    for (auto It = ActiveEvents.CreateIterator(); It; ++It)
    {
        if (It.Value().Get() == Component)
        {
            It.RemoveCurrent();
            break;
        }
    }
}
