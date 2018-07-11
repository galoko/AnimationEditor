#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define GLEW_STATIC
// Windows Header Files:
#include <windows.h>
#include <WindowsX.h>

#include <string>
#include <iostream>

#include <GL/glew.h>

#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glu.h>			/* OpenGL utilities header file */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

using namespace std;
using namespace glm;

static HWND WindowHandle;
static HDC WindowDC;
static BOOL IsInFocus;

void OpenConsole(void) {

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
}

bool ProcessMessages(void) {

	MSG Msg;

	// messages
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

		if (Msg.message == WM_QUIT)
			return false;

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return true;
}

static LARGE_INTEGER Freq;

void InitTime(void) {
	QueryPerformanceFrequency(&Freq);
}

LONGLONG GetTime(void) {

	LARGE_INTEGER Result;

	QueryPerformanceCounter(&Result);

	return Result.QuadPart;
}

double TimeToSeconds(LONGLONG Time) {

	return (double)Time / (double)Freq.QuadPart;
} 

static GLuint programID;
static GLuint MatrixID;
static mat4 MVP;
static GLuint buffer;

static mat4 Projection;
static mat4 View;
static mat4 Model;

void SetupWindow(void) {

	PIXELFORMATDESCRIPTOR PixelFormatDesc =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	WindowDC = GetDC(WindowHandle);
	int PixelFormat = ChoosePixelFormat(WindowDC, &PixelFormatDesc);
	SetPixelFormat(WindowDC, PixelFormat, &PixelFormatDesc);

	HGLRC hGL = wglCreateContext(WindowDC);
	if (!wglMakeCurrent(WindowDC, hGL))
		throw new runtime_error("Couldn't assign DC to OpenGL");

	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
		throw new runtime_error("Couldn't init GLEW");

	glEnable(GL_DEPTH_TEST);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	Projection = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	View = lookAt(
		vec3(0, -5, 0), 
		vec3(0, 0, 0), // and looks at the origin
		vec3(0, 0, 1)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	static const GLfloat g_buffer_data[] = {
		-1.0f,-1.0f,-1.0f,   0.583f, 0.771f, 0.014f,
		-1.0f,-1.0f, 1.0f,   0.609f, 0.115f, 0.436f,
		-1.0f, 1.0f, 1.0f,   0.327f, 0.483f, 0.844f,
		 1.0f, 1.0f,-1.0f,   0.822f, 0.569f, 0.201f,
		-1.0f,-1.0f,-1.0f,   0.435f, 0.602f, 0.223f,
		-1.0f, 1.0f,-1.0f,   0.310f, 0.747f, 0.185f,
		 1.0f,-1.0f, 1.0f,   0.597f, 0.770f, 0.761f,
		-1.0f,-1.0f,-1.0f,   0.559f, 0.436f, 0.730f,
		 1.0f,-1.0f,-1.0f,   0.359f, 0.583f, 0.152f,
		 1.0f, 1.0f,-1.0f,   0.483f, 0.596f, 0.789f,
		 1.0f,-1.0f,-1.0f,   0.559f, 0.861f, 0.639f,
		-1.0f,-1.0f,-1.0f,   0.195f, 0.548f, 0.859f,
		-1.0f,-1.0f,-1.0f,   0.014f, 0.184f, 0.576f,
		-1.0f, 1.0f, 1.0f,   0.771f, 0.328f, 0.970f,
		-1.0f, 1.0f,-1.0f,   0.406f, 0.615f, 0.116f,
		 1.0f,-1.0f, 1.0f,   0.676f, 0.977f, 0.133f,
		-1.0f,-1.0f, 1.0f,   0.971f, 0.572f, 0.833f,
		-1.0f,-1.0f,-1.0f,   0.140f, 0.616f, 0.489f,
		-1.0f, 1.0f, 1.0f,   0.997f, 0.513f, 0.064f,
		-1.0f,-1.0f, 1.0f,   0.945f, 0.719f, 0.592f,
		 1.0f,-1.0f, 1.0f,   0.543f, 0.021f, 0.978f,
		 1.0f, 1.0f, 1.0f,   0.279f, 0.317f, 0.505f,
		 1.0f,-1.0f,-1.0f,   0.167f, 0.620f, 0.077f,
		 1.0f, 1.0f,-1.0f,   0.347f, 0.857f, 0.137f,
		 1.0f,-1.0f,-1.0f,   0.055f, 0.953f, 0.042f,
		 1.0f, 1.0f, 1.0f,   0.714f, 0.505f, 0.345f,
		 1.0f,-1.0f, 1.0f,   0.783f, 0.290f, 0.734f,
		 1.0f, 1.0f, 1.0f,   0.722f, 0.645f, 0.174f,
		 1.0f, 1.0f,-1.0f,   0.302f, 0.455f, 0.848f,
		-1.0f, 1.0f,-1.0f,   0.225f, 0.587f, 0.040f,
		 1.0f, 1.0f, 1.0f,   0.517f, 0.713f, 0.338f,
		-1.0f, 1.0f,-1.0f,   0.053f, 0.959f, 0.120f,
		-1.0f, 1.0f, 1.0f,   0.393f, 0.621f, 0.362f,
		 1.0f, 1.0f, 1.0f,   0.673f, 0.211f, 0.457f,
		-1.0f, 1.0f, 1.0f,   0.820f, 0.883f, 0.371f,
		 1.0f,-1.0f, 1.0f,   0.982f, 0.099f, 0.879f
	};

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_buffer_data), g_buffer_data, GL_STATIC_DRAW);

	glClearColor(1, 1, 1, 1);

	// Use our shader
	glUseProgram(programID);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		6 * sizeof(float),  // stride
		(void*)0            // array buffer offset
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		6 * sizeof(float),  // stride
		(void*)(3 * sizeof(float))            // array buffer offset
	);

	// glDisableVertexAttribArray(0);
}

