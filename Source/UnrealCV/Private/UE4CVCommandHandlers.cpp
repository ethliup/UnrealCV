#include "UE4CVCommandHandlers.h"
#include "UnrealCVPrivate.h"
#include "ExecStatus.h"
#include "UE4CVHelpers.h"
#include <fstream>
#include <string>
#include <ctime>

FString GenerateSeqFilename()
{
	static uint32 NumCaptured = 0;
	NumCaptured++;
	FString Filename = FString::Printf(TEXT("%08d.png"), NumCaptured);
	return Filename;
}

void FUE4CVCommandHandlers::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::CaptureCurrentCameraView);
	mCommandDispatcher->BindCommand("vget /camera/[uint]/[str] [uint]", Cmd, "Get snapshot from camera, the third parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::CaptureCurrentCameraView);
	mCommandDispatcher->BindCommand("vget /camera/[uint]/[str] [uint] [str]", Cmd, "Get snapshot from camera, the third parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCurrentPose);
	mCommandDispatcher->BindCommand("vget /pose", Cmd, "Get current actor pose, from reference camera to global world frame"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCurrentPose);
	mCommandDispatcher->BindCommand("vget /pose [str]", Cmd, "Get current actor pose and append to file, from body frame to global world frame"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::SetCurrentPose);
	mCommandDispatcher->BindCommand("vset /pose [str] [str] [str] [str] [str] [str] [str]", Cmd, "Set pose, [qw qx qy qz x y z] from body to world");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCameraIntrinsics);
	mCommandDispatcher->BindCommand("vget /camera/K [str]", Cmd, "Get camera intrinsic parameters and save them into file [str]"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCameraExtrinsics);
	mCommandDispatcher->BindCommand("vget /camera/extrin [str]", Cmd, "Get camera extrinsic parameters and save them into file [str], from camera to body frame");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::SetCameraFoV);
	mCommandDispatcher->BindCommand("vset /fov [float]", Cmd, "Set camrea FoV to [float]");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCameraFoV);
	mCommandDispatcher->BindCommand("vget /fov", Cmd, "Get camrea FoV");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::SetCameraResolution);
	mCommandDispatcher->BindCommand("vset /resolution [uint] [uint]", Cmd, "Set camrea resolution to (H, W)");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FUE4CVCommandHandlers::GetCameraResolution);
	mCommandDispatcher->BindCommand("vget /resolution", Cmd, "Get camrea resolution");
}

FExecStatus FUE4CVCommandHandlers::CaptureCurrentCameraView(const TArray<FString>& Args)
{
	if (Args.Num() <= 4) // The first is camera id, the second is ViewMode, the third is the row to capture
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		FString ViewMode = Args[1];
		int32 RowToCapture = FCString::Atoi(*Args[2]);

		FString Filename;
		if (Args.Num() == 4)
		{
			Filename = Args[3];
		}
		else
		{
			Filename = GenerateSeqFilename();
		}

		UGTCaptureComponent* GTCapturer = mPawnActor->GetCamera(CameraId);
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		FAsyncRecord* AsyncRecord = GTCapturer->Capture(*ViewMode, *Filename, RowToCapture); // Due to sandbox implementation of UE4, it is not possible to specify an absolute path directly.
		
		if (AsyncRecord == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Unrecognized capture mode %s"), *ViewMode));
		}

		// TODO: Check IsPending is problematic.
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([Filename, AsyncRecord]()
		{
			if (AsyncRecord->bIsCompleted)
			{
				FString msg = AsyncRecord->msg;
				AsyncRecord->Destory();
				return FExecStatus::OK(*msg);
			}
			else
			{
				return FExecStatus::Pending();
			}
		});
		FString Message = FString::Printf(TEXT("File will be saved to %s"), *Filename);
		return FExecStatus::AsyncQuery(FPromise(PromiseDelegate), Message); 
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FUE4CVCommandHandlers::GetCurrentPose(const TArray<FString>& Args)
{
	FVector trans = mPawnActor->GetActorLocation();
	FRotator rots = mPawnActor->GetController()->GetControlRotation();
	FTransform pose_lhc(rots, trans);
	
	FTransform pose_rhc = T_lhc2rhc(pose_lhc);

	FQuat R = pose_rhc.GetRotation();
	FVector t = pose_rhc.GetTranslation();
	FString pose = FString::Printf(TEXT("%f, %f, %f, %f, %f, %f, %f"), R.W, R.X, R.Y, R.Z, t.X, t.Y, t.Z);

	if (Args.Num() == 1)
	{
		// append to file
		FString fileName = Args[0];
		std::ofstream fileStream(*fileName, std::ios_base::app);

		if (!fileStream.is_open())
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid file name %s"), *fileName));
		}
		std::wstring ws(*pose);
		std::string str_pose(ws.begin(), ws.end());
		fileStream << str_pose << "\n";
	}
	return FExecStatus::OK(pose);
}

