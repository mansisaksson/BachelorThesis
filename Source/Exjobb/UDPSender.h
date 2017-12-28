#pragma once
#include "Networking.h"
#include "NetDataStructures.h"
#include "GameFramework/Actor.h"
#include "UDPSender.generated.h"

UCLASS()
class EXJOBB_API AUDPSender : public AActor
{
	GENERATED_BODY()
	
public:
	AUDPSender();

	TSharedPtr<FInternetAddr> RemoteAddr;
	FSocket* SenderSocket;

	bool OpenUDPSocket(
		const FString& SocketName,
		const FString& IP,
		const int32 Port
	);

	bool CloseUDPSocket();

	bool SendNetData(FNetData* netData);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Sender")
		bool ShowOnScreenDebugMessages;


public:

	/** Called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
