#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DownfallPlayerController.generated.h"

/** Local presentation controller; authority stays with movement/gameplay components. */
UCLASS()
class DOWNFALL_API ADownfallPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void ToggleMouseCapture();
	bool bMenusCaptureMouse = false;
};
