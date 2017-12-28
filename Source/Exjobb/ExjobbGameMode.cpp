// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Exjobb.h"
#include "ExjobbGameMode.h"
#include "PlayerCharacter.h"
#include "UDPSender.h"
#include "ExPlayerController.h"
#include "RemoteDestructibleActor.h"

static uint32 packetID = 0;

AExjobbGameMode::AExjobbGameMode()
	: Super()
{

}

void AExjobbGameMode::BeginPlay()
{
	Super::BeginPlay();

	//GetWorld()->SpawnActor<ARemoteDestructibleActor>(ARemoteDestructibleActor::StaticClass())->SetActorLocation(FVector(-58.425415f, -98.056320f, 240.701645f));
	
}

void AExjobbGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (AExPlayerController* ExPlayerController = Cast<AExPlayerController>(NewPlayer))
	{
		AUDPSender* playerConnection = GetWorld()->SpawnActor<AUDPSender>(AUDPSender::StaticClass());

		if (playerConnection && NewPlayer->NetConnection)
		{
			FString IP = NewPlayer->NetConnection->RemoteAddressToString();
			int32 Port = NewPlayer->NetConnection->GetAddrPort() + 1; // Can't block their connection ;)

			FString socketName(TEXT("%d"), NewPlayer->NetPlayerIndex);
			playerConnection->OpenUDPSocket(socketName, IP, Port);

			PlayerConnections.Add(NewPlayer, playerConnection);
			ExPlayerController->EstablishConnection(IP, Port);
		}
		else
			UE_LOG(LogTemp, Warning, TEXT("Failed to create player connection"));
	}
}

void AExjobbGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (AUDPSender** playerConnection = PlayerConnections.Find(Cast<APlayerController>(Exiting)))
	{
		if (!(*playerConnection))
		{
			UE_LOG(LogTemp, Warning, TEXT("Player Connection existed but UDP sender is not valid!"));
			return;
		}
		(*playerConnection)->CloseUDPSocket();
		(*playerConnection)->Destroy();
		PlayerConnections.Remove(Cast<APlayerController>(Exiting));
	}

	UE_LOG(LogTemp, Warning, TEXT("Exiting Player Connection did not exist!"));
	
}

void AExjobbGameMode::BroadcastUDPPackage(FNetData* netData)
{
	if (!netData)
	{
		UE_LOG(LogNet, Warning, TEXT("Tried to broadcast invalid package"));
		return;
	}

	netData->packetID = packetID;
	packetID++;

	for (TPair<APlayerController*, AUDPSender*> &playerConnection : PlayerConnections)
	{
		if (!playerConnection.Key)
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Player Controller"));
			return;
		}

		if (!playerConnection.Value)
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Player Connection"));
			return;
		}

		playerConnection.Value->SendNetData(netData);
	}
}
