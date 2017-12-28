#pragma once
#include "Runtime/Core/Public/Misc/NetworkGuid.h"
#include "NetDataStructures.generated.h"

namespace NetIDs
{
	static const uint8 NetDataID = 0;
	static const uint8 ChunkDataID = 1;
	static const uint8 ChunkDataArrayID = 2;
};

USTRUCT()
struct FNetData
{
	GENERATED_BODY()

	uint32 packetID;

	uint8 DataID;

	FNetData(uint8 dataID = NetIDs::NetDataID)
		: DataID(dataID)
	{}

	virtual FArchive& Serialize(FArchive &Ar)
	{
		Ar << packetID;
		Ar << DataID;
		return Ar;
	}
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FNetData& netData)
{
	return netData.Serialize(Ar);
}

USTRUCT()
struct FChunkData : public FNetData
{
	GENERATED_BODY()

	uint8 ChunkIndex;

	FVector Location;

	FRotator Rotation;

	FChunkData()
		: FNetData(NetIDs::ChunkDataID)
	{}

	FChunkData(const int32 &chunkIndex, const FVector &location, const FRotator &rotation)
		: FNetData(NetIDs::ChunkDataID)
		, ChunkIndex(chunkIndex)
		, Location(location)
		, Rotation(rotation)
	{ }

	virtual FArchive& Serialize(FArchive &Ar) override
	{
		Ar << packetID; // We need to know if the packet we received contains old information since UDP can arrive in the wrong order and we only care about the newest one
		Ar << ChunkIndex;
		Ar << Location;
		Ar << Rotation;
		//SerializePackedVector<1, 20>(Location, Ar);

		//Rotation.SerializeCompressed(Ar); // Clamps every axis to one byte (360/255)

		return Ar;
	}
};


USTRUCT()
struct FChunkDataArray : public FNetData
{
	GENERATED_BODY()

	FNetworkGUID Recipient;

	TArray<FChunkData> Chunks;

	FChunkDataArray()
		: FNetData(NetIDs::ChunkDataArrayID)
	{}

	FChunkDataArray(const FNetworkGUID &recipient)
		: FNetData(NetIDs::ChunkDataArrayID)
		, Recipient(recipient)
	{}

	virtual FArchive& Serialize(FArchive &Ar) override
	{
		Ar << packetID;
		Ar << DataID;
		Ar << Recipient;
		Ar << Chunks;
		return Ar;
	}
};