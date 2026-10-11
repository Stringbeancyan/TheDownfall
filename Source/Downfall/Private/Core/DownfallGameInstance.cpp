#include "Core/DownfallGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/NetDriver.h"

void UDownfallGameInstance::Init()
{
	Super::Init();
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this,
			&UDownfallGameInstance::HandleNetworkFailure);
	}
}

void UDownfallGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	Super::Shutdown();
}

void UDownfallGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (World == GetWorld())
	{
		LastNetworkError = ErrorString;
		UE_LOG(LogTemp, Warning, TEXT("Downfall connection failure (%d): %s"),
			static_cast<int32>(FailureType), *ErrorString);
	}
}
