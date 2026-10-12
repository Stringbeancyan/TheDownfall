#include "World/DownfallWorldDirector.h"
#include "World/DownfallWorldMath.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ADownfallWorldDirector::ADownfallWorldDirector()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    bReplicates = true;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(2.0f);
    PrimaryActorTick.bCanEverTick = false;
}

void ADownfallWorldDirector::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        Clock.AnchorServerSeconds = ServerTime();
        Clock.AnchorHour = DownfallWorldMath::WrapHours(InitialHour);
        Clock.SecondsPerDay = FMath::Max(60.0f, RealSecondsPerDay);
        Clock.DayOfYear = FMath::Clamp(InitialDayOfYear, 1, 365);
        Clock.LatitudeDegrees = FMath::Clamp(Latitude, -89.9f, 89.9f);
        Weather.From = PresetSample(InitialWeather);
        Weather.Target = InitialWeather;
        Weather.AnchorServerSeconds = ServerTime();
        Weather.DurationSeconds = 0.0f;
        if (bAutomaticWeatherCycle)
            GetWorldTimerManager().SetTimer(WeatherCycleTimer, this, &ThisClass::AdvanceWeatherCycle,
                FMath::Max(10.f, WeatherPeriodSeconds), true);
    }
    if (GetNetMode() != NM_DedicatedServer)
    {
        DiscoverSceneLighting();
        UpdateCosmetics();
        GetWorldTimerManager().SetTimer(CosmeticsTimer, this, &ThisClass::UpdateCosmetics, 0.1f, true);
    }
}

void ADownfallWorldDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(CosmeticsTimer);
    GetWorldTimerManager().ClearTimer(WeatherCycleTimer);
    Super::EndPlay(Reason);
}

void ADownfallWorldDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallWorldDirector, Clock);
    DOREPLIFETIME(ADownfallWorldDirector, Weather);
}

double ADownfallWorldDirector::ServerTime() const
{
    if (const AGameStateBase* State = GetWorld()->GetGameState()) return State->GetServerWorldTimeSeconds();
    return GetWorld()->GetTimeSeconds();
}

float ADownfallWorldDirector::GetTimeOfDayHours() const
{
    return DownfallWorldMath::EvaluateHours(Clock.AnchorHour, ServerTime(), Clock.AnchorServerSeconds, Clock.SecondsPerDay);
}

float ADownfallWorldDirector::GetRainIntensity() const { return GetWeatherSample().Rain; }

FText ADownfallWorldDirector::GetWeatherLabel() const
{
    switch (Weather.Target)
    {
    case EDownfallWeatherPreset::Rain: return NSLOCTEXT("Downfall", "Rain", "RAIN");
    case EDownfallWeatherPreset::Storm: return NSLOCTEXT("Downfall", "Storm", "STORM");
    case EDownfallWeatherPreset::Fog: return NSLOCTEXT("Downfall", "Fog", "FOG");
    case EDownfallWeatherPreset::Snow: return NSLOCTEXT("Downfall", "Snow", "SNOW");
    default: return NSLOCTEXT("Downfall", "Clear", "CLEAR");
    }
}

FDownfallWeatherSample ADownfallWorldDirector::PresetSample(EDownfallWeatherPreset Preset)
{
    FDownfallWeatherSample Result;
    switch (Preset)
    {
    case EDownfallWeatherPreset::Rain:
        Result.Rain = .5f; Result.FogDensity = .015f; Result.WindSpeed = 5.f;
        Result.CloudCoverage = .8f; Result.Wetness = 1.f; break;
    case EDownfallWeatherPreset::Storm:
        Result.Rain = 1.f; Result.FogDensity = .035f; Result.WindSpeed = 18.f;
        Result.CloudCoverage = 1.f; Result.Wetness = 1.f; break;
    case EDownfallWeatherPreset::Fog:
        Result.FogDensity = .07f; Result.WindSpeed = .5f; Result.CloudCoverage = .5f;
        Result.Wetness = .2f; break;
    case EDownfallWeatherPreset::Snow:
        Result.Snow = .8f; Result.FogDensity = .03f; Result.WindSpeed = 7.f;
        Result.CloudCoverage = .9f; break;
    default: break;
    }
    return Result;
}

