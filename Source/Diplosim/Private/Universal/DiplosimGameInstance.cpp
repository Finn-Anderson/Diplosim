#include "Universal/DiplosimGameInstance.h"

UDiplosimGameInstance::UDiplosimGameInstance()
{
	bShutdown = false;
}

void UDiplosimGameInstance::Shutdown()
{
	bShutdown = true;

	Super::Shutdown();
}