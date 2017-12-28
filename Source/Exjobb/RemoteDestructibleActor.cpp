// Fill out your copyright notice in the Description page of Project Settings.

#include "Exjobb.h"
#include "RemoteDestructibleActor.h"
#include "Engine/DestructibleMesh.h"
#include "Kismet/KismetStringLibrary.h"
#include "RemoteDestructibleComponent.h"
#include "Components/DestructibleComponent.h"
#include "Components/InputComponent.h"
#include "DummyDestructibleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ExjobbGameMode.h"
#include "RemoteDestructibleComponent.h"

//~~~~~~~~~~~~~~~~~~~~~~~~
//	   PhysX 			
#include "PhysXIncludes.h" 
#include "PhysicsPublic.h"  //FPhysScene
#include "PhysXPublic.h"    //PtoU conversions
//~~~~~~~~~~~~~~~~~~~~~~~~

namespace 
{
	template <typename T>
	T* DynamicConstructObject(AActor* Outer)
	{
		if (Outer)
		{
			T* createdObject = NewObject<T>(Outer);
			createdObject->SetupAttachment(Outer->GetRootComponent());
			createdObject->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			createdObject->RegisterComponent();
			return createdObject;
		}
		
		return nullptr;
	}

	FVector MoveTowards(FVector A, FVector B, float stepSize)
	{
		if (A == B)
			return B;

		FVector dir = (B - A);
		FVector dirN = dir.GetSafeNormal();

		FVector step = dirN * stepSize;
		if (dir.Size() > step.Size())
			return A + step;

		return B;
	}
};

ARemoteDestructibleActor::ARemoteDestructibleActor()
	: bUpdate(false)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(Root);

	bReplicates = true;

	TickRate = 30.f;
}

void ARemoteDestructibleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
#if WITH_EDITOR
	if (DestructibleMesh->IsValidLowLevel() && GIsEditor && !GetWorld()->IsGameWorld())
	{
		VisualComponent = DynamicConstructObject<USkeletalMeshComponent>(this);
		VisualComponent->SetSkeletalMesh(DestructibleMesh);
		VisualComponent->SetHiddenInGame(true);
	}
	
#endif //WITH_EDITOR

}

void ARemoteDestructibleActor::BeginPlay()
{
	Super::BeginPlay();

	if (DestructibleMesh->IsValidLowLevel())
	{
		if (HasAuthority())
		{
			DestructibleComponent = DynamicConstructObject<UDestructibleComponent>(this);
			DestructibleComponent->SetDestructibleMesh(DestructibleMesh);
			DestructibleComponent->SetIsReplicated(false);
			DestructibleComponent->SetSimulatePhysics(false);
			DestructibleComponent->OnComponentFracture.AddDynamic(this, &ARemoteDestructibleActor::OnServerFractureEvent);

			PrimaryActorTick.TickInterval = 1.f / TickRate;

			//GetNetDriver()->GuidCache->AssignNewNetGUID_Server(this); // Is not needed
		}
		else
		{
			DummyDestructibleComponent = DynamicConstructObject<UDummyDestructibleComponent>(this);
			DummyDestructibleComponent->SetSkeletalMesh(DestructibleMesh);
			DummyDestructibleComponent->SetIsReplicated(false);
			DummyDestructibleComponent->SetSimulatePhysics(false);
			PrimaryActorTick.TickInterval = 0.0f;
		}
	}
	else
		UKismetSystemLibrary::PrintString(this, TEXT("awdawdawd"));

	if (InputComponent)
	{
		InputComponent->BindAction("Test", EInputEvent::IE_Pressed, this, &ARemoteDestructibleActor::TogglePhysicsUpdate);
		//InputComponent->BindKey(EKeys::T, EInputEvent::IE_Pressed, this, &ARemoteDestructibleActor::TogglePhysicsUpdate);
	}
}

void ARemoteDestructibleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// Send physics info
		if (!DestructibleComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Server) No DestructibleComponent"));
			return;
		}

		if (!DestructibleComponent->ApexDestructibleActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Server) No ApexDestructibleActor"));
			return;
		}

		AExjobbGameMode* gameMode = Cast<AExjobbGameMode>(GetWorld()->GetAuthGameMode());
		if (!gameMode)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Server) failed to get GameMode"));
			return;
		}

		UNetDriver* netDriver = GetNetDriver();
		if (!netDriver)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Server) failed to get NetDirver"));
			return;
		}

		FNetworkGUID Recipient = netDriver->GuidCache->GetNetGUID(this);
		if (Recipient.IsDefault())
		{
			UE_LOG(LogNet, Warning, TEXT("Unknown Recipiant for Chunk Package"));
			return;
		}

		FChunkDataArray chunkDataArray;
		chunkDataArray.Recipient = Recipient;

		int32 ChunkCount = DestructibleComponent->ApexDestructibleActor->getNumVisibleChunks();
		const uint16* ChunkIdxs = DestructibleComponent->ApexDestructibleActor->getVisibleChunks();

		// Check for chunks that have moved and broadcast them to relevant clients
		for (int32 i = 0; i < ChunkCount; i++)
		{
			PxRigidDynamic* PxChunkActor = DestructibleComponent->ApexDestructibleActor->getChunkPhysXActor(ChunkIdxs[i]);
			if (PxChunkActor)
			{
				FTransform ChunkTransfrom = P2UTransform(PxChunkActor->getGlobalPose());
				FChunkData chunk(ChunkIdxs[i], ChunkTransfrom.GetLocation(), ChunkTransfrom.Rotator());

				if (!HasChunkMoved(chunk)) // We don't want to send the client info if the chunk has not moved
					continue;

				uint32 packetID = 0;
				if (auto lastReceivedChunk = ChunkCache.Find(ChunkIdxs[i])) // Check for id of last chunk sent with this index and add one (Used for knowing if the packet is old)
					packetID = lastReceivedChunk->packetID + 1;

				chunk.packetID = packetID;
				chunkDataArray.Chunks.Add(chunk);
				ChunkCache.Add(ChunkIdxs[i], chunk); // Cache the sent information

				if ((i + 1) % 27 == 0) // Max 27 chunks per package (UDP has a "safe limit" of 512 bytes)
				{
					gameMode->BroadcastUDPPackage((FNetData*)&chunkDataArray);
					chunkDataArray.Chunks.Empty();
				}
			}
		}	

		if (chunkDataArray.Chunks.Num() > 0)
			gameMode->BroadcastUDPPackage(&chunkDataArray);
	}
	else
	{
		// Update physics info on the client
		if (!DummyDestructibleComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Client) No DummyDestructibleComponent"));
			return;
		}

		if (!DummyDestructibleComponent->ApexDestructibleActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("(Client) No ApexDestructibleActor"));
			return;
		}

		for (auto It = ChunkMovment.CreateIterator(); It; ++It)
		{
			PxRigidDynamic* PxChunkActor = DummyDestructibleComponent->ApexDestructibleActor->getChunkPhysXActor(It.Key());

			if (!PxChunkActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("(Client) Tried to update non existing Chunk, chunk id: %s"), *UKismetStringLibrary::Conv_IntToString(It.Key()));
				return;
			}

			//UE_LOG(LogTemp, Log, TEXT("Pre: %f"), It.Value().TravelAlpha);
			//UE_LOG(LogTemp, Log, TEXT("Pre Start Location: %s"), *UKismetStringLibrary::Conv_VectorToString(It.Value().StartLocation));
			//UE_LOG(LogTemp, Log, TEXT("Pre Target Location: %s"), *UKismetStringLibrary::Conv_VectorToString(It.Value().TargetLocation));

			It.Value().TravelAlpha = UKismetMathLibrary::FClamp(It.Value().TravelAlpha + TickRate * DeltaTime, 0.0f, 1.0f);
			FVector newLocation = UKismetMathLibrary::VLerp(It.Value().StartLocation, It.Value().TargetLocation, It.Value().TravelAlpha);
			FRotator newRotation = UKismetMathLibrary::RLerp(It.Value().StartRotation, It.Value().TargetRotation, It.Value().TravelAlpha, true);

			//UE_LOG(LogTemp, Log, TEXT("ID: %d"), It.Key());
			//UE_LOG(LogTemp, Log, TEXT("Travel alpha: %f"), It.Value().TravelAlpha);
			//UE_LOG(LogTemp, Log, TEXT("Target Rotation: %s"), *It.Value().TargetRotation.ToString());
			//UE_LOG(LogTemp, Log, TEXT("new Rotation: %s "), *newRotation.ToString());
			
			//FVector newLocation = It.Value().TargetLocation;
			//FRotator newRotation = It.Value().TargetRotation;

			//PxChunkActor->setGlobalPose(U2PTransform(FTransform(newRotation, newLocation)), false);
			PxChunkActor->setKinematicTarget(U2PTransform(FTransform(newRotation, newLocation)));
			DummyDestructibleComponent->SetChunkWorldRT(It.Key(), newRotation.Quaternion(), newLocation);

			FTransform chunkTransform = P2UTransform(PxChunkActor->getGlobalPose());
			//UE_LOG(LogTemp, Log, TEXT("actual Rotation: %s "), *chunkTransform.GetRotation().ToString());
			//UE_LOG(LogTemp, Log, TEXT("----"));

			if (It.Value().TravelAlpha == 1.f /*&& It.Value().RotationAlpha == 1.f*/)
				It.RemoveCurrent();
		}
	}
}

