#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

typedef class Form {
private:
	Form(void) { };

	const WCHAR* WindowClass = L"OpenGLTest";
	const WCHAR* Title = L"OpenGL Test Box";

	// Window handling

	HWND WindowHandle;

	static LRESULT CALLBACK WndProcStaticCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	static Form& GetInstance(void) {
		static Form Instance;

		return Instance;
	}

	Form(Form const&) = delete;
	void operator=(Form const&) = delete;

	HWND Initialize(HINSTANCE hInstance);
	void Tick(double dt);
} Form;