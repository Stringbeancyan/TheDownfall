#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DownfallVehicle.generated.h"

class ADownfallCharacter;
class UBoxComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** One-seat, server-moved blockout vehicle. Not a Chaos vehicle or prediction model. */
UCLASS()
class DOWNFALL_API ADownfallVehicle : public APawn
{
    GENERATED_BODY()
public:
    ADownfallVehicle();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void UnPossessed() override;

    /** Authority-only. Distance, life, seat and ownership are checked again here. */
    bool TryEnter(ADownfallCharacter* Character);
    bool TryExit();
    UFUNCTION(BlueprintPure, Category="Downfall|Vehicle") float GetSpeedKmh() const { return FMath::Abs(SpeedCmPerSecond) * 0.036f; }
    UFUNCTION(BlueprintPure, Category="Downfall|Vehicle") bool IsOccupied() const { return Driver != nullptr; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(Replicated) TObjectPtr<ADownfallCharacter> Driver;
    UPROPERTY(Replicated) float SpeedCmPerSecond = 0.f;
    // APawn has no prediction component here. Explicit state also reaches its autonomous owner.
    UPROPERTY(ReplicatedUsing=OnRep_ServerPose) FTransform ServerPose;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> DrivingContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ThrottleAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> SteeringAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ExitAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> BrakeAction;

    float LocalThrottle = 0.f;
    float LocalSteering = 0.f;
    bool bLocalBrake = false;
    float ServerThrottle = 0.f;
    float ServerSteering = 0.f;
    bool bServerBrake = true;
    float InputSendElapsed = 0.f;
    double LastServerInputTime = -1.0;
    double LastAcceptedInputTime = -1.0;
    double LastExitRequestTime = -1.0;
    bool bInputContextInstalled = false;
    TWeakObjectPtr<APlayerController> InputController;

    void BuildInputActions();
    void UpdateInputContext();
    void RemoveInputContext();
    void InputThrottle(const FInputActionValue& Value);
    void InputSteering(const FInputActionValue& Value);
    void InputBrake(const FInputActionValue& Value);
    void InputExit();
    void SimulateAuthority(float DeltaSeconds);
    bool FindExitLocation(FVector& OutLocation) const;
    UFUNCTION() void OnRep_ServerPose();

    UFUNCTION(Server, Unreliable) void ServerDrive(float Throttle, float Steering, bool bBrake);
    UFUNCTION(Server, Reliable) void ServerExit();
};
