#include "Exjobb.h"
#include "UDPSender.h"

AUDPSender::AUDPSender()
{
	SenderSocket = NULL;

	ShowOnScreenDebugMessages = true;
}

void AUDPSender::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CloseUDPSocket();
}

bool AUDPSender::OpenUDPSocket(
	const FString& SocketName,
	const FString& IP,
	const int32 Port) 
{
	RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bIsValid;
	RemoteAddr->SetIp(*IP, bIsValid);
	RemoteAddr->SetPort(Port);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDP Sender>> IP address was not valid! %s"), *IP);
		return false;
	}

	SenderSocket = FUdpSocketBuilder(*SocketName).
		AsReusable().
		WithBroadcast();


	//Set Send Buffer Size
	int32 SendSize = 2 * 1024 * 1024;
	SenderSocket->SetSendBufferSize(SendSize, SendSize);
	SenderSocket->SetReceiveBufferSize(SendSize, SendSize);

	UE_LOG(LogNet, Log, TEXT("\n\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
	UE_LOG(LogNet, Log, TEXT("****UDP**** Sender Initialized Successfully!!!"));
	UE_LOG(LogNet, Log, TEXT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n"));

	return true;
}

bool AUDPSender::CloseUDPSocket()
{
	if (SenderSocket)
	{
		SenderSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SenderSocket);
		return true;
	}
	return false;
}

bool AUDPSender::SendNetData(FNetData* netData)
{
	if (!SenderSocket)
	{
		UE_LOG(LogNet, Warning, TEXT("No sender socket! %s"));
		return false;
	}

	if (!netData)
	{
		UE_LOG(LogNet, Log, TEXT("Tried to sen invalid NetData"));
		return false;
	}

	// **** Serialize package *****
	FArrayWriter Writer;
	Writer << *netData;

	// **** Send package *****
	int32 BytesSent = 0;
	SenderSocket->SendTo(Writer.GetData(), Writer.Num(), BytesSent, *RemoteAddr);

	if (BytesSent <= 0)
	{
		const FString Str = "Socket is valid but the receiver received 0 bytes, make sure it is listening properly!";
		UE_LOG(LogNet, Error, TEXT("%s"), *Str);
		return false;
	}

	return true;
}