
#include "UTOnslaught.h"
#include "ModuleManager.h"
#include "ModuleInterface.h"

class FUTOnslaughtPlugin : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FUTOnslaughtPlugin, UTOnslaught )

void FUTOnslaughtPlugin::StartupModule()
{
}

void FUTOnslaughtPlugin::ShutdownModule()
{
}