// Fill out your copyright notice in the Description page of Project Settings.

#include "Exjobb.h"
#include "Engine/DestructibleMesh.h"
#include "DummyDestructibleComponent.h"

//~~~~~~~~~~~~~~~~~~~~~~~~
//	   PhysX 			
#include "PhysXIncludes.h" 
#include "PhysicsPublic.h"  //FPhysScene
#include "PhysXPublic.h"    //PtoU conversions
//~~~~~~~~~~~~~~~~~~~~~~~~

UDummyDestructibleComponent::UDummyDestructibleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	OnComponentFracture.AddDynamic(this, &UDummyDestructibleComponent::OnComponentFractureEvent);
}

void UDummyDestructibleComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UDummyDestructibleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//if (SkeletalMesh == NULL)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Skeletal mesh is NULL"));
	//	return;
	//}

	//UWorld* World = GetWorld();
	//FPhysScene* PhysScene = World->GetPhysicsScene();
	//check(PhysScene);

	//if (GApexModuleDestructible == NULL)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("UDestructibleComponent::OnCreatePhysicsState(): APEX must be enabled to init UDestructibleComponent physics."));
	//	return;
	//}

	//UDestructibleMesh* TheDestructibleMesh = GetDestructibleMesh();
	//if (TheDestructibleMesh == NULL || TheDestructibleMesh->ApexDestructibleAsset == NULL)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("UDestructibleComponent::OnCreatePhysicsState(): No DestructibleMesh or missing ApexDestructibleAsset."));
	//	return;
	//}

	//int32 ChunkCount = TheDestructibleMesh->ApexDestructibleAsset->getChunkCount();
	//// Ensure the chunks start off invisible.  RefreshBoneTransforms should make them visible.
	//for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
	//{
	//	//UE_LOG(LogTemp, Log, TEXT("Set visibility chunk index: %d"), ChunkIndex);
	//	SetChunkVisible(ChunkIndex, true/*(ChunkIndex % 2) == 1*/);
	//}

	//SetChunkVisible(0, false);

	//ChunkCount = ApexDestructibleActor->getNumVisibleChunks();
	//const uint16* ChunkIdxs = ApexDestructibleActor->getVisibleChunks();

	//for (int32 ThisIdx = 0; ThisIdx < ChunkCount; ThisIdx++)
	//{
	//	PxRigidDynamic* PxChunkActor = ApexDestructibleActor->getChunkPhysXActor(ChunkIdxs[ThisIdx]);

	//	if (PxChunkActor)
	//		PxChunkActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
	//	
	//	//UE_LOG(LogTemp, Log, TEXT("Setting Fracture peice to Kinematic: %i"), ThisIdx);
	//}
}

void UDummyDestructibleComponent::FractureComponent(const FVector &HitPosition)
{
#if WITH_APEX
	if (ApexDestructibleActor != NULL)
	{
		UE_LOG(LogTemp, Log, TEXT("*** applyRadiusDamage to dummy component!"));
		//ApexDestructibleActor->applyDamage(10000000.f, 0.f, U2PVector(HitPosition), U2PVector(FVector::ZeroVector));
		ApexDestructibleActor->applyRadiusDamage(9999999999.f, 0.f, U2PVector(HitPosition), 1000.f, false);
	}
#endif
}

void UDummyDestructibleComponent::OnComponentFractureEvent(const FVector &HitPoint, const FVector &HitDirection) 
{
	UE_LOG(LogTemp, Log, TEXT("*** DummyComponent Fractured!"));

	if (!ApexDestructibleActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("*** Invalid ApexDestructibleActor"));
		return;
	}

	int32 ChunkCount = ApexDestructibleActor->getNumVisibleChunks();
	const uint16* ChunkIdxs = ApexDestructibleActor->getVisibleChunks();

	for (int32 ThisIdx = 0; ThisIdx < ChunkCount; ThisIdx++)
	{
		PxRigidDynamic* PxChunkActor = ApexDestructibleActor->getChunkPhysXActor(ChunkIdxs[ThisIdx]);

		if (PxChunkActor)
		{
			PxChunkActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			PxChunkActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true);
			PxChunkActor->setActorFlag(physx::PxActorFlag::eDISABLE_SIMULATION, false);
		}
		else
			UE_LOG(LogTemp, Warning, TEXT("*** Tried to set invalid chunk to Kinematic (Invalid Chunk Idex?)"));
	}
}