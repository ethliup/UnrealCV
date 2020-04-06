#include "UE4CVGameMode.h"

AUE4CVGameMode::AUE4CVGameMode(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// create pawn character
	DefaultPawnClass = AUE4CVPawn::StaticClass();
}

void AUE4CVGameMode::StartPlay()
{
	Super::StartPlay();

	mCommandDispatcher = new FCommandDispatcher();
	mConsoleHelper = new FConsoleHelper();

	AUE4CVPawn* character = static_cast<AUE4CVPawn*>(GetWorld()->GetPlayerControllerIterator()->Get()->GetCharacter());
	mCustomCommandHandler = new FUE4CVCommandHandlers(character, mCommandDispatcher);
	mCustomCommandHandler->RegisterCommands();

	mConsoleHelper->SetCommandDispatcher(mCommandDispatcher);

	mUE4CVTcpServer = new FUE4CVServer(9000, mCommandDispatcher);
}

AUE4CVGameMode::~AUE4CVGameMode()
{
	delete mCustomCommandHandler;
	delete mConsoleHelper;
	delete mCommandDispatcher;
	delete mUE4CVTcpServer;
}
