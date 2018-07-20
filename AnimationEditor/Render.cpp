#include "Render.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/gl.h>			
#include <GL/glu.h>	

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PhysicsManager.hpp"
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

	// Create and compile our GLSL program from the shaders
	ShaderID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(ShaderID, "MVP");
	ColorIntencityID = glGetUniformLocation(ShaderID, "ColorIntencity");

	Projection = perspective(radians(45.0f), AspectRatio, 0.1f, 100.0f);

	// Camera matrix

	CameraPosition = { 2.5f, -2.5, 1.264f };
	LookAtPoint({ 0, 0, 0.264f });
	UpdateViewMatrix();

	GenerateCube(Buffer);

	glGenBuffers(1, &BufferName);
	glBindBuffer(GL_ARRAY_BUFFER, BufferName);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Buffer), Buffer, GL_DYNAMIC_DRAW);

	glClearColor(1, 1, 1, 1);

	// Use our shader
	glUseProgram(ShaderID);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, floatsizeofmember(Vertex, Position), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, floatsizeofmember(Vertex, Normal), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, floatsizeofmember(Vertex, Color), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Color));
}

void Render::DrawScene(void) {

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Character* Char = CharacterManager::GetInstance().GetCharacter();
	DrawCharacter(Char);

	DrawFloor();

	glFlush();
	SwapBuffers(WindowDC);
}

void Render::DrawCharacter(Character* Char) {

	for (Bone* Bone : Char->Bones) {

		mat4 SizeMatrix = scale(mat4(1.0f), Bone->Size);
		mat4 Model = Bone->WorldTransform * Bone->MiddleTranslation;

		mat4 FinalMatrix = Projection * View * Model * SizeMatrix;

		// setup matrix
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(FinalMatrix));
		glUniform1f(ColorIntencityID, 1.0f - min((float)Bone->Depth / 6.0f, 1.0f));
		// draw cube
		glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
	}
}

void Render::DrawFloor(void)
{
	mat4 Translation = translate(mat4(1.0f), PhysicsManager::GetInstance().GetFloorPosition());
	mat4 SizeMatrix = scale(mat4(1.0f), PhysicsManager::GetInstance().GetFloorSize());

	mat4 FinalMatrix = Projection * View * Translation * SizeMatrix;

	// setup matrix
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(FinalMatrix));
	glUniform1f(ColorIntencityID, 0.1f);
	// draw cube
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
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

void Render::LookAtPoint(vec3 Point)
{
	vec3 Direction = normalize(Point - CameraPosition);

	float SinX = sqrt(1 - Direction.z * Direction.z);

	CameraAngleZ = atan2(Direction.x / SinX, Direction.y / SinX);
	CameraAngleX = atan2(SinX, Direction.z);

	UpdateViewMatrix();
}

vec3 Render::GetLookingDirection(void)
{
	vec3 Direction;
	Direction.x = sinf(CameraAngleX) * sinf(CameraAngleZ);
	Direction.y = sinf(CameraAngleX) * cosf(CameraAngleZ);
	Direction.z = cosf(CameraAngleX);

	return Direction;
}

void Render::GetBoneFromScreenPoint(LONG x, LONG y, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal)
{
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

	vec3 Direction = normalize(RayEndWorld - RayStartWorld);

	return PhysicsManager::GetInstance().GetBoneFromRay(RayStartWorld, Direction, TouchedBone, WorldPoint, WorldNormal);
}

// Model Generation

void Render::GenerateCubeQuad(Vertex* QuadMesh, vec3 normal, vec3 color, uint32 id) {

	vec3 tangent = cross(normal, { 1, 1, 1 });

	vec3 quad_verticies[4];

	for (int i = 0; i < 4; i++) {

		tangent = cross(normal, tangent);

		quad_verticies[i] = (tangent + normal) * 0.5f;
	}

	for (int i = 0; i < 6; i++) {
		QuadMesh[i].Normal = normal;
		QuadMesh[i].Color = color;
	}

	QuadMesh[0].Position = quad_verticies[0];
	QuadMesh[1].Position = quad_verticies[1];
	QuadMesh[2].Position = quad_verticies[2];

	QuadMesh[3].Position = quad_verticies[2];
	QuadMesh[4].Position = quad_verticies[3];
	QuadMesh[5].Position = quad_verticies[0];
}

void Render::GenerateCube(Vertex* CubeMesh) {

	uint32 id = 0;

	GenerateCubeQuad(CubeMesh, { 1,  0,  0 }, { 1, 0, 0 }, id++);
	CubeMesh += 2 * 3;

	GenerateCubeQuad(CubeMesh, { -1,  0,  0 }, { 0, 1, 0 }, id++);
	CubeMesh += 2 * 3;

	GenerateCubeQuad(CubeMesh, { 0,  1,  0 }, { 0, 0, 1 }, id++);
	CubeMesh += 2 * 3;

	GenerateCubeQuad(CubeMesh, { 0, -1,  0 }, { 1, 0, 1 }, id++);
	CubeMesh += 2 * 3;

	GenerateCubeQuad(CubeMesh, { 0,  0,  1 }, { 0, 1, 1 }, id++);
	CubeMesh += 2 * 3;

	GenerateCubeQuad(CubeMesh, { 0,  0, -1 }, { 1, 1, 0 }, id++);
	CubeMesh += 2 * 3;
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
}