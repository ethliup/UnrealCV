// Fill out your copyright notice in the Description page of Project Settings.

#include "UE4CVServer.h"
#include "ConsoleHelper.h"
#include <string>
#include "TimerManager.h"

uint32 FSocketMessageHeader::DefaultMagic = 0x9E2B83C1;

bool FSocketMessageHeader::WrapAndSendPayload(const TArray<uint8>& Payload, FSocket* Socket)
{
	if (Socket == NULL)
	{
		return false;
	}
	FSocketMessageHeader Header(Payload);

	FBufferArchive Ar;
	Ar << Header.Magic;
	Ar << Header.PayloadSize;
	Ar.Append(Payload);

	int32 AmountSent;
	Socket->Send(Ar.GetData(), Ar.Num(), AmountSent);
	if (AmountSent != Ar.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to send."));
		return false;
	}
	return true;
}

bool FSocketMessageHeader::ReceivePayload(FArrayReader& OutPayload, FSocket* Socket)
{
	return true;
}

FUE4CVServer::FUE4CVServer(size_t portNum, FCommandDispatcher* commandDispatcher)
{
	mPortNum = portNum;
	mOffsetForDataRecieve = 0;
	mState = DECODE_STATE::MAGIC;
	mCommandDispatcher = commandDispatcher;

	mListenerSocket = nullptr;
	mConnectedClient = nullptr;

	if (!this->Start()) // default port number is 9000
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot start TCP server on port 9000, exit..."));
		exit(0);
	}
}

FUE4CVServer::~FUE4CVServer()
{
	this->Stop();
}

bool FUE4CVServer::Start() // Restart the server if configuration changed
{
	// create new TCP server
	FIPv4Endpoint Endpoint(FIPv4Address::Any, mPortNum);
	mListenerSocket = FTcpSocketBuilder(TEXT("FTcpListener server"))
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	if (mListenerSocket)
	{
		int32 NewSize = 0;
		mListenerSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);
		UE_LOG(LogTemp, Warning, TEXT("Start listening on port %d"), mPortNum);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Can not start listening on port %d, Port might be in use"), mPortNum);
		return false;
	}
	return true;
}

void FUE4CVServer::Stop()
{
	if (mListenerSocket)
	{
		mListenerSocket->Close();
		UE_LOG(LogTemp, Warning, TEXT("Close socket on port %d"), mPortNum);
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(mListenerSocket);
		mListenerSocket = nullptr;
	}
	if (mConnectedClient)
	{
		mConnectedClient->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(mConnectedClient);
		mConnectedClient = nullptr;
	}
}

void FUE4CVServer::Tick(float DeltaTime)
{
	// scan client connect request regularly
	TCPConnectionListener();

	if (mConnectedClient == NULL) return;

	int nRead = 0;
	bool bRecvStatus = mConnectedClient->Recv(&mRecievedDataBuffer[mOffsetForDataRecieve], MAX_BUFFERSIZE4RECEIVE - mOffsetForDataRecieve, nRead);
	mOffsetForDataRecieve += nRead;
	
	if (nRead == 0) return;
	if (mOffsetForDataRecieve < sizeof(FSocketMessageHeader)) return;

	// try to decode the data
	uint32 index4track = 0;
	while (true)
	{
		switch (mState)
		{
		case MAGIC:
		{
			uint32 MagicNum;
			memcpy(&MagicNum, mRecievedDataBuffer + index4track, 4);
			index4track += 4;
			if (MagicNum != FSocketMessageHeader::DefaultMagic)
			{
				UE_LOG(LogTemp, Warning, TEXT("Bad network header magic"));
			}
			else
			{
				mState = SIZE;
			}
			break;
		}
		case SIZE:
		{
			memcpy(&mPayloadSize, mRecievedDataBuffer + index4track, 4);
			index4track += 4;
			if (!mPayloadSize) 
			{
				UE_LOG(LogTemp, Warning, TEXT("Empty payload"));
				mState = MAGIC;
			}
			else
			{
				mState = PAYLOAD;
			}
			break;
		}
		case PAYLOAD:
		{
			if ((mPayloadSize + index4track) <= mOffsetForDataRecieve + 1)
			{
				// copy data 
				TArray<uint8> payloadData;
				payloadData.Append(mRecievedDataBuffer + index4track, mPayloadSize);
				this->HandleRawMessage(FString(StringFromBinaryArray(payloadData)));
				index4track += mPayloadSize;
				mState = MAGIC;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Wait for more payload data..."));
			}
			break;
		}
		}
		if ((mState == MAGIC || mState == SIZE) && index4track + 4 > (mOffsetForDataRecieve + 1))
		{	
			break;
		}
		if (mState == PAYLOAD && (index4track + mPayloadSize) > (mOffsetForDataRecieve + 1))
		{
			break;
		}
	}
	// do memory copy
	uint8* dummy = new uint8[2048];
	memcpy(dummy, mRecievedDataBuffer + index4track, mOffsetForDataRecieve - index4track + 1);
	memcpy(mRecievedDataBuffer, dummy, mOffsetForDataRecieve - index4track + 1);
	mOffsetForDataRecieve = (mOffsetForDataRecieve - index4track > 0 ? mOffsetForDataRecieve - index4track : 0);
	delete dummy;
	dummy = NULL;

	ProcessPendingRequest();
}

