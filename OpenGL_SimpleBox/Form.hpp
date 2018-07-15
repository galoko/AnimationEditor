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

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <btBulletDynamicsCommon.h>

#include "Character.hpp"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

using namespace std;
using namespace glm;

#define PHYSICS_FPS 240

typedef class Form {
private:
	Form(void);

	// Integration
	double StepTime, PhysicsTime;
	uint64 DoneStepCount;

	// Character

	Character* Char;

	// Floor
	
	vec3 FloorPosition, FloorSize;
	void CreateFloor(float FloorSize2D, float FloorHeight, float FloorZ);

	// OpenGL

	typedef struct Vertex {

		vec3 Position, Normal, Color;

	} Vertex;

	GLuint ShaderID, MatrixID, ColorIntencityID;
	mat4 Projection, View;
	GLuint BufferName;

	void SetupOpenGL(void);
	void DrawScene(void);
	void DrawCharacter(Character* Char);
	void DrawFloor(void);

	// Model generation
	static void GenerateCubeQuad(Vertex* QuadMesh, vec3 normal, vec3 color, uint32 id);
	static void GenerateCube(Vertex* CubeMesh);

	// Physics
	btDiscreteDynamicsWorld* World;

	void SetupBulletWorld(void);
	btRigidBody* AddDynamicBox(mat4 Transform, vec3 Size, float Mass);
	btRigidBody* AddStaticBox(mat4 Transform, vec3 Size);

	// Controls
	void ProcessMouseInput(LONG dx, LONG dy);

	// Window handling

	HWND WindowHandle;
	HDC WindowDC;
	BOOL IsInFocus;

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