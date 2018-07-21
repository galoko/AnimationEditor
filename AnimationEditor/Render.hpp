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

		vec3 Position, Normal;
		vec2 TexCoord;

	} Vertex;

	GLuint 
		ShaderID,
		ProjectionID, ViewID, ModelID, ModelNormalID,
		DiffuseColorID, SpecularColorID,
		IsLightEnabledID, LightPositionID, LightDirectionID, LightAmbientColorID, LightDiffuseColorID, LightPowerID;

	mat4 Projection, View;

	GLuint BufferName;
	uint32 CubeStart, CubeSize, SphereStart, SphereSize, PlaneStart, PlaneSize, LineStart, LineSize;

	vec3 CameraPosition;
	float CameraAngleX, CameraAngleZ;

	void LoadPrimitiveModel(const wchar_t* ModelName, Vertex* Buffer, uint32 BufferSize, uint32 &DestIndex, uint32 &ResultIndex, uint32& ResultSize);
	void LoadPrimitiveModels(void);

	void SetWireframeMode(bool IsWireFrame);
	void SetColors(vec4 DiffuseColor, vec3 SpecularColor = { 0, 0, 0 });

	void DrawCube(vec3 Position, mat4 Rotation, vec3 Size);
	void DrawCube(mat4 Model, vec3 Size);
	void DrawPlane(vec3 Position, vec3 Normal, vec3 Size);
	void DrawSphere(vec3 Position, mat4 Rotation, vec3 Size);

	void DrawCharacter(Character* Char);
	void DrawFloor(void);
	void DrawPickedPoint(void);

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

	void Initialize(HWND WindowHandle);

	void DrawScene(void);

	void RotateCamera(float OffsetZ, float OffsetX);
	void MoveCamera(vec3 Offset);

	void LookAtPoint(vec3 Point);
	vec3 GetLookingDirection(void);
	vec3 GetCameraPosition(void);

	void GetPointAndDirectionFromScreenPoint(LONG x, LONG y, vec3& Point, vec3& Direction);
	void GetScreenPointFromPoint(vec3 Point, LONG& x, LONG& y);
	void GetBoneFromScreenPoint(LONG x, LONG y, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal);
} Render;