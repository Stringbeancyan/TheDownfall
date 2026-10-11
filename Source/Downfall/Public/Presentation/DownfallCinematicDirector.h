#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DownfallCinematicDirector.generated.h"

class ACameraActor;
class APlayerController;

/** Camera coordinates are relative to the placed director, in centimetres. */
USTRUCT(BlueprintType)
struct DOWNFALL_API FDownfallCinematicShot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    FVector StartLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    FVector EndLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    FVector LookAtLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0.1", ClampMax="60.0"))
    float DurationSeconds = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="20.0", ClampMax="120.0"))
    float HorizontalFieldOfView = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FText Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(MultiLine="true"))
    FText Subtitle;
};

/** Local-only, camera-only presentation state. No input locks are acquired. */
struct FDownfallLocalPresentation
{
    TWeakObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<AActor> PreviousViewTarget;
    TWeakObjectPtr<ACameraActor> Camera;
    FVector InitialLocation = FVector::ZeroVector;
    FQuat InitialRotation = FQuat::Identity;
    float InitialFOV = 90.0f;
    float ElapsedSeconds = 0.0f;
    int32 ShotIndex = 0;
};

/**
 * PROTOTYPED: optional 12-second "Last Light" camera rehearsal with subtitle cues.
 * Start only after a local controller possesses a pawn. The live match continues.
 * Camera possession by another system cancels this presentation; no server RPC is used.
 *
 * Production asset contract (PLANNED): replace these shots with LS_LastLight in
 * /Game/Downfall/Cinematics/LastLight. Three shots: rain-street establish, vehicle-side
 * over-shoulder, robot reveal. Use CineCamera 36 mm sensor at 35/50/70 mm; preserve
 * authored exposure and warm emergency practicals against cold overcast ambience.
 * Sequence bindings must resolve local actors, with Restore State and no game pause.
 * Mira/Orin require original skeletal meshes, recorded performance/VO, facial curves,
 * gaze targets and MetaSounds dialogue routing. No such assets are bundled here.
 * Blueprint integration: local Begin/Skip input -> StartForPlayer/SkipForPlayer;
 * widget binding -> GetSubtitleForPlayer. Never call this through a multicast RPC.
 */
UCLASS(Blueprintable)
class DOWNFALL_API ADownfallCinematicDirector : public AActor
{
    GENERATED_BODY()

public:
    ADownfallCinematicDirector();

    UFUNCTION(BlueprintCallable, Category="Downfall|Cinematic")
    bool StartForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="Downfall|Cinematic")
    void SkipForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintPure, Category="Downfall|Cinematic")
    bool IsPlayingForPlayer(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintPure, Category="Downfall|Cinematic")
    FText GetSubtitleForPlayer(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintPure, Category="Downfall|Cinematic")
    FText GetSceneTitle() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Downfall|Cinematic")
    TArray<FDownfallCinematicShot> Shots;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Downfall|Cinematic", meta=(ClampMin="0.0", ClampMax="2.0"))
    float CameraBlendSeconds = 0.65f;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TArray<FDownfallLocalPresentation> Presentations;

    bool HasCameraOwnership(const FDownfallLocalPresentation& Presentation) const;
    void FinishPresentation(int32 Index, bool bInstant);
    float GetTotalDuration() const;
};