void FUE4CVServer::TCPConnectionListener()
{
	if (!mListenerSocket) return;
	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	// handle incoming connections
	if (mListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		if (mConnectedClient != NULL)
		{
			return;
		}

		////Already have a Connection? destroy previous
		//if (mConnectedClient)
		//{
		//	mConnectedClient->Close();
		//	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(mConnectedClient);
		//	mConnectedClient = nullptr;
		//}
		//New Connection receive!
		FSocket* newSocket = mListenerSocket->Accept(*RemoteAddress, TEXT("TCP Server Received Socket Connection"));

		if (newSocket != NULL)
		{
			/*if (mConnectedClient != NULL)
			{
				mConnectedClient->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(mConnectedClient);
				delete mConnectedClient;
				mConnectedClient = NULL;
			} 
			*/
			mConnectedClient = newSocket;

			UE_LOG(LogTemp, Warning, TEXT("New TCP client connected from %s"), *FIPv4Endpoint(RemoteAddress).ToString());

			FString MsgAck = FString::Printf(TEXT("Welcome, you are connected to %s"), FApp::GetGameName());
			SendClientMessage(MsgAck);
		}
	}
}


FString FUE4CVServer::StringFromBinaryArray(const TArray<uint8>& BinaryArray)
{
	std::string cstr(reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num());
	return FString(cstr.c_str());
}

void FUE4CVServer::BinaryArrayFromString(const FString& Message, TArray<uint8>& OutBinaryArray)
{
	FTCHARToUTF8 Convert(*Message);
	OutBinaryArray.Empty();
	OutBinaryArray.Append((UTF8CHAR*)Convert.Get(), Convert.Length());
}

/** Message handler for server */
void FUE4CVServer::HandleRawMessage(const FString& InRawMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("Request: %s"), *InRawMessage);
	// Parse Raw Message
	FString MessageFormat = "(\\d{1,8}):(.*)";
	FRegexPattern RegexPattern(MessageFormat);
	FRegexMatcher Matcher(RegexPattern, InRawMessage);

	if (Matcher.FindNext())
	{
		// TODO: Handle malform request message
		FString StrRequestId = Matcher.GetCaptureGroup(1);
		FString Message = Matcher.GetCaptureGroup(2);

		uint32 RequestId = FCString::Atoi(*StrRequestId);
		FRequest Request(Message, RequestId);
		this->mPendingRequest.Enqueue(Request);
	}
	else
	{
		SendClientMessage(FString::Printf(TEXT("error: Malformat raw message '%s'"), *InRawMessage));
	}
}

// Each tick of GameThread.
void FUE4CVServer::ProcessPendingRequest()
{
	while (!mPendingRequest.IsEmpty())
	{
		FRequest Request;
		bool DequeueStatus = mPendingRequest.Dequeue(Request);
		check(DequeueStatus);
		int32 RequestId = Request.RequestId;

		FCallbackDelegate CallbackDelegate;
		CallbackDelegate.BindLambda([this, RequestId](FExecStatus ExecStatus)
		{
			UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ExecStatus.GetMessage());
			FString ReplyRawMessage = FString::Printf(TEXT("%d:%s"), RequestId, *ExecStatus.GetMessage());
			SendClientMessage(ReplyRawMessage);
		});
		mCommandDispatcher->ExecAsync(Request.Message, CallbackDelegate);
	}
}

void FUE4CVServer::SendClientMessage(FString Message)
{
	if (mConnectedClient)
	{
		TArray<uint8> Payload;
		BinaryArrayFromString(Message, Payload);

		FSocketMessageHeader::WrapAndSendPayload(Payload, mConnectedClient);
	}
}