FDownfallWeatherSample ADownfallWorldDirector::GetWeatherSample() const
{
    const float LinearAlpha = Weather.DurationSeconds <= KINDA_SMALL_NUMBER ? 1.0f
        : FMath::Clamp(static_cast<float>((ServerTime() - Weather.AnchorServerSeconds) / Weather.DurationSeconds), 0.f, 1.f);
    const float Alpha = LinearAlpha * LinearAlpha * (3.f - 2.f * LinearAlpha);
    const FDownfallWeatherSample To = PresetSample(Weather.Target);
    FDownfallWeatherSample Result;
    Result.Rain = FMath::Lerp(Weather.From.Rain, To.Rain, Alpha);
    Result.Snow = FMath::Lerp(Weather.From.Snow, To.Snow, Alpha);
    Result.FogDensity = FMath::Lerp(Weather.From.FogDensity, To.FogDensity, Alpha);
    Result.WindSpeed = FMath::Lerp(Weather.From.WindSpeed, To.WindSpeed, Alpha);
    Result.CloudCoverage = FMath::Lerp(Weather.From.CloudCoverage, To.CloudCoverage, Alpha);
    Result.Wetness = FMath::Lerp(Weather.From.Wetness, To.Wetness, Alpha);
    return Result;
}

float ADownfallWorldDirector::GetApproximateMoonPhase() const
{
    return DownfallWorldMath::ApproximateMoonPhase((ServerTime() - Clock.AnchorServerSeconds) / Clock.SecondsPerDay, .5);
}

void ADownfallWorldDirector::SetTimeOfDayHours(float Hours)
{
    if (!HasAuthority() || !FMath::IsFinite(Hours)) return;
    Clock.AnchorHour = DownfallWorldMath::WrapHours(Hours);
    Clock.AnchorServerSeconds = ServerTime();
    ForceNetUpdate();
    OnRep_Environment();
}

void ADownfallWorldDirector::TransitionWeather(EDownfallWeatherPreset Preset, float Duration)
{
    if (!HasAuthority() || !FMath::IsFinite(Duration)
        || static_cast<uint8>(Preset) > static_cast<uint8>(EDownfallWeatherPreset::Snow)) return;
    Weather.From = GetWeatherSample(); // Interrupted transitions remain continuous.
    Weather.Target = Preset;
    Weather.AnchorServerSeconds = ServerTime();
    Weather.DurationSeconds = FMath::Clamp(Duration, 0.f, 3600.f);
    ForceNetUpdate();
    OnRep_Environment();
}

void ADownfallWorldDirector::OnRep_Environment()
{
    if (HasActorBegunPlay() && GetNetMode() != NM_DedicatedServer) UpdateCosmetics();
}

void ADownfallWorldDirector::AdvanceWeatherCycle()
{
    // Scripted temperate-climate rehearsal. Production fronts need authored regional rules.
    EDownfallWeatherPreset Next = EDownfallWeatherPreset::Rain;
    switch (Weather.Target)
    {
        case EDownfallWeatherPreset::Rain: Next = EDownfallWeatherPreset::Storm; break;
        case EDownfallWeatherPreset::Storm: Next = EDownfallWeatherPreset::Fog; break;
        case EDownfallWeatherPreset::Fog: Next = EDownfallWeatherPreset::Clear; break;
        default: break;
    }
    TransitionWeather(Next, FMath::Min(35.f, FMath::Max(1.f, WeatherPeriodSeconds * .5f)));
}

