#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DownfallHUD.generated.h"

class ADownfallCaptureObjective;
class ADownfallWorldDirector;
class ADownfallCinematicDirector;

/** Native diagnostic slice HUD. Production CommonUI menus are a separate milestone. */
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
    double NextDiscoveryTime = 0.0;
    void RefreshSceneReferences();
};
