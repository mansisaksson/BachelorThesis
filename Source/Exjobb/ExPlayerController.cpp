#include "Exjobb.h"
#include "ExPlayerController.h"
#include "UDPReceiver.h"

void AExPlayerController::EstablishConnection_Implementation(const FString &IP, int32 port)
{
	if (ServerConnection)
	{
		ServerConnection->Destroy();
		UE_LOG(LogTemp, Log, TEXT("Esablished new connection, destroying the old one"));
	}	

	if (NetConnection)
	{
		ServerConnection = GetWorld()->SpawnActor<AUDPReceiver>(AUDPReceiver::StaticClass());
		ServerConnection->OpenConnection(TEXT("ServerConnection"), IP, port);
	}
}

void AExPlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (ServerConnection)
	{
		ServerConnection->CloseUDPSocket();
		ServerConnection->Destroy();
	}
}