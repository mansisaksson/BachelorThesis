#include "Exjobb.h"
#include "UDPReceiver.h"

static TAutoConsoleVariable<int32> CVarSimulatedPhysicsDelay(
	TEXT("net.SimulatedPhysicsDelay"),
	0,
	TEXT("Changes the amount of delay applied to the UDP (Physics) packets"));

static TAutoConsoleVariable<int32> CVarSimulatedPhysicsDelay_AddedRandom(
	TEXT("net.SimulatedPhysicsDelay_AddedRandom"),
	0,
	TEXT("adds a random delay to net.SimulatedPhysicsDelay"));

AUDPReceiver::AUDPReceiver()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	ListenSocket = NULL;
}

void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	delete UDPReceiver;
	UDPReceiver = nullptr;

	if (ListenSocket)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
	}
}

bool AUDPReceiver::OpenConnection(
	const FString& SocketName,
	const FString& IP,
	const int32 Port
) 
{
	lastReceivedPacketID = 0;
	FIPv4Address Addr;
	FIPv4Address::Parse(IP, Addr);

	//Create Socket
	FIPv4Endpoint Endpoint(Addr, Port);

	//BUFFER SIZE
	int32 BufferSize = 2 * 1024 * 1024;

	ListenSocket = FUdpSocketBuilder(*SocketName)
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferSize);
	;

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(0);
	UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("UDP RECEIVER"));
	UDPReceiver->OnDataReceived().BindUObject(this, &AUDPReceiver::Recv);
	UDPReceiver->Start();

	UE_LOG(LogNet, Log, TEXT("\n\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
	UE_LOG(LogNet, Log, TEXT("****UDP**** Receiver Initialized Successfully!!!"));
	UE_LOG(LogNet, Log, TEXT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n"));

	return true;
}

bool AUDPReceiver::CloseUDPSocket()
{
	if (UDPReceiver)
	{
		UDPReceiver->Stop();
		return true;
	}
	return false;
}

void AUDPReceiver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	mutex.Lock();
	for (auto &netPacket : PacketStack)
	{
		//UE_LOG(LogNet, Log, TEXT("Received Chunk Data Array %s"), *netPacket.RDActor->GetName());
		
		//netPacket.RDActor->UpdateChunkData(netPacket.ChunkDataArray);
		
		int32 AddedRandomDelay = CVarSimulatedPhysicsDelay_AddedRandom.GetValueOnGameThread();
		int32 SimulatedNetDelay = CVarSimulatedPhysicsDelay.GetValueOnGameThread() + FMath::RandRange(0, AddedRandomDelay);
		float delay = FMath::Clamp((float)SimulatedNetDelay / 1000.f, 0.0001f, 999999.f);
		//UE_LOG(LogNet, Log, TEXT("Delaying packet with %f seconds"), delay);
		FTimerHandle DummyHandle;
		GetWorldTimerManager().SetTimer(DummyHandle, FTimerDelegate::CreateUObject(this, &AUDPReceiver::UpdateChunkData, netPacket.RDActor, netPacket.ChunkDataArray), delay, false, delay);
	}

	PacketStack.Empty();
	mutex.Unlock();
}

void AUDPReceiver::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	//UE_LOG(LogNet, Log, TEXT("Received bytes: %d"), ArrayReaderPtr->Num());

	FChunkDataArray ChunkDataArray;
	*ArrayReaderPtr << ChunkDataArray;

	if (lastReceivedPacketID >= ChunkDataArray.packetID)
	{
		UE_LOG(LogNet, Warning, TEXT("Received packet in the wrong order"));
		//return; // Might still contain usefull information
	}

	lastReceivedPacketID = ChunkDataArray.packetID;

	if (UObject* recipiant = GetNetDriver()->GuidCache->GetObjectFromNetGUID(ChunkDataArray.Recipient, true))
	{
		if (ARemoteDestructibleActor* rdActor = Cast<ARemoteDestructibleActor>(recipiant))
		{
			mutex.Lock();
			PacketStack.Add(FNetPacket(rdActor, ChunkDataArray));
			mutex.Unlock();
		}
		else
			UE_LOG(LogNet, Warning, TEXT("Actor is not of ARemoteDestructibleActor type"), *recipiant->GetName());
	}
	else
	{
		UE_LOG(LogNet, Warning, TEXT("Unknow recipiant"));
	}

	// TODO: This if ever needed
	//FNetData Data;
	//*ArrayReaderPtr << Data;		//Deserialize ID // This breaks it by removing the id from the archive

	//if (Data.DataID == NetIDs::NetDataID)
	//{
	//	UE_LOG(LogNet, Warning, TEXT("Received empty UDP package"));
	//}
	//else if (Data.DataID == NetIDs::ChunkDataID)
	//{
	//	FChunkData ChunkData;
	//	*ArrayReaderPtr << ChunkData;
	//	UE_LOG(LogNet, Log, TEXT("Received Chunk Data"));
	//}
	//else if (Data.DataID == NetIDs::ChunkDataArrayID)
	//{
	//	FChunkDataArray ChunkDataArray;
	//	*ArrayReaderPtr << ChunkDataArray;

	//	/*if (AActor* recipiant = GetNetDriver()->GetActorForGUID(ChunkDataArray.Recipient))
	//	{
	//		UE_LOG(LogNet, Log, TEXT("Received Chunk Data Array %s"), *recipiant->GetName());
	//	}*/
	//}
}

void AUDPReceiver::UpdateChunkData(ARemoteDestructibleActor* rdActor, FChunkDataArray chunkDataArray)
{
	rdActor->UpdateChunkData(chunkDataArray);
}