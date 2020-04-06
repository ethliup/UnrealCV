#include "UE4CVPawn.h"

/**
 * UE4CVPawn can move freely in the 3D space
 */
AUE4CVPawn::AUE4CVPawn(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// turn off gravity
	GetCharacterMovement()->GravityScale = 0; 
	GetCharacterMovement()->BrakingDecelerationFalling = 2048;
	GetCharacterMovement()->BrakingDecelerationFlying = 2048;
	GetCharacterMovement()->BrakingDecelerationSwimming = 2048;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048;

 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	BaseEyeHeight = 0;
	SetActorLocation(GetActorLocation() + FVector(0, 0, 50));

	UGTCaptureComponent* Camera_0 = ObjectInitializer.CreateDefaultSubobject<UGTCaptureComponent>(this, TEXT("Camera_0"));
	Camera_0->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	Camera_0->SetRelativeLocation(FVector(0, 0, BaseEyeHeight));
	mCaptureComponents.push_back(Camera_0);

	UGTCaptureComponent* Camera_1 = ObjectInitializer.CreateDefaultSubobject<UGTCaptureComponent>(this, TEXT("Camera_1"));
	Camera_1->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	Camera_1->SetRelativeLocation(FVector(0, 200, BaseEyeHeight));
	mCaptureComponents.push_back(Camera_1);
}

AUE4CVPawn::~AUE4CVPawn()
{
}

void AUE4CVPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AUE4CVPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AUE4CVPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
}

void AUE4CVPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);
	InputComponent->BindAxis("Yaw", this, &AUE4CVPawn::AddControllerYawInput);
	InputComponent->BindAxis("Pitch", this, &AUE4CVPawn::AddControllerPitchInput);
	InputComponent->BindAxis("MoveForward", this, &AUE4CVPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AUE4CVPawn::MoveRight);
	InputComponent->BindAxis("MoveUpward", this, &AUE4CVPawn::MoveUpward);
}

void AUE4CVPawn::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		FRotator Rotation = Controller->GetControlRotation();
		// Limit pitch when walking or falling
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			Rotation.Pitch = 0.0f;
		}
		// add movement in that direction
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AUE4CVPawn::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);

		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AUE4CVPawn::MoveUpward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		FVector Direction = GetActorForwardVector();
		FVector Motion = FVector(0, 0, 10.f * Value);
		FVector NewLocation = GetActorLocation() + Motion;

		SetActorLocation(NewLocation);
	}
}

void AUE4CVPawn::AddControllerYawInput(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		Super::AddControllerYawInput(Value);
		FRotator rots = this->GetController()->GetControlRotation();
		this->SetActorRotation(rots);
	}
}

void AUE4CVPawn::AddControllerPitchInput(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		Super::AddControllerPitchInput(Value);
		FRotator rots = this->GetController()->GetControlRotation();
		this->SetActorRotation(rots);
	}
}

UGTCaptureComponent* AUE4CVPawn::GetCamera(int index)
{
	if (!(index < mCaptureComponents.size())) return nullptr;
	return mCaptureComponents.at(index);
}