#include "Presentation/DownfallCinematicDirector.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ADownfallCinematicDirector::ADownfallCinematicDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));

    FDownfallCinematicShot Establish;
    Establish.StartLocation = FVector(-4200.0, -1900.0, 500.0);
    Establish.EndLocation = FVector(-3000.0, -1500.0, 360.0);
    Establish.LookAtLocation = FVector(1000.0, 300.0, 150.0);
    Establish.HorizontalFieldOfView = 65.0f;
    Establish.Speaker = NSLOCTEXT("Downfall", "LastLightMira", "Mira");
    Establish.Subtitle = NSLOCTEXT("Downfall", "LastLightLine1", "Wait. That light was on a second ago.");
    Shots.Add(Establish);

    FDownfallCinematicShot Cover;
    Cover.StartLocation = FVector(-1700.0, -450.0, 165.0);
    Cover.EndLocation = FVector(-1450.0, -300.0, 155.0);
    Cover.LookAtLocation = FVector(-500.0, 250.0, 160.0);
    Cover.HorizontalFieldOfView = 48.0f;
    Cover.Speaker = NSLOCTEXT("Downfall", "LastLightOrin", "Orin");
    Cover.Subtitle = NSLOCTEXT("Downfall", "LastLightLine2", "Mira. Behind the car. Now.");
    Shots.Add(Cover);

    FDownfallCinematicShot Reveal;
    Reveal.StartLocation = FVector(1700.0, -400.0, 190.0);
    Reveal.EndLocation = FVector(2200.0, -150.0, 160.0);
    Reveal.LookAtLocation = FVector(2700.0, 600.0, 120.0);
    Reveal.HorizontalFieldOfView = 38.0f;
    Reveal.Speaker = Establish.Speaker;
    Reveal.Subtitle = NSLOCTEXT("Downfall", "LastLightLine3", "I see it. Keep your head down.");
    Shots.Add(Reveal);
}

float ADownfallCinematicDirector::GetTotalDuration() const
{
    float Total = 0.0f;
    for (const FDownfallCinematicShot& Shot : Shots)
    {
        Total += FMath::Clamp(Shot.DurationSeconds, 0.1f, 60.0f);
    }
    return Total;
}

bool ADownfallCinematicDirector::StartForPlayer(APlayerController* PlayerController)
{
    if (!GetWorld() || GetNetMode() == NM_DedicatedServer || !IsValid(PlayerController)
        || !PlayerController->IsLocalController() || !PlayerController->GetPawn()
        || !PlayerController->PlayerCameraManager || Shots.IsEmpty()
        || IsPlayingForPlayer(PlayerController))
    {
        return false;
    }

    // Do not interrupt a camera sequence or camera transition belonging to another system.
    APlayerCameraManager* Manager = PlayerController->PlayerCameraManager;
    AActor* PreviousTarget = PlayerController->GetViewTarget();
    if (PreviousTarget != PlayerController->GetPawn()
        || Manager->PendingViewTarget.Target != nullptr)
    {
        return false;
    }

    FDownfallLocalPresentation Presentation;
    Presentation.Controller = PlayerController;
    Presentation.PreviousViewTarget = PreviousTarget;
    Presentation.InitialLocation = Manager->GetCameraLocation();
    Presentation.InitialRotation = Manager->GetCameraRotation().Quaternion();
    Presentation.InitialFOV = Manager->GetFOVAngle();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParameters.ObjectFlags |= RF_Transient;
    ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(
        Presentation.InitialLocation, Presentation.InitialRotation.Rotator(), SpawnParameters);
    if (!Camera)
    {
        return false;
    }

    Camera->SetReplicates(false);
    Camera->SetActorEnableCollision(false);
    Camera->GetCameraComponent()->SetFieldOfView(Presentation.InitialFOV);
    Camera->GetCameraComponent()->bConstrainAspectRatio = false;
    Presentation.Camera = Camera;
    Presentations.Add(Presentation);

    // Start at exactly the existing viewpoint, then dolly toward the first authored shot.
    // Movement and combat input remain live. No input counters or global time are changed.
    PlayerController->SetViewTarget(Camera);
    SetActorTickEnabled(true);
    return true;
}

bool ADownfallCinematicDirector::HasCameraOwnership(const FDownfallLocalPresentation& Presentation) const
{
    const APlayerController* Controller = Presentation.Controller.Get();
    const ACameraActor* Camera = Presentation.Camera.Get();
    if (!Controller || !Camera || !Controller->PlayerCameraManager)
    {
        return false;
    }
    const AActor* Pending = Controller->PlayerCameraManager->PendingViewTarget.Target;
    return Controller->GetViewTarget() == Camera && (!Pending || Pending == Camera);
}

void ADownfallCinematicDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float TotalDuration = GetTotalDuration();
    for (int32 Index = Presentations.Num() - 1; Index >= 0; --Index)
    {
        FDownfallLocalPresentation& Presentation = Presentations[Index];
        const APlayerController* Controller = Presentation.Controller.Get();
        if (!HasCameraOwnership(Presentation) || !Controller || !Controller->GetPawn())
        {
            FinishPresentation(Index, false);
            continue;
        }

        Presentation.ElapsedSeconds += FMath::Max(DeltaSeconds, 0.0f);
        if (Presentation.ElapsedSeconds >= TotalDuration || Shots.IsEmpty())
        {
            FinishPresentation(Index, false);
            continue;
        }

        float TimeInShot = Presentation.ElapsedSeconds;
        Presentation.ShotIndex = 0;
        while (Presentation.ShotIndex < Shots.Num() - 1)
        {
            const float Duration = FMath::Clamp(Shots[Presentation.ShotIndex].DurationSeconds, 0.1f, 60.0f);
            if (TimeInShot < Duration)
            {
                break;
            }
            TimeInShot -= Duration;
            ++Presentation.ShotIndex;
        }

        const FDownfallCinematicShot& Shot = Shots[Presentation.ShotIndex];
        const float Alpha = FMath::Clamp(TimeInShot / FMath::Clamp(Shot.DurationSeconds, 0.1f, 60.0f), 0.0f, 1.0f);
        const float EasedAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
        FVector Location = GetActorTransform().TransformPosition(FMath::Lerp(Shot.StartLocation, Shot.EndLocation, EasedAlpha));
        const FVector LookAt = GetActorTransform().TransformPosition(Shot.LookAtLocation);
        FQuat Rotation = (LookAt - Location).Rotation().Quaternion();
        float FOV = FMath::Clamp(Shot.HorizontalFieldOfView, 20.0f, 120.0f);
        const float BlendDuration = FMath::Clamp(CameraBlendSeconds, 0.0f, 2.0f);
        if (BlendDuration > UE_SMALL_NUMBER && Presentation.ElapsedSeconds < BlendDuration)
        {
            const float BlendAlpha = FMath::SmoothStep(0.0f, BlendDuration, Presentation.ElapsedSeconds);
            Location = FMath::Lerp(Presentation.InitialLocation, Location, BlendAlpha);
            Rotation = FQuat::Slerp(Presentation.InitialRotation, Rotation, BlendAlpha).GetNormalized();
            FOV = FMath::Lerp(Presentation.InitialFOV, FOV, BlendAlpha);
        }
        Presentation.Camera->SetActorLocationAndRotation(Location, Rotation);
        Presentation.Camera->GetCameraComponent()->SetFieldOfView(FOV);
    }
    SetActorTickEnabled(!Presentations.IsEmpty());
}

void ADownfallCinematicDirector::FinishPresentation(int32 Index, bool bInstant)
{
    if (!Presentations.IsValidIndex(Index))
    {
        return;
    }
    const FDownfallLocalPresentation Presentation = Presentations[Index];
    Presentations.RemoveAtSwap(Index);
    APlayerController* Controller = Presentation.Controller.Get();
    ACameraActor* Camera = Presentation.Camera.Get();
    const float BlendDuration = bInstant ? 0.0f : FMath::Clamp(CameraBlendSeconds, 0.0f, 2.0f);
    if (HasCameraOwnership(Presentation))
    {
        AActor* ReturnTarget = Presentation.PreviousViewTarget.Get();
        // A respawn may have replaced the pawn since playback began.
        if (!IsValid(ReturnTarget) || (Cast<APawn>(ReturnTarget) && ReturnTarget != Controller->GetPawn()))
        {
            ReturnTarget = Controller->GetPawn();
        }
        Controller->SetViewTargetWithBlend(ReturnTarget ? ReturnTarget : Controller,
            BlendDuration, VTBlend_Cubic, 0.0f, true);
    }
    if (Camera)
    {
        if (bInstant)
        {
            Camera->Destroy();
        }
        else
        {
            // Keep the outgoing camera alive until the camera-manager blend has finished.
            Camera->SetLifeSpan(BlendDuration + 1.0f);
        }
    }
    SetActorTickEnabled(!Presentations.IsEmpty());
}

void ADownfallCinematicDirector::SkipForPlayer(APlayerController* PlayerController)
{
    const int32 Index = Presentations.IndexOfByPredicate([PlayerController](const FDownfallLocalPresentation& Presentation)
    {
        return Presentation.Controller.Get() == PlayerController;
    });
    FinishPresentation(Index, false);
}

bool ADownfallCinematicDirector::IsPlayingForPlayer(APlayerController* PlayerController) const
{
    return PlayerController && Presentations.ContainsByPredicate([PlayerController](const FDownfallLocalPresentation& Presentation)
    {
        return Presentation.Controller.Get() == PlayerController;
    });
}

FText ADownfallCinematicDirector::GetSubtitleForPlayer(APlayerController* PlayerController) const
{
    const FDownfallLocalPresentation* Presentation = Presentations.FindByPredicate([PlayerController](const FDownfallLocalPresentation& Candidate)
    {
        return PlayerController && Candidate.Controller.Get() == PlayerController;
    });
    if (!Presentation || !Shots.IsValidIndex(Presentation->ShotIndex))
    {
        return FText::GetEmpty();
    }
    const FDownfallCinematicShot& Shot = Shots[Presentation->ShotIndex];
    return FText::Format(NSLOCTEXT("Downfall", "SubtitleWithSpeaker", "{0}: {1}"), Shot.Speaker, Shot.Subtitle);
}

FText ADownfallCinematicDirector::GetSceneTitle() const
{
    return NSLOCTEXT("Downfall", "LastLightTitle", "Last Light");
}

void ADownfallCinematicDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    while (!Presentations.IsEmpty())
    {
        FinishPresentation(Presentations.Num() - 1, true);
    }
    Super::EndPlay(EndPlayReason);
}