FExecStatus FUE4CVCommandHandlers::SetCurrentPose(const TArray<FString>& Args)
{
	// from body to global world frame
	if (Args.Num() == 7)
	{
		float qw = FCString::Atof(*Args[0]);
		float qx = FCString::Atof(*Args[1]);
		float qy = FCString::Atof(*Args[2]);
		float qz = FCString::Atof(*Args[3]);
		float x = FCString::Atof(*Args[4]);
		float y = FCString::Atof(*Args[5]);
		float z = FCString::Atof(*Args[6]);

		FTransform pose_rhc(FQuat(qx, qy, qz, qw), FVector(x, y, z));
		FTransform pose_lhc = T_rhc2lhc(pose_rhc);

		mPawnActor->SetActorLocation(pose_lhc.GetTranslation());
		mPawnActor->GetController()->SetControlRotation(pose_lhc.Rotator());
		mPawnActor->SetActorRotation(pose_lhc.Rotator());
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FUE4CVCommandHandlers::GetCameraIntrinsics(const TArray<FString>& Args)
{
	if(Args.Num() != 1) return FExecStatus::InvalidArgument;

	FString fileName = Args[0];

	std::ofstream fileStream(*fileName);

	if (!fileStream.is_open())
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid file name %s"), *fileName));
	}

	std::wstring ws(FApp::GetGameName());
	std::string  str_name(ws.begin(), ws.end());
	fileStream << "# Camera intrinsic parameters for unreal dataset " << str_name << "\n";

	time_t t = std::time(0);   // get time now
	struct tm * now = localtime(&t);
	fileStream <<"# date: " << now->tm_year + 1900 << "-" << now->tm_mon + 1 << '-' << now->tm_mday << "-" << now->tm_hour << "-" << now->tm_min << "-" << now->tm_sec << "\n\n";

	size_t numCameras = mPawnActor->GetNumCameras();

	fileStream << "numCameras " << numCameras << "\n\n";

	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);

		if (camera == nullptr) continue;

		float fx, fy, cx, cy;

		ComputeCameraIntrinsic(camera, fx, fy, cx, cy);

		int res_rows, res_cols;

		camera->GetCameraResolution(res_rows, res_cols);

		fileStream << "devName cam" << i << "\n";

		fileStream << "# resolution: nCols, nRows\n";
		fileStream << "resolution " << res_cols << " " << res_rows << "\n";

		fileStream << "# K: fx fy cx cy with pinhole camera model\n";
		fileStream << "K " << fx << " " << fy << " " << cx << " " << cy << "\n\n";
	}

	fileStream.close();

	return FExecStatus::OK();
}

