#pragma once
#include "NetDataStructures.h"
#include "ExjobbGameMode.generated.h"

UCLASS(minimalapi)
class AExjobbGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AExjobbGameMode();

	virtual void BeginPlay();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	void BroadcastUDPPackage(FNetData* netData);

private:
	class AUDPSender* UDPSender;
	TMap<APlayerController*, class AUDPSender*> PlayerConnections;
};

