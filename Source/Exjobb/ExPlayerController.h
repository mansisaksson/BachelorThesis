// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "ExPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class EXJOBB_API AExPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	UFUNCTION(Client, Reliable)
		void EstablishConnection(const FString &IP, int32 port);
	void EstablishConnection_Implementation(const FString &ip, int32 port);
	
protected:
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason);

private:
	class AUDPReceiver* ServerConnection;
};
