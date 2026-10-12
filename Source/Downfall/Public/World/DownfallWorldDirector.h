#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "DownfallWorldDirector.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class ULightComponent;
class UMaterialParameterCollection;
class USkyLightComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EDownfallWeatherPreset : uint8 { Clear, Rain, Storm, Fog, Snow };

USTRUCT(BlueprintType)
struct FDownfallWeatherSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float Rain = 0.0f;
    UPROPERTY(BlueprintReadOnly) float Snow = 0.0f;
    UPROPERTY(BlueprintReadOnly) float FogDensity = 0.006f;
    UPROPERTY(BlueprintReadOnly) float WindSpeed = 1.0f; // metres/second
    UPROPERTY(BlueprintReadOnly) float CloudCoverage = 0.1f;
    UPROPERTY(BlueprintReadOnly) float Wetness = 0.0f;
};

USTRUCT()
struct FDownfallWorldClock
{
    GENERATED_BODY()
    UPROPERTY() double AnchorServerSeconds = 0.0;
    UPROPERTY() float AnchorHour = 16.0f;
    UPROPERTY() float SecondsPerDay = 1800.0f;
    UPROPERTY() uint16 DayOfYear = 120;
    UPROPERTY() float LatitudeDegrees = 41.0f;
};

USTRUCT()
struct FDownfallWeatherTransition
{
    GENERATED_BODY()
    UPROPERTY() FDownfallWeatherSample From;
    UPROPERTY() EDownfallWeatherPreset Target = EDownfallWeatherPreset::Rain;
    UPROPERTY() double AnchorServerSeconds = 0.0;
    UPROPERTY() float DurationSeconds = 30.0f;
};

/** Replicates transition endpoints, never per-frame cosmetic lighting values.
 * Scene actors: tag directional lights Downfall.Sun / Downfall.Moon and night lamps
 * Downfall.NightLight. Precipitation and surface response require authored VFX/materials.
 */
UCLASS()
class DOWNFALL_API ADownfallWorldDirector : public AActor
{
    GENERATED_BODY()
public:
    ADownfallWorldDirector();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure, Category="Downfall|World") float GetTimeOfDayHours() const;
    UFUNCTION(BlueprintPure, Category="Downfall|World") float GetRainIntensity() const;
    UFUNCTION(BlueprintPure, Category="Downfall|World") FText GetWeatherLabel() const;
    UFUNCTION(BlueprintPure, Category="Downfall|World") FDownfallWeatherSample GetWeatherSample() const;
    UFUNCTION(BlueprintPure, Category="Downfall|World") float GetApproximateMoonPhase() const;
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Downfall|World")
    void SetTimeOfDayHours(float Hours);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Downfall|World")
    void TransitionWeather(EDownfallWeatherPreset Preset, float DurationSeconds = 30.0f);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditAnywhere, Category="World|Clock", meta=(ClampMin="0", ClampMax="24")) float InitialHour = 16.0f;
    UPROPERTY(EditAnywhere, Category="World|Clock", meta=(ClampMin="60")) float RealSecondsPerDay = 1800.0f;
    UPROPERTY(EditAnywhere, Category="World|Clock", meta=(ClampMin="1", ClampMax="365")) int32 InitialDayOfYear = 120;
    UPROPERTY(EditAnywhere, Category="World|Clock", meta=(ClampMin="-89.9", ClampMax="89.9")) float Latitude = 41.0f;
    UPROPERTY(EditAnywhere, Category="World|Weather") EDownfallWeatherPreset InitialWeather = EDownfallWeatherPreset::Rain;
    UPROPERTY(EditAnywhere, Category="World|Weather") bool bAutomaticWeatherCycle = true;
    UPROPERTY(EditAnywhere, Category="World|Weather", meta=(ClampMin="10")) float WeatherPeriodSeconds = 120.f;
    UPROPERTY(EditAnywhere, Category="World|Materials") TObjectPtr<UMaterialParameterCollection> WorldParameters;

private:
    UPROPERTY(ReplicatedUsing=OnRep_Environment) FDownfallWorldClock Clock;
    UPROPERTY(ReplicatedUsing=OnRep_Environment) FDownfallWeatherTransition Weather;
    UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> SunLight;
    UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> MoonLight;
    UPROPERTY(Transient) TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY(Transient) TObjectPtr<UExponentialHeightFogComponent> FogComponent;
    UPROPERTY(Transient) TArray<TObjectPtr<ULightComponent>> NightLights;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> SurfaceMaterials;
    TArray<float> NightLightIntensities;
    FTimerHandle CosmeticsTimer;
    FTimerHandle WeatherCycleTimer;
    void AdvanceWeatherCycle();
    UFUNCTION() void OnRep_Environment();
    void DiscoverSceneLighting();
    void DiscoverSurfaceMaterials();
    void UpdateCosmetics();
    double ServerTime() const;
    static FDownfallWeatherSample PresetSample(EDownfallWeatherPreset Preset);
};
