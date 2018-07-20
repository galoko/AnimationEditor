#pragma once

#include <windows.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "Character.hpp"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

using namespace glm;

typedef class Render {
private:
	Render(void) { };

	HDC WindowDC;

	typedef struct Vertex {

		vec3 Position, Normal, Color;

	} Vertex;

	GLuint ShaderID, MatrixID, ColorIntencityID;
	mat4 Projection, View;

	GLuint BufferName;
	Vertex Buffer[6 * 2 * 3];

	vec3 CameraPosition;
	float CameraAngleX, CameraAngleZ;

	static void GenerateCubeQuad(Vertex* QuadMesh, vec3 normal, vec3 color, uint32 id);
	static void GenerateCube(Vertex* CubeMesh);

	void DrawCharacter(Character* Char);
	void DrawFloor(void);

	void UpdateViewMatrix(void);
public:
	static Render& GetInstance(void) {
		static Render Instance;

		return Instance;
	}

	Render(Render const&) = delete;
	void operator=(Render const&) = delete;

	const int Width = 1280;
	const int Height = 720;
	const float AspectRatio = (float)Width / Height;
	const vec3 Up = { 0, 0, 1 };

	void RotateCamera(float OffsetZ, float OffsetX);
	void MoveCamera(vec3 Offset);

	void LookAtPoint(vec3 Point);
	vec3 GetLookingDirection(void);

	void Initialize(HWND WindowHandle);

	void DrawScene(void);

	void GetBoneFromScreenPoint(LONG x, LONG y, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal);
} Render;