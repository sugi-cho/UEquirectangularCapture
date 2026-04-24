#include "Modules/ModuleManager.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FUEquirectangularCaptureModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UEquirectangularCapture"));
		if (Plugin.IsValid())
		{
			const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
			AddShaderSourceDirectoryMapping(TEXT("/UEquirectangularCapture"), ShaderDir);
		}
	}
};

IMPLEMENT_MODULE(FUEquirectangularCaptureModule, UEquirectangularCapture)
