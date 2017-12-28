// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PhysicsEngine/DestructibleActor.h"
#include "NetDataStructures.h"
#include "RemoteDestructibleActor.generated.h"


USTRUCT()
struct FChunkMovementInfo
{
	GENERATED_BODY()

	UPROPERTY()
		FVector StartLocation;

	UPROPERTY()
		FVector TargetLocation;

	UPROPERTY()
		FRotator StartRotation;

	UPROPERTY()
		FRotator TargetRotation;

	UPROPERTY()
		float TravelAlpha;

	UPROPERTY()
		float RotationAlpha;

	FChunkMovementInfo() {};

	FChunkMovementInfo(const FVector &startLocation, const FVector &targetLocation, const FRotator &startRotation, const FRotator &targetRotation)
		: StartLocation(startLocation)
		, TargetLocation(targetLocation)
		, StartRotation(startRotation)
		, TargetRotation(targetRotation)
		, TravelAlpha(0.f)
		, RotationAlpha(0.f)
	{ }
};

UCLASS(config = Game)
class EXJOBB_API ARemoteDestructibleActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, Category = Components)
		class USceneComponent* Root;

	ARemoteDestructibleActor();

	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void TogglePhysicsUpdate();

	void UpdateChunkData(const FChunkDataArray &chunkDataArray);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DestructibleComponent)
		UDestructibleMesh* DestructibleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
		float TickRate;

private:
	bool HasChunkMoved(const FChunkData &chunk);

	UFUNCTION()
		void OnServerFractureEvent(const FVector &HitPoint, const FVector &HitDirection);

	UFUNCTION(NetMulticast, Reliable)
		void FractureDummyComponent_Broadcast(const FVector &HitPoint);
	void FractureDummyComponent_Broadcast_Implementation(const FVector &HitPoint);


#if WITH_EDITOR
	USkeletalMeshComponent* VisualComponent;
#endif //WITH_EDITOR
	class UDestructibleComponent* DestructibleComponent;
	class UDummyDestructibleComponent* DummyDestructibleComponent;

	bool bUpdate;

	TMap<int32, FChunkData> ChunkCache; // Use by both the client and the server to know what the last chunk packet sent/received looked like
	TMap<int32, FChunkMovementInfo> ChunkMovment;
};
