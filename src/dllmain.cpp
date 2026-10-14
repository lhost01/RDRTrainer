#include "pch.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(module);
			Application::Initialize(module);
			break;

		case DLL_PROCESS_DETACH:
			Application::Shutdown();
			break;

		default:
			break;
	}

	return TRUE;
}
