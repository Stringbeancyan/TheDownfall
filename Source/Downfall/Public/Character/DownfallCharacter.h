#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DownfallCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UDownfallHealthComponent;
class UDownfallCombatComponent;

/** Networked third-person prototype. Primitive visuals intentionally expose missing character art. */
UCLASS()
class DOWNFALL_API ADownfallCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	explicit ADownfallCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_Controller() override;
	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintPure, Category = "Downfall|Character")
	UDownfallHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Downfall|Character")
	UDownfallCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Downfall|Character")
	bool IsSeated() const { return bSeated; }

	/** Vehicle authority controls this state. Call before possessing or after exiting a vehicle. */
	void SetSeated(bool bNewSeated);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Downfall|Character")
	TObjectPtr<UDownfallHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Downfall|Character")
	TObjectPtr<UDownfallCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Downfall|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Downfall|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	void BuildNativeInputMapping();
	void RemoveNativeInputMapping();
	bool CanAct() const;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void StartSprint();
	void StopSprint();
	void ToggleCrouch();
	void StartFire();
	void StopFire();
	void Reload();
	void NextWeapon();
	void Interact();
	void SwitchShoulder();
	void StartCinematic();
	void SkipCinematic();

	/** No target or position accepted from the client; the server finds and validates the target. */
	UFUNCTION(Server, Reliable)
	void ServerInteract();

	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void OnRep_Seated();

	UPROPERTY(ReplicatedUsing = OnRep_Seated)
	bool bSeated = false;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PrototypeBody;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PrototypeHead;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PrototypeWeaponBody;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PrototypeWeaponBarrel;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> NativeMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputLocalPlayerSubsystem> BoundInputSubsystem;

	// The context and these references retain the native action objects through garbage collection.
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> FireAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> NextWeaponAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ShoulderAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CinematicAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SkipCinematicAction;

	double LastServerInteractionTime = -100.0;
	bool bDeathHandled = false;
};
