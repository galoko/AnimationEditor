#include "Render.hpp"

#include <iostream>
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/gl.h>			
#include <GL/glu.h>	

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PhysicsManager.hpp"
#include "InputManager.hpp"
#include "CharacterManager.hpp"
#include "shader.hpp"

#define floatsizeofmember(s, m) (sizeof((((s*)0)->m)) / sizeof(GLfloat))

void Render::Initialize(HWND WindowHandle) {

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

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	// Create and compile our GLSL program from the shaders
	ShaderID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	ProjectionID = glGetUniformLocation(ShaderID, "Projection");
	ViewID = glGetUniformLocation(ShaderID, "View");
	ModelID = glGetUniformLocation(ShaderID, "Model");
	ModelNormalID = glGetUniformLocation(ShaderID, "ModelNormal");

	DiffuseColorID = glGetUniformLocation(ShaderID, "DiffuseColor");
	SpecularColorID = glGetUniformLocation(ShaderID, "SpecularColor");

	IsLightEnabledID = glGetUniformLocation(ShaderID, "IsLightEnabled");
	LightPositionID = glGetUniformLocation(ShaderID, "LightPosition");
	LightDirectionID = glGetUniformLocation(ShaderID, "LightDirection");
	LightAmbientColorID = glGetUniformLocation(ShaderID, "LightAmbientColor");
	LightDiffuseColorID = glGetUniformLocation(ShaderID, "LightDiffuseColor");
	LightPowerID = glGetUniformLocation(ShaderID, "LightPower");

	glUseProgram(ShaderID);

	Projection = perspective(radians(45.0f), AspectRatio, 0.1f, 100.0f);
	glUniformMatrix4fv(ProjectionID, 1, GL_FALSE, value_ptr(Projection));

	// Camera matrix

	CameraPosition = { 2.5f, -2.5, 1.264f };
	LookAtPoint({ 0, 0, 0.264f });
	UpdateViewMatrix();

	LoadPrimitiveModels();

	glClearColor(1, 1, 1, 1);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, floatsizeofmember(Vertex, Position), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, floatsizeofmember(Vertex, Normal), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, floatsizeofmember(Vertex, TexCoord), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoord));

	vec3 LightAmbientColor = { 0.3, 0.3, 0.3 };
	vec3 LightDiffuseColor = { 1.0, 1.0, 1.0 };
	float LightPower = 1;

	glUniform1i(IsLightEnabledID, GL_TRUE);
	glUniform3fv(LightAmbientColorID, 1, value_ptr(LightAmbientColor));
	glUniform3fv(LightDiffuseColorID, 1, value_ptr(LightDiffuseColor));
	glUniform1f(LightPowerID, LightPower);
}

void Render::LoadPrimitiveModel(const wchar_t* ModelName, Vertex* Buffer, uint32 BufferSize, uint32 &DestIndex, 
	uint32 &ResultIndex, uint32& ResultSize) {
	
	wstring FileName = L".\\Models\\" + wstring(ModelName) + L".bin";

	FILE* ModelFile = _wfopen(FileName.c_str(), L"rb");

	uint32 Remaining = BufferSize - DestIndex;

	uint32 Readed = (uint32) fread(Buffer + DestIndex, sizeof(Vertex), Remaining, ModelFile);

	ResultIndex = DestIndex;
	ResultSize = Readed;

	DestIndex += Readed;

	fclose(ModelFile);
}

