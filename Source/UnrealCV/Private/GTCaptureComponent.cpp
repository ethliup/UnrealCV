#include "GTCaptureComponent.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include <fstream>
#include <iostream>
#include <future>
#include <thread>

UGTCaptureComponent::UGTCaptureComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true; //need this for triggering TickComponenet() function

	mSupportedViewModes.Add(TEXT("lit"));
	mSupportedViewModes.Add(TEXT("depth"));
	mSupportedViewModes.Add(TEXT("debug"));  // TODO: current not working, should modify the material file
	mSupportedViewModes.Add(TEXT("object_mask")); // TODO: current not working, should configure it properly
	mSupportedViewModes.Add(TEXT("normal"));
	mSupportedViewModes.Add(TEXT("wireframe"));

	CreateMaterials();

	CreateCaptureComponents(ObjectInitializer);
}

void UGTCaptureComponent::CreateMaterials()
{
	mMaterialPathMap.Add(TEXT("depth"), TEXT("Material'/UnrealCV/SceneDepthWorldUnits.SceneDepthWorldUnits'"));
	mMaterialPathMap.Add(TEXT("vis_depth"), TEXT("Material'/UnrealCV/SceneDepth.SceneDepth'"));
	mMaterialPathMap.Add(TEXT("debug"), TEXT("Material'/UnrealCV/debug.debug'"));
	mMaterialPathMap.Add(TEXT("normal"), TEXT("Material'/UnrealCV/WorldNormal.WorldNormal'"));

	for (auto& Elem : mMaterialPathMap)
	{
		FString ModeName = Elem.Key;
		FString MaterialPath = Elem.Value;

		ConstructorHelpers::FObjectFinder<UMaterial> Material(*MaterialPath);
		if (Material.Object != NULL)
		{
			mMaterialMap.Add(ModeName, (UMaterial*)Material.Object);
		}
	}
}

void UGTCaptureComponent::CreateCaptureComponents(const class FObjectInitializer& ObjectInitializer)
{
	// create sub-view mode for this camera
	for (FString Mode : mSupportedViewModes)
	{
		USceneCaptureComponent2D* CaptureComponent =  ObjectInitializer.CreateDefaultSubobject<USceneCaptureComponent2D>(this, *(GetName() + "_" + Mode));
		CaptureComponent->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
		CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>();
		CaptureComponent->TextureTarget->InitAutoFormat(256, 256); // TODO: Update this later
		

		if (Mode == "lit") // For rendered imagess
		{
			GetLitEngineShowFlags(CaptureComponent->ShowFlags);
			
			if (GEngine == nullptr)
			{
				CaptureComponent->TextureTarget->TargetGamma = 1.0;
			}
			else
			{
				CaptureComponent->TextureTarget->TargetGamma = GEngine->GetDisplayGamma();
			}
		}
		else if (Mode == "depth" || Mode == "normal" || Mode == "vis_depth" || Mode == "debug")
		{
			CaptureComponent->TextureTarget->TargetGamma = 1;
			UMaterial* Material = GetMaterial(Mode);
			if (Material == NULL) continue;
			GetPostProcessEngineShowFlags(CaptureComponent->ShowFlags);
			CaptureComponent->PostProcessSettings.AddBlendable(Material, 1);
		}
		else if (Mode == "object_mask")
		{
			GetVertexColorEngineShowFlags(CaptureComponent->ShowFlags);
		}
		else if (Mode == "wireframe")
		{
			CaptureComponent->TextureTarget->TargetGamma = 1;
			GetWireFrameEngineShowFlags(CaptureComponent->ShowFlags);
		}
		else // default
		{
			GetLitEngineShowFlags(CaptureComponent->ShowFlags);
			CaptureComponent->TextureTarget->TargetGamma = GEngine->GetDisplayGamma();
		}
		mViewCaptureComponents.Add(Mode, CaptureComponent);
	}
}

UMaterial* UGTCaptureComponent::GetMaterial(FString InModeName = TEXT(""))
{
	UMaterial* Material = mMaterialMap.FindRef(InModeName);
	if (Material == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can not recognize visualization mode %s"), *InModeName);
	}
	return Material;
}

USceneCaptureComponent2D* UGTCaptureComponent::GetSceneCaptureComponent2D(FString Mode)
{
	return this->mViewCaptureComponents.FindRef(Mode);
}

void UGTCaptureComponent::GetBasicEngineShowFlags(FEngineShowFlags& ShowFlags)
{
	ShowFlags = FEngineShowFlags(EShowFlagInitMode::ESFIM_All0);
	ShowFlags.SetRendering(true);
	ShowFlags.SetStaticMeshes(true);
}

