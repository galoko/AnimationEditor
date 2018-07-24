#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <glm/glm.hpp>

using namespace std;
using namespace glm;

typedef class Form {
private:
	Form(void) { };

	const WCHAR* WindowClass = L"OpenGLTest";
	const WCHAR* Title = L"OpenGL Test Box";

	// Window handling

	HWND WindowHandle;

	// Update

	int32 UpdateBlockCounter;
	bool IsBlockingUpdatePending;
	
	void ProcessPendingUpdates(void);

	static LRESULT CALLBACK WndProcStaticCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static void __stdcall ButtonStaticCallback(const wchar_t* Name);
	void ButtonCallback(const wstring Name);
	static void __stdcall CheckBoxStaticCallback(const wchar_t* Name, bool IsChecked);
	void CheckBoxCallback(const wstring Name, bool IsChecked);

	void SetupDefaultValues(void);
public:
	static Form& GetInstance(void) {
		static Form Instance;

		return Instance;
	}

	Form(Form const&) = delete;
	void operator=(Form const&) = delete;

	HWND Initialize(HINSTANCE hInstance);
	void Tick(double dt);

	typedef struct UpdateLock {
		UpdateLock(void) {
			Form::GetInstance().UpdateBlockCounter++;
		}
		~UpdateLock(void) {
			int32 Counter = --Form::GetInstance().UpdateBlockCounter;
			if (Counter == 0)
				Form::GetInstance().ProcessPendingUpdates();
		}
	} UpdateLock;

	void UpdateBlocking(void);
} Form;