void Render::LoadPrimitiveModels(void) {

	glGenBuffers(1, &BufferName);
	glBindBuffer(GL_ARRAY_BUFFER, BufferName);

	const int BufferSize = 500;
	Vertex Buffer[BufferSize];

	uint32 CurrentIndex = 0;

	LoadPrimitiveModel(L"plane", Buffer, BufferSize, CurrentIndex, PlaneStart, PlaneSize);
	LoadPrimitiveModel(L"cube", Buffer, BufferSize, CurrentIndex, CubeStart, CubeSize);
	LoadPrimitiveModel(L"sphere", Buffer, BufferSize, CurrentIndex, SphereStart, SphereSize);

	// line
	LineStart = CurrentIndex;
	Buffer[CurrentIndex++] = { { -0.5f, 0, 0 }, {}, {} };
	Buffer[CurrentIndex++] = { {  0.5f, 0, 0 }, {}, {} };
	LineSize = CurrentIndex - LineStart;

	glBufferData(GL_ARRAY_BUFFER, CurrentIndex * sizeof(Vertex), Buffer, GL_STATIC_DRAW);
}

void Render::DrawScene(void) {

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawFloor();

	Character* Char = CharacterManager::GetInstance().GetCharacter();
	DrawCharacter(Char);

	DrawPickedPoint();

	glFlush();
	SwapBuffers(WindowDC);
}

void Render::DrawCharacter(Character* Char) {

	vec4 DiffuseColor = { 0.7, 0.7, 0.7, 1 };
	vec3 SpecularColor = { 0, 0, 0 };

	glUniform4fv(DiffuseColorID, 1, value_ptr(DiffuseColor));
	glUniform3fv(SpecularColorID, 1, value_ptr(SpecularColor));

	for (Bone* Bone : Char->Bones) {

		mat4 SizeMatrix = scale(mat4(1.0f), Bone->Size);
		mat4 Model = Bone->WorldTransform * Bone->MiddleTranslation;

		mat4 FinalModel = Model * SizeMatrix;

		// setup matrix
		glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
		glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
		glDrawArrays(GL_TRIANGLES, CubeStart, CubeSize);
	}
}

void Render::DrawFloor(void) {

	vec4 DiffuseColor = { 0, 0, 0, 1 };
	vec3 SpecularColor = { 0, 0, 0 };

	glUniform4fv(DiffuseColorID, 1, value_ptr(DiffuseColor));
	glUniform3fv(SpecularColorID, 1, value_ptr(SpecularColor));

	mat4 Translation = translate(mat4(1.0f), PhysicsManager::GetInstance().GetFloorPosition());
	mat4 SizeMatrix = scale(mat4(1.0f), PhysicsManager::GetInstance().GetFloorSize());

	mat4 FinalModel = Translation * SizeMatrix;

	// setup matrix
	glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Translation));
	glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
	// draw cube
	glDrawArrays(GL_TRIANGLES, CubeStart, CubeSize);
}

void Render::DrawPickedPoint(void) {

	vec3 PickedPoint, PlaneNormal;
	float PlaneDistance;
	
	bool HavePickedPoint = InputManager::GetInstance().GetPickedPoint(PickedPoint, PlaneNormal, PlaneDistance);

	if (HavePickedPoint) {

		vec4 DiffuseColor = { 1, 0, 0, 0.7 };
		vec3 SpecularColor = { 0, 0, 0 };

		glUniform4fv(DiffuseColorID, 1, value_ptr(DiffuseColor));
		glUniform3fv(SpecularColorID, 1, value_ptr(SpecularColor));

		mat4 Translation = translate(mat4(1.0f), PickedPoint);
		mat4 SizeMatrix = scale(mat4(1.0f), vec3(0.05f));

		mat4 FinalModel = Translation * SizeMatrix;

		// setup matrix
		glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Translation));
		glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
		// draw cube
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, SphereStart, SphereSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Translation = translate(mat4(1.0f), PlaneNormal * PlaneDistance + (PickedPoint - PlaneNormal * dot(PlaneNormal, PickedPoint)));

		vec3 Axis = cross(PlaneNormal, Up);
		float Angle = -acos(dot(PlaneNormal, Up));
		mat4 NormalRotation = rotate(mat4(1.0f), Angle, Axis);

		mat4 FacingRotation = rotate(mat4(1.0f), atan2(PlaneNormal.y, PlaneNormal.x), Up);

		mat4 Rotation = NormalRotation * FacingRotation;

		SizeMatrix = scale(mat4(1.0f), vec3(1.0f));

		mat4 Model = Translation * Rotation;
		FinalModel = Model * SizeMatrix;

		DiffuseColor = { 0, 0, 0.5, 0.5 };
		glUniform4fv(DiffuseColorID, 1, value_ptr(DiffuseColor));

		glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
		glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
		glDrawArrays(GL_TRIANGLES, PlaneStart, PlaneSize);
	}
}