void ADownfallWorldDirector::DiscoverSceneLighting()
{
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(TEXT("Downfall.Sun"))) SunLight = It->FindComponentByClass<UDirectionalLightComponent>();
        if (It->ActorHasTag(TEXT("Downfall.Moon"))) MoonLight = It->FindComponentByClass<UDirectionalLightComponent>();
    }
    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It) { SkyLight = It->FindComponentByClass<USkyLightComponent>(); break; }
    for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It) { FogComponent = It->FindComponentByClass<UExponentialHeightFogComponent>(); break; }
    for (TActorIterator<ALight> It(GetWorld()); It; ++It)
    {
        if (!It->ActorHasTag(TEXT("Downfall.NightLight"))) continue;
        if (ULightComponent* Light = It->GetLightComponent())
        {
            NightLights.Add(Light);
            NightLightIntensities.Add(Light->Intensity);
        }
    }
}

void ADownfallWorldDirector::UpdateCosmetics()
{
    const float Hour = GetTimeOfDayHours();
    const FDownfallWeatherSample Sample = GetWeatherSample();
    const int32 DaysElapsed = FMath::FloorToInt((Clock.AnchorHour +
        (ServerTime() - Clock.AnchorServerSeconds) * 24.0 / Clock.SecondsPerDay) / 24.0);
    const int32 Day = ((static_cast<int32>(Clock.DayOfYear) - 1 + DaysElapsed) % 365 + 365) % 365 + 1;
    const auto Sun = DownfallWorldMath::SolarPosition(Hour, Clock.LatitudeDegrees, Day);
    const float Daylight = FMath::Clamp(static_cast<float>((Sun.AltitudeDegrees + 3.0) / 12.0), 0.f, 1.f);
    if (SunLight)
    {
        SunLight->SetWorldRotation(FRotator(-Sun.AltitudeDegrees, Sun.AzimuthDegrees, 0));
        SunLight->SetIntensity(110000.f * Daylight * (1.f - Sample.CloudCoverage * .8f));
    }
    if (MoonLight)
    {
        // Explicit approximation: phase-adjusted solar orbit, not real lunar declination.
        const float Phase = GetApproximateMoonPhase();
        const auto Moon = DownfallWorldMath::SolarPosition(Hour - 24.f * Phase, Clock.LatitudeDegrees, Day);
        const float Illuminated = .5f * (1.f - FMath::Cos(Phase * 2.f * PI));
        MoonLight->SetWorldRotation(FRotator(-Moon.AltitudeDegrees, Moon.AzimuthDegrees, 0));
        MoonLight->SetIntensity(.2f * Illuminated * (1.f - Daylight)
            * FMath::Clamp(static_cast<float>(Moon.AltitudeDegrees / 8.0), 0.f, 1.f));
    }
    if (SkyLight) SkyLight->SetIntensity(FMath::Lerp(.12f, 1.0f, Daylight));
    if (FogComponent)
    {
        FogComponent->SetFogDensity(Sample.FogDensity);
        FogComponent->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(FLinearColor(.03f,.045f,.07f), FLinearColor(.45f,.52f,.61f), Daylight));
    }
    for (int32 Index = 0; Index < NightLights.Num(); ++Index)
        if (IsValid(NightLights[Index])) NightLights[Index]->SetIntensity(NightLightIntensities[Index] * (1.f - Daylight));
    if (WorldParameters)
    {
        UKismetMaterialLibrary::SetScalarParameterValue(this, WorldParameters, TEXT("RainIntensity"), Sample.Rain);
        UKismetMaterialLibrary::SetScalarParameterValue(this, WorldParameters, TEXT("Wetness"), Sample.Wetness);
        UKismetMaterialLibrary::SetScalarParameterValue(this, WorldParameters, TEXT("SnowCoverage"), Sample.Snow);
        UKismetMaterialLibrary::SetScalarParameterValue(this, WorldParameters, TEXT("WindSpeed"), Sample.WindSpeed);
        UKismetMaterialLibrary::SetScalarParameterValue(this, WorldParameters, TEXT("TimeOfDayHours"), Hour);
    }
}
