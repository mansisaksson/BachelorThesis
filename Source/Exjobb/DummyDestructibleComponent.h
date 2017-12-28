// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/DestructibleComponent.h"
#include "DummyDestructibleComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Physics, meta = (BlueprintSpawnableComponent))
class EXJOBB_API UDummyDestructibleComponent : public UDestructibleComponent
{
	GENERATED_BODY()

public:
	UDummyDestructibleComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void FractureComponent(const FVector &HitPosition);

	UFUNCTION()
		void OnComponentFractureEvent(const FVector &HitPosition, const FVector &HitDirection);
};