FExecStatus FUE4CVCommandHandlers::GetCameraExtrinsics(const TArray<FString>& Args)
{
	if (Args.Num() != 1) return FExecStatus::InvalidArgument;

	FString fileName = Args[0];

	std::ofstream fileStream(*fileName);

	if (!fileStream.is_open())
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid file name %s"), *fileName));
	}

	std::wstring ws(FApp::GetGameName());
	std::string  str_name(ws.begin(), ws.end());
	fileStream << "# Camera extrinsic parameters for unreal dataset " << str_name << "\n";

	time_t t = std::time(0);   // get time now
	struct tm * now = localtime(&t);
	fileStream << "# date: " << now->tm_year + 1900 << "-" << now->tm_mon + 1 << '-' << now->tm_mday << "-" << now->tm_hour << "-" << now->tm_min << "-" << now->tm_sec << "\n\n";

	size_t numCameras = mPawnActor->GetNumCameras();

	fileStream << "numCameras " << numCameras << "\n\n";

	FMatrix _T_camFrame2BodyFrame(FVector(0, -1, 0), FVector(0, 0, -1), FVector(1, 0, 0), FVector(0, 0, 0));
	FTransform T_camFrame2BodyFrame(_T_camFrame2BodyFrame);

	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);

		if (camera == nullptr) continue;

		FTransform Tcam_2_body = camera->GetRelativeTransform();
		Tcam_2_body = T_lhc2rhc(Tcam_2_body);
		Tcam_2_body = T_camFrame2BodyFrame * Tcam_2_body;
		
		float qw = Tcam_2_body.Rotator().Quaternion().W;
		float qx = Tcam_2_body.Rotator().Quaternion().X;
		float qy = Tcam_2_body.Rotator().Quaternion().Y;
		float qz = Tcam_2_body.Rotator().Quaternion().Z;

		float x = Tcam_2_body.GetTranslation().X;
		float y = Tcam_2_body.GetTranslation().Y;
		float z = Tcam_2_body.GetTranslation().Z;

		fileStream << "devName cam" << i << "\n";

		fileStream << "# translation scaling factor to meter\n";
		fileStream << "scalingFactor 0.01" << "\n";

		fileStream << "# camera extrinsics: [qw qx qy qz x y z (cm)] from camera frame to vehicle body frame\n";
		fileStream << "T " << qw << " " << qx << " " << qy << " " << qz << " " << x << " " << y << " " << z << "\n\n";
	}

	fileStream.close();

	return FExecStatus::OK();
}

void FUE4CVCommandHandlers::ComputeCameraIntrinsic(UGTCaptureComponent* camera, float& fx, float& fy, float& cx, float& cy)
{
	int heightInPixels, widthInPixels;
	camera->GetCameraResolution(heightInPixels, widthInPixels);

	float fov = camera->GetCamearFoV() / 2.0f; // half of FoV in degrees, horizontal FoV by default, different from Unity!!!
	fov = PI * fov / 180.0f;

	double aspectRatio = static_cast<double>(widthInPixels) / static_cast<double>(heightInPixels);

	double widthInMeters = 2.0f*tan(fov);
	double heightInMeters = widthInMeters / aspectRatio;

	fx = widthInPixels / widthInMeters;
	fy = heightInPixels / heightInMeters;

	cx = widthInPixels / 2.0f;
	cy = heightInPixels / 2.0f;
}

FExecStatus FUE4CVCommandHandlers::SetCameraFoV(const TArray<FString>& Args)
{
	float fov = FCString::Atof(*Args[0]);
	size_t numCameras = mPawnActor->GetNumCameras();
	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);
		camera->SetCameraFoV(fov);

		FString msg = FString::Printf(TEXT("Set FoV %f to Camera %d"), fov, i);
		return FExecStatus::OK(msg);
	}
	return FExecStatus::OK();
}

FExecStatus FUE4CVCommandHandlers::GetCameraFoV(const TArray<FString>& Args)
{
	size_t numCameras = mPawnActor->GetNumCameras();
	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);
		float fov = camera->GetCamearFoV();

		FString msg = FString::Printf(TEXT("FoV of Camera %d: %f"), i, fov);
		return FExecStatus::OK(msg);
	}
	return FExecStatus::OK();
}

FExecStatus FUE4CVCommandHandlers::SetCameraResolution(const TArray<FString>& Args)
{
	int H = FCString::Atoi(*Args[0]);
	int W = FCString::Atoi(*Args[1]);
	size_t numCameras = mPawnActor->GetNumCameras();
	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);
		camera->SetCameraResolution(H, W);

		FString msg = FString::Printf(TEXT("Set resolution [H=%d, W=%d] to Camera %d"), H, W, i);
		return FExecStatus::OK(msg);
	}
	return FExecStatus::OK();
}

FExecStatus FUE4CVCommandHandlers::GetCameraResolution(const TArray<FString>& Args)
{
	size_t numCameras = mPawnActor->GetNumCameras();
	for (size_t i = 0; i < numCameras; ++i)
	{
		UGTCaptureComponent* camera = mPawnActor->GetCamera(i);
		int32 H, W;
		camera->GetCameraResolution(H, W);
		FString msg = FString::Printf(TEXT("Get resolution [H=%d, W=%d] of Camera %d"), H, W, i);
		return FExecStatus::OK(msg);
	}
	return FExecStatus::OK();
}
