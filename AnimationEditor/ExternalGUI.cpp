#include "ExternalGUI.hpp"

#include <string>

using namespace std;

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
void(__stdcall *aegSetTimelineCallback)(TimelineCallback Callback);

void(__stdcall *aegSetEnabled)(const wchar_t* Name, bool IsEnabled);
void(__stdcall *aegSetChecked)(const wchar_t* Name, bool IsChecked);
void(__stdcall *aegSetText)(const wchar_t* Name, const wchar_t* Text);
void(__stdcall *aegSetPosition)(const wchar_t* Name, float t);
void(__stdcall *aegSetTimelineState)(float Position, float Length, int32 SelectedID, TimelineItem* Items, int32 ItemsCount);

void(__stdcall *aegRun)(void);

void(__stdcall *aegFinalize)(void);

#define LoadApi(x) TryGetProcAddress(LibraryHandle, #x, (void**)&x)

const wstring LibraryFileName = L"AnimationEditorGUI.dll";

HMODULE LibraryHandle;

bool SetupExternalGUI(void) {

	LibraryHandle = LoadLibrary(LibraryFileName.c_str());
	if (LibraryHandle == 0)
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
		!LoadApi(aegSetTimelineCallback) ||

		!LoadApi(aegSetEnabled) ||
		!LoadApi(aegSetChecked) ||
		!LoadApi(aegSetText) ||
		!LoadApi(aegSetPosition) ||
		!LoadApi(aegSetTimelineState) ||

		!LoadApi(aegRun) ||
		!LoadApi(aegFinalize)
		)
		return false;

	return true;
}

void UnloadExternalGUI(void) {

	if (FreeLibrary(LibraryHandle))
		LibraryHandle = 0;
}