void HandleResize(int Width, int Height) {

	glViewport(0, 0, Width, Height);
}

static float angle;

void DrawScene(void) {

	// Advance scene
	angle += radians(1.0f);

	// Prepare MVP
	Model = rotate(rotate(rotate(mat4(1.0f), angle, vec3(0, 0, 1)), angle, vec3(0, 1, 0)), angle, vec3(1, 0, 0));
	MVP = Projection * View * Model;
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw the triangles !
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3); // 12*3 indices starting at 0 -> 12 triangles

	glFlush();
	SwapBuffers(WindowDC);
}

void ProcessMouseInput(LONG dx, LONG dy) {

}

LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_CREATE: {

		WindowHandle = hWnd;

		SetupWindow();

		break;
	}
	case WM_SIZE: {

		int Width = LOWORD(lParam);
		int Height = HIWORD(lParam);

		HandleResize(Width, Height);

		break;
	}
	case WM_PAINT: {
		
		DrawScene();

		break;
	}
	// controls
	case WM_INPUT: {

		UINT RimType;
		HRAWINPUT RawHandle;
		RAWINPUT RawInput;
		UINT RawInputSize, Res;

		RimType = GET_RAWINPUT_CODE_WPARAM(wParam);
		RawHandle = HRAWINPUT(lParam);

		RawInputSize = sizeof(RawInput);
		Res = GetRawInputData(RawHandle, RID_INPUT, &RawInput, &RawInputSize, sizeof(RawInput.header));

		if (Res != (sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE)))
			break;

		if ((RawInput.data.mouse.usFlags & 1) != MOUSE_MOVE_RELATIVE)
			break;

		if (IsInFocus)
			ProcessMouseInput(RawInput.data.mouse.lLastX, RawInput.data.mouse.lLastY);

		break;
	}
	case WM_KEYDOWN: {

		int Key = (int)wParam;
		// ...?
		break;
	}
	case WM_KEYUP: {

		int Key = (int)wParam;
		// ...?
		break;
	}
	case WM_RBUTTONDOWN:

		break;
	case WM_MOUSEWHEEL: {

		int WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		// ...?
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM RegisterClass(LPCWSTR ClassName, WNDPROC WndProc, HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = ClassName;
	wcex.hIconSm = 0;

	return RegisterClassExW(&wcex);
}

void CreateMainWindow(HINSTANCE hInstance) {

	const WCHAR* WindowClass = L"OpenGLTest";
	const WCHAR* Title = L"OpenGL Test Box";
	const int Width = 1280;
	const int Height = 960;

	RegisterClass(WindowClass, WndProcCallback, hInstance);

	POINT WindowPos;
	WindowPos.x = (GetSystemMetrics(SM_CXSCREEN) - Width) / 2;
	WindowPos.y = (GetSystemMetrics(SM_CYSCREEN) - Height) / 2;

	CreateWindowW(WindowClass, Title, WS_POPUP, WindowPos.x, WindowPos.y, Width, Height, nullptr, nullptr, hInstance, nullptr);
	if (WindowHandle == 0)
		throw new runtime_error("Couldn't create window");

	// register raw input

	RAWINPUTDEVICE Rid[1] = {};

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = WindowHandle;

	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])))
		throw new runtime_error("Couldn't register raw devices");

	// show the whole thing

	ShowWindow(WindowHandle, SW_SHOW);
}

void Tick(double dt) {

	RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OpenConsole();

	InitTime();

	CreateMainWindow(hInstance);

	LONGLONG LastTick = GetTime();

	// Main message loop:
	while (TRUE) {

		// process input
		if (!ProcessMessages())
			break;

		// time calculation
		LONGLONG Now = GetTime();
		double dt = TimeToSeconds(Now - LastTick);
		LastTick = Now;

		// actual draw call
		Tick(dt);
	}
}