// Controls API

void Render::RotateCamera(float OffsetZ, float OffsetX)
{
	CameraAngleZ += OffsetZ;
	CameraAngleX += OffsetX;

	UpdateViewMatrix();
}

void Render::MoveCamera(vec3 Offset)
{
	CameraPosition += Offset;

	UpdateViewMatrix();
}

void Render::LookAtPoint(vec3 Point) {

	vec3 Direction = normalize(Point - CameraPosition);

	float SinX = sqrt(1 - Direction.z * Direction.z);

	CameraAngleZ = atan2(Direction.x / SinX, Direction.y / SinX);
	CameraAngleX = atan2(SinX, Direction.z);

	UpdateViewMatrix();
}

vec3 Render::GetLookingDirection(void) {

	vec3 Direction;
	Direction.x = sinf(CameraAngleX) * sinf(CameraAngleZ);
	Direction.y = sinf(CameraAngleX) * cosf(CameraAngleZ);
	Direction.z = cosf(CameraAngleX);

	return Direction;
}

vec3 Render::GetCameraPosition(void)
{
	return CameraPosition;
}

void Render::GetPointAndDirectionFromScreenPoint(LONG x, LONG y, vec3& Point, vec3 & Direction) {

	vec4 RayStartScreen(
		((float)x / Width - 0.5f) * 2.0f,
		((float)y / Height - 0.5f) * -2.0f,
		0,
		1.0f
	);

	vec4 RayEndScreen(RayStartScreen.x, RayStartScreen.y, 1.0, 1.0f);

	mat4 M = inverse(Projection * View);

	vec4 RayStartWorld = M * RayStartScreen;
	RayStartWorld /= RayStartWorld.w;

	vec4 RayEndWorld = M * RayEndScreen;
	RayEndWorld /= RayEndWorld.w;

	Point = RayStartWorld;
	Direction = normalize(RayEndWorld - RayStartWorld);
}

void Render::GetScreenPointFromPoint(vec3 Point, LONG& x, LONG& y) {

	mat4 M = Projection * View;

	vec4 ScreenPoint = M * vec4(Point, 1);
	ScreenPoint /= ScreenPoint.w;

	x = (LONG)round(((ScreenPoint.x /  2.0f) + 0.5f) * Width);
	y = (LONG)round(((ScreenPoint.y / -2.0f) + 0.5f) * Height);
}

void Render::GetBoneFromScreenPoint(LONG x, LONG y, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal) {

	vec3 Point, Direction;

	GetPointAndDirectionFromScreenPoint(x, y, Point, Direction);

	return PhysicsManager::GetInstance().GetBoneFromRay(Point, Direction, TouchedBone, WorldPoint, WorldNormal);
}

// Matrices Calculations

void Render::UpdateViewMatrix(void)
{
	CameraAngleZ /= (float)M_PI * 2.0f;
	CameraAngleZ -= (float)(long)CameraAngleZ;
	CameraAngleZ *= (float)M_PI * 2.0f;

	CameraAngleX = min(max(0.1f, CameraAngleX), 3.13f);

	vec3 Direction = GetLookingDirection();

	View = lookAt(CameraPosition, CameraPosition + Direction, Up);

	glUniformMatrix4fv(ViewID, 1, GL_FALSE, value_ptr(View));

	glUniform3fv(LightPositionID, 1, value_ptr(CameraPosition));
	glUniform3fv(LightDirectionID, 1, value_ptr(Direction));
}