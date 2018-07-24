#include <windows.h>

#include "ExternalGUI.hpp"

bool TryGetProcAddress(HMODULE Handle, LPCSTR Name, void** Address) {

	*Address = GetProcAddress(Handle, Name);
	return *Address != nullptr;
}

void(__stdcall *aegInitialize)(void);
void(__stdcall *aegSetIdleCallback)(IdleCallback Callback);
void(__stdcall *aegSetOpenGLWindow)(HWND Window);
void(__stdcall *aegSetButtonCallback)(ButtonCallback Callback);
void(__stdcall *aegSetCheckBoxCallback)(CheckBoxCallback Callback);
void(__stdcall *aegSetEnabled)(const wchar_t* Name, bool IsEnabled);
void(__stdcall *aegSetChecked)(const wchar_t* Name, bool IsChecked);
void(__stdcall *aegRun)(void);

#define LoadApi(x) TryGetProcAddress(Handle, #x, (void**)&x)

bool SetupExternalGUI(void) {

	HMODULE Handle = LoadLibrary(L"AnimationEditorGUI.dll");
	if (Handle == 0)
		return false;

	if 
		(
		!LoadApi(aegInitialize) ||
		!LoadApi(aegSetIdleCallback) ||
		!LoadApi(aegSetOpenGLWindow) ||
		!LoadApi(aegSetButtonCallback) ||
		!LoadApi(aegSetCheckBoxCallback) ||
		!LoadApi(aegSetEnabled) ||
		!LoadApi(aegSetChecked) ||
		!LoadApi(aegRun)
		)
		return false;

	return true;
}