#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glm/glm.hpp>

using namespace glm;

bool SetupExternalGUI(void);
void UnloadExternalGUI(void);

#define RESET_ROOT_POSITION L"ResetRootPosition"
#define X_POS               L"XPos"
#define Y_POS               L"YPos"
#define Z_POS               L"ZPos"
#define X_AXIS              L"XAxis"
#define Y_AXIS              L"YAxis"
#define Z_AXIS              L"ZAxis"
#define X_POS_INPUT         L"XPosInput"
#define Y_POS_INPUT         L"YPosInput"
#define Z_POS_INPUT         L"ZPosInput"
#define X_ANGLE_INPUT       L"XAngleInput"
#define Y_ANGLE_INPUT       L"YAngleInput"
#define Z_ANGLE_INPUT       L"ZAngleInput"
#define X_AXIS_BAR          L"XAxisBar"
#define Y_AXIS_BAR          L"YAxisBar"
#define Z_AXIS_BAR          L"ZAxisBar"
#define OPEN_DIALOG         L"OpenDialog"
#define NEW_DIALOG          L"NewDialog"
#define CREATE_STATE        L"CreateState"
#define DELETE_STATE        L"DeleteState"
#define UNDO_DELETE         L"UndoDelete"
#define MIRROR_STATE        L"MirrorState"
#define ANIMATION_LENGTH    L"AnimationLength"
#define PLAY_STOP           L"PlayStop"
#define ANIMATION_LOOP      L"AnimationLoop"	 

typedef struct TimelineItem {
	int32 ID;
	float Position;
} TimelineItem;

typedef void (__stdcall *ButtonCallback)(const wchar_t* Name);
typedef void (__stdcall *CheckBoxCallback)(const wchar_t* Name, bool IsChecked);
typedef void (__stdcall *EditCallback)(const wchar_t* Name, const wchar_t* Text);
typedef void (__stdcall *TrackBarCallback)(const wchar_t* Name, float t);
typedef void (__stdcall *TimelineCallback)(float Position, float Length, int32 SelectedID, TimelineItem* Items, int32 ItemsCount);
typedef void (__stdcall *IdleCallback)(void);

extern void (__stdcall *aegInitialize)(void);
extern void (__stdcall *aegSetOpenGLWindow)(HWND Window);

extern void (__stdcall *aegSetIdleCallback)(IdleCallback Callback);
extern void (__stdcall *aegSetButtonCallback)(ButtonCallback Callback);
extern void (__stdcall *aegSetCheckBoxCallback)(CheckBoxCallback Callback);
extern void (__stdcall *aegSetEditCallback)(EditCallback Callback);
extern void (__stdcall *aegSetTrackBarCallback)(TrackBarCallback Callback);
extern void (__stdcall *aegSetTimelineCallback)(TimelineCallback Callback);

extern void (__stdcall *aegSetEnabled)(const wchar_t* Name, bool IsEnabled);
extern void (__stdcall *aegSetChecked)(const wchar_t* Name, bool IsChecked);
extern void (__stdcall *aegSetText)(const wchar_t* Name, const wchar_t* Text);
extern void (__stdcall *aegSetPosition)(const wchar_t* Name, float t);
extern void (__stdcall *aegSetTimelineState)(float Position, float Length, int32 SelectedID, TimelineItem* Items, int32 ItemsCount);

extern void (__stdcall *aegRun)(void);

extern void (__stdcall *aegFinalize)(void);