void ARemoteDestructibleActor::TogglePhysicsUpdate()
{
	bUpdate = !bUpdate;
	UE_LOG(LogTemp, Log, TEXT("Physics Update Toggled: %s"), *UKismetStringLibrary::Conv_BoolToString(bUpdate));
}

void ARemoteDestructibleActor::UpdateChunkData(const FChunkDataArray &chunkDataArray)
{
	if (!HasAuthority() && DummyDestructibleComponent && DummyDestructibleComponent->ApexDestructibleActor)
	{
		for (auto &chunkData : chunkDataArray.Chunks)
		{
			if (auto lastReceivedChunk = ChunkCache.Find(chunkData.ChunkIndex))
			{
				if (lastReceivedChunk->packetID >= chunkData.packetID)
				{
					UE_LOG(LogNet, Warning, TEXT("Received old chunk data, discardning packet ReceivedID: %d, CachedID: %d"), chunkData.packetID, lastReceivedChunk->packetID);
					continue;
				}
			}
			ChunkCache.Add(chunkData.ChunkIndex, chunkData);

			PxRigidDynamic* PxChunkActor = DummyDestructibleComponent->ApexDestructibleActor->getChunkPhysXActor(chunkData.ChunkIndex);
			if (PxChunkActor)
			{
				FTransform ChunkTransfrom = P2UTransform(PxChunkActor->getGlobalPose());
				ChunkMovment.Add(chunkData.ChunkIndex, FChunkMovementInfo(ChunkTransfrom.GetLocation(), chunkData.Location, ChunkTransfrom.Rotator(), chunkData.Rotation));
			}
		}
	}
}

bool ARemoteDestructibleActor::HasChunkMoved(const FChunkData &chunk)
{
	if (auto chunkCache = ChunkCache.Find(chunk.ChunkIndex))
	{
		if (chunk.Location.Equals(chunkCache->Location, 0.1f) && chunk.Rotation.Equals(chunkCache->Rotation, 1.4f))
			return false;
	}
	return true;
}

void ARemoteDestructibleActor::OnServerFractureEvent(const FVector &HitPoint, const FVector &HitDirection)
{
	UE_LOG(LogTemp, Log, TEXT("*** Server Fractured!"));
	FractureDummyComponent_Broadcast(HitPoint);
}

void ARemoteDestructibleActor::FractureDummyComponent_Broadcast_Implementation(const FVector &HitPoint)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("*** Client received fracture event"));

		if (DummyDestructibleComponent)
			DummyDestructibleComponent->FractureComponent(HitPoint);
		else
			UE_LOG(LogTemp, Warning, TEXT("Failed to fracture DummyDestructibleComponent (NULL Dummy Component)"));
	}
}