#include "Presentation/DownfallHUD.h"
#include "Presentation/DownfallCinematicDirector.h"
#include "Character/DownfallCharacter.h"
#include "Combat/DownfallHealthComponent.h"
#include "Combat/DownfallCombatComponent.h"
#include "Vehicle/DownfallVehicle.h"
#include "World/DownfallWorldDirector.h"
#include "World/DownfallCaptureObjective.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void ADownfallHUD::RefreshSceneReferences()
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextDiscoveryTime) return;
    NextDiscoveryTime = Now + 2.0;
    if (!WorldDirector.IsValid())
        for (TActorIterator<ADownfallWorldDirector> It(GetWorld()); It; ++It) { WorldDirector = *It; break; }
    if (!Objective.IsValid())
        for (TActorIterator<ADownfallCaptureObjective> It(GetWorld()); It; ++It) { Objective = *It; break; }
    if (!Cinematic.IsValid())
        for (TActorIterator<ADownfallCinematicDirector> It(GetWorld()); It; ++It) { Cinematic = *It; break; }
}

void ADownfallHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !GEngine) return;
    RefreshSceneReferences();
    const float Scale = FMath::Clamp(Canvas->ClipY / 1080.f, 0.7f, 1.5f);
    const float Left = 32.f * Scale;
    const FLinearColor White(0.88f, 0.93f, 0.94f);
    const FLinearColor Accent(0.33f, 0.82f, 0.78f);
    UFont* Font = GEngine->GetMediumFont();
    DrawText(TEXT("DOWNFALL  /  BREAKWATER"), White, Left, 26.f * Scale, Font, Scale);
    DrawText(TEXT("SOURCE PROTOTYPE"), Accent, Left, 54.f * Scale, Font, 0.8f * Scale);
    FString TimeWeather = TEXT("Synchronizing environment...");
    if (WorldDirector.IsValid())
    {
        const float Hours = WorldDirector->GetTimeOfDayHours();
        const int32 MinuteOfDay = FMath::FloorToInt(Hours * 60.f) % 1440;
        TimeWeather = FString::Printf(TEXT("%02d:%02d  |  %s"), MinuteOfDay / 60, MinuteOfDay % 60,
            *WorldDirector->GetWeatherLabel().ToString());
    }
    DrawText(TimeWeather, White, Left, 82.f * Scale, Font, 0.85f * Scale);
    DrawText(TEXT("RESTORE THE RELAY — hold the marked zone"), White, Left, 120.f * Scale, Font, Scale);
    if (Objective.IsValid())
    {
        const float Progress = FMath::Clamp(Objective->GetCaptureProgress(), 0.f, 1.f);
        const bool bRestored = Objective->GetOwnerTeam() != 255;
        DrawRect(FLinearColor(0.02f,0.04f,0.05f,0.8f), Left, 153.f * Scale, 260.f * Scale, 6.f * Scale);
        DrawRect(Accent, Left, 153.f * Scale, 260.f * Scale * (bRestored ? 1.f : Progress), 6.f * Scale);
        DrawText(bRestored ? TEXT("RELAY ONLINE") : Objective->IsContested() ? TEXT("CONTESTED") :
            FString::Printf(TEXT("UPLINK %.0f%%"), Progress * 100.f), Accent, Left, 170.f * Scale, Font, 0.85f * Scale);
    }
    if (const auto* Character = Cast<ADownfallCharacter>(PlayerOwner->GetPawn()))
    {
        const auto* Health = Character->GetHealthComponent();
        const auto* Combat = Character->GetCombatComponent();
        DrawText(FString::Printf(TEXT("HEALTH %.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth()),
            White, Left, Canvas->ClipY - 103.f * Scale, Font, Scale);
        DrawText(FString::Printf(TEXT("%s    %d / %d  %s"), *Combat->GetWeaponName().ToString(),
            Combat->GetAmmo(), Combat->GetReserveAmmo(), Combat->IsReloading() ? TEXT("RELOADING") : TEXT("")),
            White, Left, Canvas->ClipY - 74.f * Scale, Font, Scale);
        // Project the head-origin aim used by the server, accounting for shoulder-camera parallax.
        const FVector AimStart = Character->GetPawnViewLocation();
        const FVector AimEnd = AimStart + Character->GetBaseAimRotation().Vector() * 150000.f;
        FHitResult AimHit;
        FCollisionQueryParams AimParams(SCENE_QUERY_STAT(HUDAim), true, Character);
        const bool bHit = GetWorld()->LineTraceSingleByChannel(AimHit, AimStart, AimEnd, ECC_Visibility, AimParams);
        FVector2D Reticle(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
        PlayerOwner->ProjectWorldLocationToScreen(bHit ? AimHit.ImpactPoint : AimEnd, Reticle, true);
        const float CX = Reticle.X, CY = Reticle.Y;
        DrawLine(CX - 7, CY, CX - 3, CY, White);
        DrawLine(CX + 3, CY, CX + 7, CY, White);
        DrawLine(CX, CY - 7, CX, CY - 3, White);
        DrawLine(CX, CY + 3, CX, CY + 7, White);
        DrawText(TEXT("WASD move  |  Mouse aim/fire  |  R reload  |  Q weapon  |  E vehicle  |  F8 intro / F9 skip"),
            White, Left, Canvas->ClipY - 35.f * Scale, Font, 0.73f * Scale);
    }
    else if (const auto* Vehicle = Cast<ADownfallVehicle>(PlayerOwner->GetPawn()))
    {
        DrawText(FString::Printf(TEXT("UTILITY VEHICLE    %.0f km/h"), Vehicle->GetSpeedKmh()), White,
            Left, Canvas->ClipY - 80.f * Scale, Font, Scale);
        DrawText(TEXT("W/S throttle/reverse  |  A/D steer  |  Space brake  |  E exit when stopped"), White,
            Left, Canvas->ClipY - 40.f * Scale, Font, 0.8f * Scale);
    }
    if (Cinematic.IsValid() && Cinematic->IsPlayingForPlayer(PlayerOwner))
    {
        const FString Subtitle = Cinematic->GetSubtitleForPlayer(PlayerOwner).ToString();
        float Width = 0.f, Height = 0.f;
        GetTextSize(Subtitle, Width, Height, Font, Scale);
        const float X = FMath::Max(Left, (Canvas->ClipX - Width) * 0.5f);
        const float Y = Canvas->ClipY * 0.79f;
        DrawRect(FLinearColor(0.f,0.f,0.f,0.75f), X - 12, Y - 6, Width + 24, Height + 12);
        DrawText(Subtitle, White, X, Y, Font, Scale);
    }
}
