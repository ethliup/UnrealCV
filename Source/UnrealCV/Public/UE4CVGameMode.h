#pragma once

#include "UnrealCVPrivate.h"
#include "GameFramework/GameMode.h"
#include "UE4CVCommandHandlers.h"
#include "UE4CVPawn.h"
#include "CommandDispatcher.h"
#include "ConsoleHelper.h"
#include "UE4CVServer.h"
#include "UE4CVGameMode.generated.h"

/**
 *
 */
UCLASS()
class UNREALCV_API AUE4CVGameMode : public AGameMode
{
	GENERATED_BODY()
	AUE4CVGameMode(const class FObjectInitializer& ObjectInitializer);
	~AUE4CVGameMode();

public:
	virtual void StartPlay() override;

private:
	FUE4CVCommandHandlers* mCustomCommandHandler;
	FConsoleHelper* mConsoleHelper;
	FCommandDispatcher* mCommandDispatcher;
	FUE4CVServer* mUE4CVTcpServer;
};
