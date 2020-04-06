#pragma once

#include "UnrealCVPrivate.h"
#include "UE4CVHelpers.h"
#include "GTCaptureComponent.generated.h"

struct FGTCaptureTask
{
	FString Mode;
	FString Filename;
	FAsyncRecord* AsyncRecord;
	uint64 CurrentFrame;
	int32 RowToCapture;
	FGTCaptureTask() {}
	FGTCaptureTask(FString InMode, FString InFilename, uint64 InCurrentFrame, int32 InRowToCapture, FAsyncRecord* InAsyncRecord) :
		Mode(InMode), Filename(InFilename), AsyncRecord(InAsyncRecord), CurrentFrame(InCurrentFrame), RowToCapture(InRowToCapture) {}
};

/**
 * Use USceneCaptureComponent2D to export information from the scene.
 * This class needs to be tickable to update the rotation of the USceneCaptureComponent2D 
 */
UCLASS()
class UNREALCV_API UGTCaptureComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UGTCaptureComponent(const class FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
public:
	void  CreateCaptureComponents(const class FObjectInitializer& ObjectInitializer);
	void  SetCameraFoV(float fov);
	float GetCamearFoV();
	void  SetCameraResolution(int32 height, int width);
	void  GetCameraResolution(int32& height, int32& width);
	FAsyncRecord* Capture(FString Mode, FString Filename, int32 InRowToCapture);

public:
	USceneCaptureComponent2D* GetSceneCaptureComponent2D(FString Mode);

private:
	void CreateMaterials();
	UMaterial* GetMaterial(FString ModeName);

	/**
	get engine render flags for different veiw modes
	*/
	void GetBasicEngineShowFlags(FEngineShowFlags& ShowFlags);
	void GetLitEngineShowFlags(FEngineShowFlags& ShowFlags);
	void GetPostProcessEngineShowFlags(FEngineShowFlags& ShowFlags);
	void GetVertexColorEngineShowFlags(FEngineShowFlags& ShowFlags);
	void GetWireFrameEngineShowFlags(FEngineShowFlags& ShowFlags);

private:
	void SaveExr(UTextureRenderTarget2D* RenderTarget, FString Filename);
	void SaveTXTDepthFile(UTextureRenderTarget2D* RenderTarget, FString Filename, int RowToSave);
	void SavePng(UTextureRenderTarget2D* RenderTarget, FString Filename, int RowToSave);

private:
	TMap<FString, FString> mMaterialPathMap;
	TArray<FString> mSupportedViewModes;
	TQueue<FGTCaptureTask, EQueueMode::Spsc> mPendingTasks;	
	TMap<FString, USceneCaptureComponent2D*> mViewCaptureComponents;
	TMap<FString, UMaterial*> mMaterialMap;
};