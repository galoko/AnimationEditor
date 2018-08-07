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
#include "PoseManager.hpp"
#include "CharacterManager.hpp"
#include "SerializationManager.hpp"
#include "shader.hpp"
#include "texture.hpp"

void Render::DrawScene(void) {

	bool IsKinematic = SerializationManager::GetInstance().IsInKinematicMode();

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawFloor();

	Character* Char = CharacterManager::GetInstance().GetCharacter();
	DrawCharacter(Char, IsKinematic);

	// DrawCharacterGrid();
	
	if (!IsKinematic) {
		DrawAxes();

		DrawPickedPoint();
	}

	glFlush();
	SwapBuffers(WindowDC);
}

void Render::DrawCharacter(Character* Char, bool IsKinematic) {

	SetWireframeMode(false);
	EnableLighting(true);

	InputSelection Selection = InputManager::GetInstance().GetSelection();

	for (Bone* Bone : Char->Bones) {

		vec4 Color;

		if (!IsKinematic && Bone == Selection.Bone)
			Color = { 255 / 255.0f, 163 / 255.0f, 0, 1 };
		else 
			Color = { 0.7, 0.7, 0.7, 1 };

		if (!IsKinematic && PoseManager::GetInstance().GetBoneBlocking(Bone).IsFullyBlocked())
			Color = vec4(vec3(Color) * 0.36f, 1);
	
		SetColors(Color);

		DrawCube(Bone->WorldTransform * Bone->MiddleTranslation, Bone->Size);

		if (!IsKinematic && Bone->PoseCtx->Pinpoint.IsActive()) {

			SetWireframeMode(true);
			SetColors({ 0, 0, 1, 0.7 });

			DrawSphere(Bone->PoseCtx->Pinpoint.DestWorldPoint, mat4(1.0f), vec3(0.04f));

			SetWireframeMode(false);
		}
	}

	if (!IsKinematic) {

		EnableLighting(false);
		SetColors({ 0, 0.7, 0, 1 });

		for (Bone* Bone : Char->Bones) {

			mat4 World = Bone->WorldTransform * Bone->MiddleTranslation;

			vec3 LocalBoneCenter = Bone->LogicalDirection * (dot(Bone->LogicalDirection, Bone->Size) * 0.5f);
			vec3 LocalDirectionEnd = LocalBoneCenter + Bone->LogicalDirection * 0.125f;

			vec3 WorldBoneCenter = World * vec4(LocalBoneCenter, 1);
			vec3 WorldDirectionEnd = World * vec4(LocalDirectionEnd, 1);

			DrawLine(WorldBoneCenter, WorldDirectionEnd);
		}
	}
}

void Render::DrawFloor(void) {

	SetWireframeMode(false);
	EnableLighting(false);
	// SetColors({ 0, 0, 0, 1 });
	SetTexture(FloorTextureID);

	vec3 Position = PhysicsManager::GetInstance().GetFloorPosition();
	vec3 Size = PhysicsManager::GetInstance().GetFloorSize();

	DrawCube(Position, mat4(1.0f), Size);
}

void Render::DrawPickedPoint(void) {

	InputState State = InputManager::GetInstance().GetState();
	InputSelection Selection = InputManager::GetInstance().GetSelection();

	if (Selection.Bone == nullptr)
		return;

	EnableLighting(false);

	SetWireframeMode(true);
	SetColors({ 1, 0, 0, 0.7 });

	DrawSphere(Selection.GetWorldPoint(), mat4(1.0f), vec3(0.05f));

	if (State == InverseKinematic) {

		vec3 PlaneNormal = InputManager::GetInstance().GetPlaneNormal();

		SetWireframeMode(false);
		SetColors({ 0, 0, 0.5, 0.5 });

		DrawPlane(Selection.GetWorldPoint(), PlaneNormal, vec3(1.0f));
	}
}

