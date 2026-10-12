#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DownfallHUD.generated.h"

class ADownfallCaptureObjective;
class ADownfallWorldDirector;
class ADownfallCinematicDirector;
class ADownfallVehicle;

/** Field HUD with viewport scaling, contextual prompts and optional help. */
UCLASS()
class DOWNFALL_API ADownfallHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    TWeakObjectPtr<ADownfallCaptureObjective> Objective;
    TWeakObjectPtr<ADownfallWorldDirector> WorldDirector;
    TWeakObjectPtr<ADownfallCinematicDirector> Cinematic;
    TWeakObjectPtr<ADownfallVehicle> NearbyVehicle;
    bool bShowControls = false;
    double NextDiscoveryTime = 0.0;
    void RefreshSceneReferences();
};
