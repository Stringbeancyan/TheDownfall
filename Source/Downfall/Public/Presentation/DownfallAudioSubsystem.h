#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "DownfallAudioSubsystem.generated.h"

class UAudioComponent;
class UBoxComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;
class USoundSubmixBase;

UENUM(BlueprintType)
enum class EDownfallAcousticEnvironment : uint8
{
    OpenAir,
    Room,
    ConcreteGarage,
    Tunnel
};

/** Supplied submixes require their own authored reverb/early-reflection effect chain. */
USTRUCT(BlueprintType)
struct DOWNFALL_API FDownfallAcousticProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    EDownfallAcousticEnvironment Environment = EDownfallAcousticEnvironment::OpenAir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ReverbSend = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics", meta=(ClampMin="20.0", ClampMax="20000.0"))
    float LowPassFrequencyHz = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    TObjectPtr<USoundSubmixBase> ReverbSubmix = nullptr;
};

/** Lightweight authoring volume; acoustics never rely on collision/overlap events. */
UCLASS(Blueprintable)
class DOWNFALL_API ADownfallAcousticZone : public AActor
{
    GENERATED_BODY()

public:
    ADownfallAcousticZone();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Acoustics")
    TObjectPtr<UBoxComponent> Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Acoustics")
    FDownfallAcousticProfile Profile;

    /** Higher priority wins; equal priority selects the smaller containing volume. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Acoustics")
    int32 Priority = 0;

    bool ContainsPoint(const FVector& WorldLocation) const;
    double GetVolume() const;
};

USTRUCT(BlueprintType)
struct DOWNFALL_API FDownfallSpatialSoundParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound", meta=(ClampMin="0.0", ClampMax="4.0"))
    float VolumeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound", meta=(ClampMin="0.25", ClampMax="4.0"))
    float PitchMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    bool bUsePropagationDelay = true;

    /** Age of a received authoritative event, using synchronised server time, not wall time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound", meta=(ClampMin="0.0"))
    float EventAgeSeconds = 0.0f;

    /** One visibility ray at playback start: a prototype, not acoustic ray tracing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    bool bTraceOcclusion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound", meta=(ClampMin="0"))
    int32 LocalPlayerIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    TObjectPtr<USoundAttenuation> Attenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    TObjectPtr<USoundConcurrency> Concurrency = nullptr;
};

USTRUCT()
struct FDownfallPendingSpatialSound
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    FDownfallSpatialSoundParameters Parameters;

    FTimerHandle TimerHandle;
};

/**
 * PROTOTYPED: local one-shot playback, distance delay, box-zone submix sends and
 * a single-ray occlusion filter. Missing sound assets return INDEX_NONE silently.
 * No audio is generated or bundled. No audio RPC is sent; invoke from validated
 * local presentation of replicated gameplay events, never from an untrusted client RPC.
 *
 * Asset contract (PLANNED): /Game/Downfall/Audio/MetaSounds/MS_Weapon_Rifle,
 * MS_Weapon_Sidearm, MS_RainExterior, MS_RobotServo and MS_VehicleMotor;
 * submixes SMX_SFX/Dialogue/Ambience/Music/Voice plus SMX_RoomReturn and
 * SMX_GarageReturn. Provide licensed/original recordings, mono positional sources,
 * concurrency limits and virtualization rules. Weapon graph inputs are Distance,
 * Surface, InteriorMix and Suppressed, with mechanical/blast/tail layers.
 * ReverbSubmix must have an actual reverb effect preset to produce reverberation.
 * Spatialization uses the engine/device backend. Object-based Atmos, voice chat,
 * portal acoustics, reflections, dynamic propagation and hearing effects are not
 * implemented by this class. Submix sends/filters are sampled once per one-shot.
 */
UCLASS()
class DOWNFALL_API UDownfallAudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /** Returns a cancellable local ID or INDEX_NONE for a rejected/missing event. */
    UFUNCTION(BlueprintCallable, Category="Downfall|Audio")
    int32 PlaySpatialEvent(USoundBase* Sound, FVector Location, const FDownfallSpatialSoundParameters& Parameters);

    UFUNCTION(BlueprintCallable, Category="Downfall|Audio")
    void CancelSpatialEvent(int32 EventId);

    UFUNCTION(BlueprintPure, Category="Downfall|Audio")
    FDownfallAcousticProfile GetAcousticProfileAt(FVector Location) const;

private:
    // A local presentation budget; changing it has no effect on authoritative gameplay.
    static constexpr int32 MaximumLocalEvents = 128;
    static constexpr float SpeedOfSoundCmPerSecond = 34300.0f;
    static constexpr float MaximumAudibleDistanceCm = 100000.0f;

    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> DefaultAttenuation;

    UPROPERTY(Transient)
    TMap<int32, FDownfallPendingSpatialSound> PendingEvents;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UAudioComponent>> ActiveEvents;

    int32 NextEventId = 0;

    bool GetListenerLocation(int32 LocalPlayerIndex, FVector& OutLocation) const;
    void StartQueuedEvent(int32 EventId);
    void HandleAudioFinished(UAudioComponent* Component);
};