void Render::DrawCharacterGrid(void) {

	SetWireframeMode(false);
	EnableLighting(false);
	SetColors({ 0, 0.7, 0, 0.5 });
	DrawGrid({}, 2, 0.5f);
}

void Render::DrawAxes(void) {

	SetWireframeMode(false);
	EnableLighting(false);

	SetColors({ 1, 0, 0, 1 });
	DrawLine({ -1, 0, 0 }, { 1, 0, 0 });

	SetColors({ 0, 1, 0, 1 });
	DrawLine({ 0, -1, 0 }, { 0, 1, 0 });

	SetColors({ 0, 0, 1, 1 });
	DrawLine({ 0, 0, -1 }, { 0, 0, 1 });
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

void Render::Serialize(RenderSerializedState & State)
{
	State.CameraPosition = CameraPosition;
	State.CameraAngleX = CameraAngleX;
	State.CameraAngleZ = CameraAngleZ;
}

void Render::Deserialize(RenderSerializedState & State)
{
	CameraPosition = State.CameraPosition;
	CameraAngleX = State.CameraAngleX;
	CameraAngleZ = State.CameraAngleZ;

	UpdateViewMatrix();
}

// Primitives

void Render::LoadPrimitiveModel(const wchar_t* ModelName, Vertex* Buffer, uint32 BufferSize, uint32 &DestIndex,
	uint32 &ResultIndex, uint32& ResultSize) {

	wstring FileName = L".\\Models\\" + wstring(ModelName) + L".bin";

	FILE* ModelFile = _wfopen(FileName.c_str(), L"rb");

	uint32 Remaining = BufferSize - DestIndex;

	uint32 Readed = (uint32)fread(Buffer + DestIndex, sizeof(Vertex), Remaining, ModelFile);

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
	Buffer[CurrentIndex++] = { {}, {}, {} };
	Buffer[CurrentIndex++] = { Up, {}, {} };
	LineSize = CurrentIndex - LineStart;

	glBufferData(GL_ARRAY_BUFFER, CurrentIndex * sizeof(Vertex), Buffer, GL_STATIC_DRAW);
}

// Render API

void Render::SetWireframeMode(bool IsWireFrame) {

	if (IsWireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Render::EnableLighting(bool Enabled) {

	glUniform1i(IsLightEnabledID, Enabled ? GL_TRUE : GL_FALSE);
}

void Render::SetColors(vec4 DiffuseColor, vec3 SpecularColor) {

	glUniform1i(UseTextureID, GL_FALSE);
	glUniform4fv(DiffuseColorID, 1, value_ptr(DiffuseColor));
	glUniform3fv(SpecularColorID, 1, value_ptr(SpecularColor));
}

void Render::SetTexture(GLint TextureID)
{
	glUniform1i(UseTextureID, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D, TextureID);
}

void Render::DrawCube(vec3 Position, mat4 Rotation, vec3 Size) {

	mat4 Model = translate(mat4(1.0f), Position) * Rotation;

	DrawCube(Model, Size);
}

void Render::DrawCube(mat4 Model, vec3 Size) {

	mat4 FinalModel = Model * scale(mat4(1.0f), Size);

	glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
	glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
	glDrawArrays(GL_TRIANGLES, CubeStart, CubeSize);
}

void Render::DrawPlane(vec3 Position, vec3 Normal, vec3 Size) {

	vec3 Axis = cross(Normal, Up);
	mat4 NormalRotation;
	if (length(Axis) > 0) {
		float Angle = -acos(dot(Normal, Up));
		NormalRotation = rotate(mat4(1.0f), Angle, Axis);
	}
	else
		NormalRotation = mat4(1.0f);

	mat4 FacingRotation = rotate(mat4(1.0f), atan2(Normal.y, Normal.x), Up);

	mat4 Rotation = NormalRotation * FacingRotation;

	mat4 Model = translate(mat4(1.0f), Position) * Rotation;
	mat4 FinalModel = Model * scale(mat4(1.0f), Size);

	glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
	glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
	glDrawArrays(GL_TRIANGLES, PlaneStart, PlaneSize);
}

void Render::DrawSphere(vec3 Position, mat4 Rotation, vec3 Size) {

	mat4 Model = translate(mat4(1.0f), Position) * Rotation;

	mat4 FinalModel = Model * scale(mat4(1.0f), Size);

	glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
	glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
	glDrawArrays(GL_TRIANGLES, SphereStart, SphereSize);
}

void Render::DrawLine(vec3 Start, vec3 End) {

	mat4 Translation = translate(mat4(1.0f), Start);

	vec3 Delta = End - Start;
	float Len = length(Delta);
	vec3 Normal = Delta / Len;

	mat4 Model = Translation;

	vec3 Axis = cross(Normal, Up);
	if (length(Axis) < 10e-10)
		Axis = { 1, 0, 0 };

	float Angle = -acos(dot(Normal, Up));
	mat4 NormalRotation = rotate(mat4(1.0f), Angle, Axis);

	Model = Model * NormalRotation;

	mat4 FinalModel = Model * scale(mat4(1.0f), vec3(Len));

	glUniformMatrix4fv(ModelNormalID, 1, GL_FALSE, value_ptr(Model));
	glUniformMatrix4fv(ModelID, 1, GL_FALSE, value_ptr(FinalModel));
	glDrawArrays(GL_LINES, LineStart, LineSize);
}

vec3 ShiftVector(vec3 v) {
	return vec3(v.z, v.x, v.y);
}

void Render::DrawGrid(vec3 Position, float Size, float Spacing) {

	const vec3 Normals[3] = { { 1, 0, 0 },{ 0, 1, 0 },{ 0, 0, 1 } };

	int LinesCount = (int)(Size / Spacing);

	for (vec3 Normal : Normals) {

		vec3 Up = ShiftVector(Normal);
		vec3 Side = ShiftVector(Up);

		for (int X = 0; X < LinesCount; X++) {

			float tx = (float)X / (LinesCount - 1) - 0.5f;
			vec3 UpPart = Up * vec3(tx * Size);

			for (int Y = 0; Y < LinesCount; Y++) {

				float ty = (float)Y / (LinesCount - 1) - 0.5f;

				vec3 SidePart = Side * vec3(ty * Size);

				vec3 v = SidePart + UpPart;

				DrawLine(v + Normal * (Size * 0.5f), v + Normal * (-Size * 0.5f));
			}
		}
	}
}

// OpenGL

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

	/*
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	*/

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

	// Load textures

	SamplerID = glGetUniformLocation(ShaderID, "TextureSampler");
	UseTextureID = glGetUniformLocation(ShaderID, "UseTexture");

	FloorTextureID = loadBMP("Floor.bmp");

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(SamplerID, 0);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

	glUniform3fv(LightAmbientColorID, 1, value_ptr(LightAmbientColor));
	glUniform3fv(LightDiffuseColorID, 1, value_ptr(LightDiffuseColor));
	glUniform1f(LightPowerID, LightPower);
}

// Matrices Calculations

void Render::UpdateViewMatrix(void)
{
	CameraAngleZ /= (float)M_PI * 2.0f;
	CameraAngleZ -= (float)(long)CameraAngleZ;
	CameraAngleZ *= (float)M_PI * 2.0f;

	CameraAngleX = std::min(std::max(0.1f, CameraAngleX), 3.13f);

	vec3 Direction = GetLookingDirection();

	View = lookAt(CameraPosition, CameraPosition + Direction, Up);

	glUniformMatrix4fv(ViewID, 1, GL_FALSE, value_ptr(View));

	glUniform3fv(LightPositionID, 1, value_ptr(CameraPosition));
	glUniform3fv(LightDirectionID, 1, value_ptr(Direction));
}