#pragma once

bool SetupExternalGUI(void);

typedef void(__stdcall *ButtonCallback)(const wchar_t* Name);
typedef void(__stdcall *IdleCallback)(void);

extern void (__stdcall *aegInitialize)(void);
extern void (__stdcall *aegSetIdleCallback)(IdleCallback Callback);
extern void (__stdcall *aegSetOpenGLWindow)(HWND Window);
extern void (__stdcall *aegSetButtonCallback)(const wchar_t* Name, ButtonCallback Callback);
extern void (__stdcall *aegRun)(void);