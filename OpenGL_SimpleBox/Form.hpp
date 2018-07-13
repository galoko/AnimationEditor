#pragma once

#include <string>
#include <iostream>
#include <vector>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <WindowsX.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <GL/gl.h>			
#include <GL/glu.h>	

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

using namespace std;
using namespace glm;

typedef class Form {
private:
	Form(void) { };

	HWND WindowHandle;
	HDC WindowDC;
	BOOL IsInFocus;

	GLuint ShaderID, MatrixID, DrawColorID;
	mat4 MVP;
	GLuint BufferName;

	double StepTime;
	float Angle;

	mat4 Projection, View, Model;

	typedef struct Vertex {

		vec3 Position, Normal, Color;

	} Vertex;

	// OpenGL
	void SetupOpenGL(void);
	void DrawScene(void);

	// Model generation
	static void GenerateCubeQuad(Vertex* QuadMesh, vec3 normal, vec3 color, uint32 id);
	static void GenerateCube(Vertex* CubeMesh);

	typedef class Bone {

		uint32 StartId; // this id..id+5 are ids of this bone quads

		vec3 Offset, Size;

		vector<Bone*> Childs;

	} Bone;

	typedef class Character {

		vec3 Position;

		Bone* Pelvis; // also known as Root

	} Character;

	// TODO character generation

	// Controls
	void ProcessMouseInput(LONG dx, LONG dy);

	// Window handling
	static LRESULT CALLBACK WndProcStaticCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	static Form& GetInstance(void) {
		static Form Instance;

		return Instance;
	}

	Form(Form const&) = delete;
	void operator=(Form const&) = delete;

	// Window handling
	void CreateMainWindow(HINSTANCE hInstance);

	// Integration
	void Tick(double dt);
} Form;