void UGTCaptureComponent::GetLitEngineShowFlags(FEngineShowFlags& ShowFlags)
{
	GetBasicEngineShowFlags(ShowFlags);
	ShowFlags = FEngineShowFlags(EShowFlagInitMode::ESFIM_Game);
	ApplyViewMode(VMI_Lit, true, ShowFlags);
	ShowFlags.SetMaterials(true);
	ShowFlags.SetLighting(true);
	ShowFlags.SetPostProcessing(true);
	// ToneMapper needs to be enabled, otherwise the screen will be very dark
	ShowFlags.SetTonemapper(true);
	// TemporalAA needs to be disabled, otherwise the previous frame might contaminate current frame.
	// Check: https://answers.unrealengine.com/questions/436060/low-quality-screenshot-after-setting-the-actor-pos.html for detail

	// ShowFlags.SetTemporalAA(true);
	ShowFlags.SetTemporalAA(false);
	ShowFlags.SetAntiAliasing(true);
	ShowFlags.SetEyeAdaptation(false); // Eye adaption is a slow temporal procedure, not useful for image capture
}

void UGTCaptureComponent::GetPostProcessEngineShowFlags(FEngineShowFlags& ShowFlags)
{
	GetBasicEngineShowFlags(ShowFlags);
	// These are minimal setting
	ShowFlags.SetPostProcessing(true);
	ShowFlags.SetPostProcessMaterial(true);
	ShowFlags.SetInstancedStaticMeshes(true);
	ShowFlags.SetInstancedGrass(true);
	// ShowFlags.SetVertexColors(true); // This option will change object material to vertex color material, which don't produce surface normal

	GVertexColorViewMode = EVertexColorViewMode::Color;
}

void UGTCaptureComponent::GetVertexColorEngineShowFlags(FEngineShowFlags& ShowFlags)
{
	ApplyViewMode(VMI_Lit, true, ShowFlags);

	// From MeshPaintEdMode.cpp:2942
	ShowFlags.SetMaterials(false);
	ShowFlags.SetLighting(false);
	ShowFlags.SetBSPTriangles(true);
	ShowFlags.SetVertexColors(true);
	ShowFlags.SetPostProcessing(false);
	ShowFlags.SetHMDDistortion(false);
	ShowFlags.SetTonemapper(false); // This won't take effect here

	GVertexColorViewMode = EVertexColorViewMode::Color;
}

void UGTCaptureComponent::GetWireFrameEngineShowFlags(FEngineShowFlags& ShowFlags)
{
	ShowFlags.SetPostProcessing(true);
	ShowFlags.SetWireframe(true);
}

void UGTCaptureComponent::SaveExr(UTextureRenderTarget2D* RenderTarget, FString Filename)
{
	int32 Width = RenderTarget->SizeX, Height = RenderTarget->SizeY;
	TArray<FFloat16Color> FloatImage;
	FloatImage.AddZeroed(Width * Height);
	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	RenderTargetResource->ReadFloat16Pixels(FloatImage);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);

	ImageWrapper->SetRaw(FloatImage.GetData(), FloatImage.GetAllocatedSize(), Width, Height, ERGBFormat::RGBA, 16);
	const TArray<uint8>& PngData = ImageWrapper->GetCompressed(100);
	{
		FFileHelper::SaveArrayToFile(PngData, *Filename);
	}
}

void UGTCaptureComponent::SaveTXTDepthFile(UTextureRenderTarget2D* RenderTarget, FString Filename, int RowToSave)
{
	int32 Width = RenderTarget->SizeX, Height = RenderTarget->SizeY;
	TArray<FFloat16Color> FloatImage;
	FloatImage.AddZeroed(Width * Height);
	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	RenderTargetResource->ReadFloat16Pixels(FloatImage);

	// check whether we have the directory
	int32 count = -1;
	Filename.FindLastChar(TCHAR('/'), count);
	if (count < 0)
	{
		return;
	}
	FString directory = Filename.Left(count);

	// create directory if it does not exist
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*directory);

	// create txt file
	std::ofstream fileWriter;
	fileWriter.open(TCHAR_TO_ANSI(*Filename));
	if (!fileWriter.is_open())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open file for depth data"));
		return;
	}

	for (int32 row = 0; row < Height; row++)
	{
		if (RowToSave < 9999 && RowToSave != row) continue;

		for (int32 col = 0; col < Width; col++)
		{
			int32 index = row*Width + col;
			FFloat16Color RGBA = FloatImage[index];
			float depth = RGBA.R.GetFloat();
			fileWriter << depth << " ";
		}
		fileWriter << "\n";
	}
	fileWriter.close();
}

