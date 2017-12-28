// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Networking.h"
#include "GameFramework/Actor.h"
#include "NetDataStructures.h"
#include "RemoteDestructibleActor.h"
#include "UDPReceiver.generated.h"

struct FNetPacket
{
	FNetPacket(ARemoteDestructibleActor* rdActor, const FChunkDataArray &chunkDataArray)
		: RDActor(rdActor)
		, ChunkDataArray(MoveTemp(chunkDataArray))
	{}

	ARemoteDestructibleActor* RDActor;
	FChunkDataArray ChunkDataArray;
};

UCLASS()
class EXJOBB_API AUDPReceiver : public AActor
{
	GENERATED_BODY()
	
public:
	AUDPReceiver();

public:
	FSocket* ListenSocket;

	FUdpSocketReceiver* UDPReceiver = nullptr;
	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

	bool OpenConnection(
		const FString& SocketName,
		const FString& IP,
		const int32 Port
	);

	bool CloseUDPSocket();

private:

	void UpdateChunkData(ARemoteDestructibleActor* rdActor, FChunkDataArray chunkDataArray);

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FCriticalSection mutex;
	FEvent * semaphore;

	uint32 lastReceivedPacketID;

	TArray<FNetPacket> PacketStack;
};
