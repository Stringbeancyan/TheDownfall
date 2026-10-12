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
#include "InputCoreTypes.h"

void ADownfallHUD::RefreshSceneReferences()
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextDiscoveryTime) return;
    NextDiscoveryTime = Now + 1.0;
    if (!WorldDirector.IsValid())
        for (TActorIterator<ADownfallWorldDirector> It(GetWorld()); It; ++It) { WorldDirector = *It; break; }
    if (!Objective.IsValid())
        for (TActorIterator<ADownfallCaptureObjective> It(GetWorld()); It; ++It) { Objective = *It; break; }
    if (!Cinematic.IsValid())
        for (TActorIterator<ADownfallCinematicDirector> It(GetWorld()); It; ++It) { Cinematic = *It; break; }
    NearbyVehicle.Reset();
    if (const APawn* LocalPawn = PlayerOwner->GetPawn())
    {
        float BestDistance = FMath::Square(350.f);
        for (TActorIterator<ADownfallVehicle> It(GetWorld()); It; ++It)
        {
            const float Distance = FVector::DistSquared(LocalPawn->GetActorLocation(), It->GetActorLocation());
            if (!It->IsOccupied() && Distance < BestDistance)
            {
                BestDistance = Distance;
                NearbyVehicle = *It;
            }
        }
    }
}

void ADownfallHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !GEngine) return;
    RefreshSceneReferences();
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F1)) bShowControls = !bShowControls;
    const float Scale = FMath::Clamp(FMath::Min(Canvas->ClipX / 1920.f, Canvas->ClipY / 1080.f), .55f, 2.f);
    const float Margin = 48.f * Scale;
    const float Right = Canvas->ClipX - Margin;
    const float Bottom = Canvas->ClipY - Margin;
    const FLinearColor Ink(.91f,.90f,.85f), Quiet(.64f,.66f,.64f), Amber(.85f,.62f,.30f);
    const FLinearColor Red(.92f,.30f,.23f), Back(.012f,.015f,.018f,.78f);
    UFont* Font = GEngine->GetMediumFont();
    auto Label = [&](const FString& Value, float X, float Y, float Size, FLinearColor Color)
    {
        DrawText(Value, FLinearColor(0,0,0,.7f), X + Scale, Y + Scale, Font, Size * Scale);
        DrawText(Value, Color, X, Y, Font, Size * Scale);
    };
    auto RightLabel = [&](const FString& Value, float X, float Y, float Size, FLinearColor Color)
    {
        float Width = 0.f, Height = 0.f;
        GetTextSize(Value, Width, Height, Font, Size * Scale);
        Label(Value, X - Width, Y, Size, Color);
    };

    const bool bInScene = Cinematic.IsValid() && Cinematic->IsPlayingForPlayer(PlayerOwner);
    if (bInScene)
    {
        const float Bar = Canvas->ClipY * .075f;
        DrawRect(FLinearColor::Black, 0, 0, Canvas->ClipX, Bar);
        DrawRect(FLinearColor::Black, 0, Canvas->ClipY - Bar, Canvas->ClipX, Bar);
        const FString Subtitle = Cinematic->GetSubtitleForPlayer(PlayerOwner).ToString();
        float Width = 0.f, Height = 0.f;
        GetTextSize(Subtitle, Width, Height, Font, Scale);
        const float SubtitleScale = FMath::Min(1.f, (Canvas->ClipX - 2.f * Margin) / FMath::Max(1.f, Width));
        GetTextSize(Subtitle, Width, Height, Font, Scale * SubtitleScale);
        const float X = (Canvas->ClipX - Width) * .5f;
        const float Y = Canvas->ClipY - Bar - 76.f * Scale;
        DrawRect(Back, X - 16 * Scale, Y - 9 * Scale, Width + 32 * Scale, Height + 18 * Scale);
        Label(Subtitle, X, Y, SubtitleScale, Ink);
        RightLabel(TEXT("F9  SKIP"), Right, Canvas->ClipY - Bar + 16 * Scale, .7f, Quiet);
        return;
    }

    Label(TEXT("BREAKWATER"), Margin, Margin, .8f, Quiet);
    const bool bRestored = Objective.IsValid() && Objective->GetOwnerTeam() != 255;
    DrawRect(Amber, Margin, Margin + 35 * Scale, 2 * Scale, 50 * Scale);
    Label(bRestored ? TEXT("Relay restored") : TEXT("Restore the relay"), Margin + 14 * Scale, Margin + 30 * Scale, 1.1f, Ink);
    Label(bRestored ? TEXT("Hold position. Watch the approaches.") : TEXT("Reach Relay 03 and secure the uplink."),
        Margin + 14 * Scale, Margin + 62 * Scale, .75f, Quiet);
    if (Objective.IsValid() && !bRestored && Objective->GetCaptureProgress() > 0)
    {
        DrawRect(Back, Margin + 14 * Scale, Margin + 95 * Scale, 240 * Scale, 3 * Scale);
        DrawRect(Objective->IsContested() ? Red : Amber, Margin + 14 * Scale, Margin + 95 * Scale,
            240 * Scale * FMath::Clamp(Objective->GetCaptureProgress(), 0.f, 1.f), 3 * Scale);
        Label(Objective->IsContested() ? TEXT("CONTESTED") : TEXT("UPLINK IN PROGRESS"),
            Margin + 14 * Scale, Margin + 108 * Scale, .65f, Quiet);
    }
    if (WorldDirector.IsValid())
    {
        const int32 MinuteOfDay = FMath::FloorToInt(WorldDirector->GetTimeOfDayHours() * 60.f) % 1440;
        RightLabel(FString::Printf(TEXT("%02d:%02d  /  %s"), MinuteOfDay / 60, MinuteOfDay % 60,
            *WorldDirector->GetWeatherLabel().ToString()), Right, Margin, .75f, Quiet);
    }
    // District convention: +X north, +Y east.
    const float Center = Canvas->ClipX * .5f;
    const float Heading = FRotator::ClampAxis(PlayerOwner->GetControlRotation().Yaw);
    for (int32 CompassTick = 0; CompassTick < 24; ++CompassTick)
    {
        const float Offset = FMath::FindDeltaAngleDegrees(Heading, CompassTick * 15.f);
        if (FMath::Abs(Offset) > 35.f) continue;
        const float X = Center + Offset * 4.f * Scale;
        DrawLine(X, Margin, X, Margin + (CompassTick % 6 == 0 ? 9 : 4) * Scale, Quiet, Scale);
        if (CompassTick % 6 == 0)
        {
            const TCHAR* Cardinal[] = {TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W")};
            Label(Cardinal[CompassTick / 6], X - 5 * Scale, Margin + 14 * Scale, .7f, Ink);
        }
    }
    DrawRect(Amber, Center - Scale, Margin - 7 * Scale, 2 * Scale, 5 * Scale);

    if (const ADownfallCharacter* LocalCharacter = Cast<ADownfallCharacter>(PlayerOwner->GetPawn()))
    {
        const UDownfallHealthComponent* Health = LocalCharacter->GetHealthComponent();
        const UDownfallCombatComponent* Combat = LocalCharacter->GetCombatComponent();
        const float Fraction = FMath::Clamp(Health->GetHealth() / FMath::Max(1.f, Health->GetMaxHealth()), 0.f, 1.f);
        Label(Health->IsAlive() ? TEXT("VITALS") : TEXT("RESPAWNING"), Margin, Bottom - 53 * Scale, .65f, Quiet);
        DrawRect(Back, Margin, Bottom - 19 * Scale, 164 * Scale, 4 * Scale);
        DrawRect(Fraction < .3f ? Red : Ink, Margin, Bottom - 19 * Scale, 164 * Scale * Fraction, 4 * Scale);
        Label(FString::Printf(TEXT("%.0f"), Health->GetHealth()), Margin + 180 * Scale, Bottom - 32 * Scale, 1.f, Ink);
        RightLabel(Combat->GetWeaponName().ToString().ToUpper(), Right, Bottom - 80 * Scale, .7f, Quiet);
        RightLabel(FString::Printf(TEXT("%02d"), Combat->GetAmmo()), Right - 70 * Scale, Bottom - 50 * Scale, 1.9f, Ink);
        RightLabel(FString::Printf(TEXT("/ %03d"), Combat->GetReserveAmmo()), Right, Bottom - 30 * Scale, .85f, Quiet);
        if (Combat->IsReloading() || Combat->GetAmmo() == 0)
            RightLabel(Combat->IsReloading() ? TEXT("RELOADING") : TEXT("R  RELOAD"), Right, Bottom + 3 * Scale, .7f, Amber);
        if (Health->IsAlive())
        {
            const FVector AimStart = LocalCharacter->GetPawnViewLocation();
            const FVector AimEnd = AimStart + LocalCharacter->GetBaseAimRotation().Vector() * 150000.f;
            FHitResult AimHit;
            FCollisionQueryParams AimParams(SCENE_QUERY_STAT(HUDAim), true, LocalCharacter);
            const bool bHit = GetWorld()->LineTraceSingleByChannel(AimHit, AimStart, AimEnd, ECC_Visibility, AimParams);
            FVector2D Reticle;
            if (PlayerOwner->ProjectWorldLocationToScreen(bHit ? AimHit.ImpactPoint : AimEnd, Reticle, true))
            {
                DrawLine(Reticle.X - 6*Scale, Reticle.Y, Reticle.X - 2*Scale, Reticle.Y, Ink, Scale);
                DrawLine(Reticle.X + 2*Scale, Reticle.Y, Reticle.X + 6*Scale, Reticle.Y, Ink, Scale);
                DrawLine(Reticle.X, Reticle.Y + 2*Scale, Reticle.X, Reticle.Y + 6*Scale, Ink, Scale);
            }
            if (NearbyVehicle.IsValid() && !NearbyVehicle->IsOccupied()
                && FVector::DistSquared(LocalCharacter->GetActorLocation(), NearbyVehicle->GetActorLocation()) < FMath::Square(350.f)
                && PlayerOwner->LineOfSightTo(NearbyVehicle.Get()))
                Label(TEXT("E   ENTER VEHICLE"), Center - 90*Scale, Canvas->ClipY * .67f, .85f, Ink);
        }
    }
    else if (const ADownfallVehicle* LocalVehicle = Cast<ADownfallVehicle>(PlayerOwner->GetPawn()))
    {
        RightLabel(FString::Printf(TEXT("%.0f"), LocalVehicle->GetSpeedKmh()), Right, Bottom - 60*Scale, 2.f, Ink);
        RightLabel(TEXT("KM/H"), Right, Bottom - 15*Scale, .7f, Quiet);
        Label(TEXT("UTILITY 04"), Margin, Bottom - 45*Scale, .8f, Quiet);
        Label(LocalVehicle->GetSpeedKmh() < 3.6f ? TEXT("E  EXIT VEHICLE") : TEXT("SPACE  BRAKE"),
            Margin, Bottom - 15*Scale, .8f, Ink);
    }
    if (bShowControls || PlayerOwner->bShowMouseCursor)
    {
        const float W = 440 * Scale, H = 302 * Scale;
        const float X = (Canvas->ClipX - W) * .5f, Y = (Canvas->ClipY - H) * .5f;
        DrawRect(Back, X, Y, W, H);
        Label(TEXT("FIELD CONTROLS"), X+24*Scale, Y+20*Scale, .85f, Ink);
        const TCHAR* Lines[] = {TEXT("WASD   Move       MOUSE   Aim / fire"), TEXT("SHIFT   Sprint     CTRL   Crouch"),
            TEXT("SPACE   Jump       V   Switch shoulder"), TEXT("R   Reload         Q   Change weapon"),
            TEXT("E   Enter / exit vehicle"), TEXT("F8   Intro         F9   Skip intro"), TEXT("F1   Close help    ESC   Mouse capture")};
        for (int32 Row = 0; Row < UE_ARRAY_COUNT(Lines); ++Row)
            Label(Lines[Row], X+24*Scale, Y+(61+Row*27)*Scale, .78f, Quiet);
        Label(TEXT("The session continues while controls are open."), X+24*Scale, Y+269*Scale, .65f, Amber);
    }
    else if (GetWorld()->GetTimeSeconds() < 25.f)
        Label(TEXT("F1  FIELD CONTROLS"), Center - 80*Scale, Bottom, .65f, Quiet);
}
