#pragma once

#include "UnrealCVPrivate.h"
#include "ExecStatus.h"
#include "CommandDispatcher.h"
#include "UE4CVPawn.h"
#include "GTCaptureComponent.h"

class FUE4CVCommandHandlers
{
public:
	FUE4CVCommandHandlers(AUE4CVPawn* pawn, FCommandDispatcher* InCommandDispatcher)
		: mCommandDispatcher(InCommandDispatcher)
		, mPawnActor(pawn){}

	void RegisterCommands();

	/** Get camera image with a given mode */
	FExecStatus CaptureCurrentCameraView(const TArray<FString>& Args);

	FExecStatus GetCurrentPose(const TArray<FString>& Args);

	FExecStatus SetCurrentPose(const TArray<FString>& Args);

	FExecStatus GetCameraIntrinsics(const TArray<FString>& Args);

	FExecStatus GetCameraExtrinsics(const TArray<FString>& Args);

	FExecStatus SetCameraFoV(const TArray<FString>& Args);
	FExecStatus GetCameraFoV(const TArray<FString>& Args);

	FExecStatus SetCameraResolution(const TArray<FString>& Args);
	FExecStatus GetCameraResolution(const TArray<FString>& Args);

private:
	void ComputeCameraIntrinsic(UGTCaptureComponent* camera, float& fx, float& fy, float& cx, float& cy);

private:
	FCommandDispatcher* mCommandDispatcher;
	AUE4CVPawn* mPawnActor;
};
