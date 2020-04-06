#pragma once

#include "UnrealCVPrivate.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GTCaptureComponent.h"
#include <vector>
#include "UE4CVHelpers.h"
#include "UE4CVPawn.generated.h"

/**
 * UE4CVPawn can move freely in the 3D space
 */
UCLASS()
class UNREALCV_API AUE4CVPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AUE4CVPawn(const class FObjectInitializer& ObjectInitializer);
	~AUE4CVPawn();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick( float DeltaSeconds ) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	//handles moving forward/backward
	UFUNCTION()
		void MoveForward(float Val);
	//handles strafing
	UFUNCTION()
		void MoveRight(float Val);

	UFUNCTION()
		void MoveUpward(float Val);

	//UFUNCTION()
		void AddControllerYawInput(float Val);

	//UFUNCTION()
		void AddControllerPitchInput(float Val);

public:
	UGTCaptureComponent* GetCamera(int index);
	size_t GetNumCameras() { return mCaptureComponents.size(); }

private:
	std::vector<UGTCaptureComponent*> mCaptureComponents;
};