void UGTCaptureComponent::SavePng(UTextureRenderTarget2D* RenderTarget, FString Filename, int RowToSave)
{
	static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	static IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	{
		int32 Width = RenderTarget->SizeX, Height = RenderTarget->SizeY;
		TArray<FColor> Image;
		FTextureRenderTargetResource* RenderTargetResource;
		Image.AddZeroed(Width * Height);
		{
			RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		}
		{
			FReadSurfaceDataFlags ReadSurfaceDataFlags;
			ReadSurfaceDataFlags.SetLinearToGamma(false); // This is super important to disable this!
														  // Instead of using this flag, we will set the gamma to the correct value directly
			RenderTargetResource->ReadPixels(Image, ReadSurfaceDataFlags);
		}
		{
			if (RowToSave > 9999)
			{
				ImageWrapper->SetRaw(Image.GetData(), Image.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
			}
			else // save the specified row only
			{
				ImageWrapper->SetRaw(Image.GetData()+RowToSave*Width, Width*sizeof(FColor), Width, 1, ERGBFormat::BGRA, 8);
			}
		}
		const TArray<uint8>& ImgData = ImageWrapper->GetCompressed(100);
		{
			if (!FFileHelper::SaveArrayToFile(ImgData, *Filename))
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to save image to %s"), *Filename);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Success to save image to %s"), *Filename);
			}
		}
	}
}

void UGTCaptureComponent::SetCameraFoV(float fov)
{
	for (auto& Elem : mViewCaptureComponents)
	{
		Elem.Value->FOVAngle = fov;
	}
}

float UGTCaptureComponent::GetCamearFoV()
{
	for (auto& Elem : mViewCaptureComponents)
	{
		return Elem.Value->FOVAngle;
	}
	return 0.0f;
}

void UGTCaptureComponent::SetCameraResolution(int32 height, int32 width)
{
	for (auto& Elem : mViewCaptureComponents)
	{
		USceneCaptureComponent2D* tmp = Elem.Value;
		tmp->TextureTarget->InitAutoFormat(width, height);
	}
}

void UGTCaptureComponent::GetCameraResolution(int32& height, int32& width)
{
	for (auto& Elem : mViewCaptureComponents)
	{
		USceneCaptureComponent2D* tmp = Elem.Value;
		width = tmp->TextureTarget->SizeX;
		height = tmp->TextureTarget->SizeY;
		return;
	}
	width = 0;
	height = 0;
}

FAsyncRecord* UGTCaptureComponent::Capture(FString Mode, FString InFilename, int32 InRowToCapture)
{
	FAsyncRecord* AsyncRecord = FAsyncRecord::Create();
	FGTCaptureTask GTCaptureTask = FGTCaptureTask(Mode, InFilename, GFrameCounter, InRowToCapture, AsyncRecord);
	mPendingTasks.Enqueue(GTCaptureTask);
	return AsyncRecord;
}

void UGTCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	while (!mPendingTasks.IsEmpty())
	{
		FGTCaptureTask Task;
		mPendingTasks.Peek(Task);
		uint64 CurrentFrame = GFrameCounter;

		if (!(CurrentFrame >= Task.CurrentFrame + 1))
		{ 
			break;
		}

		mPendingTasks.Dequeue(Task);
		
		USceneCaptureComponent2D* CaptureComponent = this->mViewCaptureComponents.FindRef(Task.Mode);
		if (CaptureComponent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Unrecognized capture mode %s"), *Task.Mode);
			Task.AsyncRecord->msg = "Unsupported capture mode";
			Task.AsyncRecord->bIsCompleted = true;
		}
		else
		{
			FString LowerCaseFilename = Task.Filename.ToLower();
			if (LowerCaseFilename.EndsWith("png"))
			{
				SavePng(CaptureComponent->TextureTarget, Task.Filename, Task.RowToCapture);
			}
			else if (LowerCaseFilename.EndsWith("exr"))
			{
				//SaveExr(CaptureComponent->TextureTarget, Task.Filename);
				SaveTXTDepthFile(CaptureComponent->TextureTarget, Task.Filename, Task.RowToCapture);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Unrecognized image file extension %s"), *LowerCaseFilename);
			}
			Task.AsyncRecord->msg = "Capture is successfully stored to: ";
			Task.AsyncRecord->msg.Append(LowerCaseFilename);
			Task.AsyncRecord->bIsCompleted = true;
		}
	}
}



