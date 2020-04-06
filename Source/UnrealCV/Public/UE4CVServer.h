// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealCVPrivate.h"
#include "CommandDispatcher.h"
#include "Networking.h"
#include "NetworkMessage.h"
#include "UE4CVHelpers.h"

#define MAX_BUFFERSIZE4RECEIVE 2048

class FRequest
{
public:
	FRequest() {}
	FRequest(FString InMessage, uint32 InRequestId) : Message(InMessage), RequestId(InRequestId) {}
	FString Message;
	uint32 RequestId;
};

class FSocketMessageHeader
{
public:
	/** Error checking */
	uint32 Magic = 0;

	/** Payload Size */
	uint32 PayloadSize = 0;

	static uint32 DefaultMagic;

public:
	FSocketMessageHeader(const TArray<uint8>& Payload)
	{
		PayloadSize = Payload.Num();  // What if PayloadSize is 0
		Magic = FSocketMessageHeader::DefaultMagic;
	}

	/** Add header to payload and send it out */
	static bool WrapAndSendPayload(const TArray<uint8>& Payload, FSocket* Socket);
	/** Receive packages and strip header */
	static bool ReceivePayload(FArrayReader& OutPayload, FSocket* Socket);
};


/**
 * UnrealCV server to interact with external programs
 */
class UNREALCV_API FUE4CVServer : public FTickableGameObject
{
public:
	FUE4CVServer(size_t portNum, FCommandDispatcher* commandDispatcher);
	virtual ~FUE4CVServer();
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const 
	{
		return true;
	}
	virtual TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FUE4CVServer, STATGROUP_Tickables);
	}

private:
	bool Start();
	void Stop();

	void ProcessPendingRequest();
	void TCPConnectionListener();

	FString StringFromBinaryArray(const TArray<uint8>& BinaryArray);
	void BinaryArrayFromString(const FString& Message, TArray<uint8>& OutBinaryArray);

	void HandleRawMessage(const FString& RawMessage);

	void SendClientMessage(FString Message);

private:
	size_t mPortNum;
	FSocket* mListenerSocket;
	FSocket* mConnectedClient;

	uint8  mRecievedDataBuffer[MAX_BUFFERSIZE4RECEIVE];
	uint32 mOffsetForDataRecieve;
	uint32 mPayloadSize;

	enum DECODE_STATE { MAGIC, SIZE, PAYLOAD };
	DECODE_STATE mState;

	TQueue<FRequest, EQueueMode::Spsc> mPendingRequest; // TQueue is a thread safe implementation

	FCommandDispatcher* mCommandDispatcher;
};
