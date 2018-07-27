#include "ExternalGUI.hpp"

bool TryGetProcAddress(HMODULE Handle, LPCSTR Name, void** Address) {

	*Address = GetProcAddress(Handle, Name);
	return *Address != nullptr;
}

void(__stdcall *aegInitialize)(void);
void(__stdcall *aegSetOpenGLWindow)(HWND Window);

void(__stdcall *aegSetIdleCallback)(IdleCallback Callback);
void(__stdcall *aegSetButtonCallback)(ButtonCallback Callback);
void(__stdcall *aegSetCheckBoxCallback)(CheckBoxCallback Callback);
void(__stdcall *aegSetEditCallback)(EditCallback Callback);
void(__stdcall *aegSetTrackBarCallback)(TrackBarCallback Callback);

void(__stdcall *aegSetEnabled)(const wchar_t* Name, bool IsEnabled);
void(__stdcall *aegSetChecked)(const wchar_t* Name, bool IsChecked);
void(__stdcall *aegSetText)(const wchar_t* Name, const wchar_t* Text);
void(__stdcall *aegSetPosition)(const wchar_t* Name, float t);

void(__stdcall *aegRun)(void);

#define LoadApi(x) TryGetProcAddress(Handle, #x, (void**)&x)

bool SetupExternalGUI(void) {

	HMODULE Handle = LoadLibrary(L"AnimationEditorGUI.dll");
	if (Handle == 0)
		return false;

	if 
		(
		!LoadApi(aegInitialize) ||
		!LoadApi(aegSetOpenGLWindow) ||

		!LoadApi(aegSetIdleCallback) ||
		!LoadApi(aegSetButtonCallback) ||
		!LoadApi(aegSetCheckBoxCallback) ||
		!LoadApi(aegSetEditCallback) ||
		!LoadApi(aegSetTrackBarCallback) ||

		!LoadApi(aegSetEnabled) ||
		!LoadApi(aegSetChecked) ||
		!LoadApi(aegSetText) ||
		!LoadApi(aegSetPosition) ||

		!LoadApi(aegRun)
		)
		return false;

	return true;
}