#include "Form.hpp"

#include "shader.hpp"

// OpenGL

#define floatsizeofmember(s, m) (sizeof((((s*)0)->m)) / sizeof(GLfloat))

void Form::SetupOpenGL(void) {

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
	ShaderID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(ShaderID, "MVP");
	ColorIntencityID = glGetUniformLocation(ShaderID, "ColorIntencity");

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	Projection = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	View = lookAt(
		vec3(2.5f, -2.5, 1.264f),
		vec3(0, 0, 0.264f), // and looks at the origin
		vec3(0, 0, 1)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	Vertex cube_mesh[6 * 2 * 3];
	GenerateCube(cube_mesh);

	glGenBuffers(1, &BufferName);
	glBindBuffer(GL_ARRAY_BUFFER, BufferName);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_mesh), cube_mesh, GL_STATIC_DRAW);

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

void Form::DrawScene(void) {

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Angle += (float)(radians(60.0f) * StepTime);
 	// Char->Spine->Rotation = rotate(mat4(1.0f), Angle, vec3(0, -1, 0));
	// Char->Spine->Childs[0]->Childs[0]->Rotation = rotate(mat4(1.0f), -Angle, vec3(0, -1, 0));

	DrawCharacter(Char);
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3); // 12*3 indices starting at 0 -> 12 triangles

	glFlush();
	SwapBuffers(WindowDC);
}

void Form::DrawBone(Bone* Bone, mat4 ParentModel, vec3 ParentSize, uint32 Depth) {

	mat4 SizeMatrix = scale(mat4(1.0f), Bone->Size);

	mat4 MiddleTranslation = translate(mat4(1.0f), Bone->Tail * Bone->Size * 0.5f);

	mat4 OffsetTranslation = translate(mat4(1.0f), Bone->Offset * ParentSize);

	mat4 Model = ParentModel * OffsetTranslation * Bone->Rotation;
	mat4 FinalMatrix = Projection * View * Model * MiddleTranslation * SizeMatrix;

	// setup matrix
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &FinalMatrix[0][0]);
	glUniform1f(ColorIntencityID, 1.0f - min((float)Depth / 4.0f, 1.0f));
	// draw cube
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

	for (Form::Bone* Child : Bone->Childs)
		DrawBone(Child, Model, Bone->Size, Depth + 1);
}

void Form::DrawCharacter(Character* Char) {

	Bone* Bone = Char->Spine;
	mat4 ParentModel = translate(mat4(1.0f), Char->Position);

	DrawBone(Bone, ParentModel, {}, 0);
}

// Integration

void Form::Tick(double dt) {

	StepTime = dt;
	RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);
	StepTime = 0;
}

// Model generation

void Form::GenerateCubeQuad(Vertex* QuadMesh, vec3 normal, vec3 color, uint32 id) {

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

void Form::GenerateCube(Vertex* CubeMesh) {

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

// Character generation

Form::Bone* Form::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name)
{
	Bone* Result = new Bone();

	// setup linked list
	Result->Parent = Parent;
	if (Parent != nullptr)
		Parent->Childs.push_back(Result);

	vec3 CmToMeters = vec3(0.01f, 0.01f, 0.01f);

	Result->Name = Name;
	Result->Tail = Tail;
	Result->Offset = Offset;
	Result->Size = Size * CmToMeters;
	Result->Rotation = mat4(1.0f);

	return Result;
}

void Form::GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection)
{
	vec3 MirrorVector = cross(MirrorDirection, cross(MirrorDirection, vec3(-1.0f))) - MirrorDirection;

	Bone* RightBone = new Bone();
	RightBone->Name = LeftBone->Name + L" Right";

	RightBone->Offset = LeftBone->Offset * MirrorVector;
	RightBone->Tail = LeftBone->Tail * MirrorVector;
	RightBone->Size = LeftBone->Size;
	RightBone->Rotation = LeftBone->Rotation;
	RightBone->Parent = RightParent;
	if (RightParent != nullptr)
		RightParent->Childs.push_back(RightBone);

	LeftBone->Name = LeftBone->Name + L" Left";

	for (Bone* LeftChild : LeftBone->Childs)
		GenerateRightSide(LeftChild, RightBone, MirrorDirection);
}

Form::Character* Form::GenerateCharacter(void)
{
	Character* Char = new Character();

	Bone* Spine = GenerateBone(nullptr, { 0, 0, 1.0f }, { 6.5f, 13.0f, 52.8f }, {}, L"Spine");

	Bone* UpperLeg = GenerateBone(Spine, { 0, 0, -1.0f }, { 6.5f, 6.5f, 46.0f }, { 0, 0.5f, 0 }, L"Upper Leg");
	Bone* LowerLeg = GenerateBone(UpperLeg, { 0, 0, -1.0f }, { 6.49f, 6.49f, 45.0f }, { 0, 0, -1 }, L"Lower Leg");
	Bone* Foot = GenerateBone(LowerLeg, { 15.5f / 22.0f, 0, 0 }, { 22.0f, 8.0f, 3.0f }, { 0, 0, -1.175f }, L"Foot");

	Bone* UpperArm = GenerateBone(Spine, { 0, 1, 0, }, { 4.5f, 32.0f, 4.5f }, { 0, 0.5f, 1.0f }, L"Upper Arm");
	Bone* LowerArm = GenerateBone(UpperArm, { 0, 1, 0, }, { 4.49f, 28.0f, 4.49f }, { 0, 1.0f, 0.0f }, L"Lower Arm");
	Bone* Hand = GenerateBone(LowerArm, { 0, 1, 0, }, { 3.5f, 15.0f, 1.5f }, { 0, 1.0f, 0.0f }, L"Hand");

	Bone* Head = GenerateBone(Spine, { 0, 0, 1, }, { 15.0f, 15.0f, 20.0f }, { 0, 0.0f, 57.8f / 52.8f }, L"Head");

	GenerateRightSide(UpperLeg, UpperLeg->Parent, { 0, 1, 0 });
	GenerateRightSide(UpperArm, UpperArm->Parent, { 0, 1, 0 });

	Char->Spine = Spine;

	return Char;
}

// Controls

void Form::ProcessMouseInput(LONG dx, LONG dy) {

}

// Window handling

void Form::CreateMainWindow(HINSTANCE hInstance) {

	const WCHAR* WindowClass = L"OpenGLTest";
	const WCHAR* Title = L"OpenGL Test Box";
	const int Width = 1280;
	const int Height = 960;

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProcStaticCallback;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = WindowClass;

	RegisterClassExW(&wcex);

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

LRESULT CALLBACK Form::WndProcStaticCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	return Form::GetInstance().WndProcCallback(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Form::WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_CREATE: {

		WindowHandle = hWnd;

		SetupOpenGL();

		Char = GenerateCharacter();

		break;
	}
	case WM_SIZE: {

		int Width = LOWORD(lParam);
		int Height = HIWORD(lParam);

		glViewport(0, 0, Width, Height);

		RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);

		break;
	}
	case WM_PAINT: {

		DrawScene();

		static PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);

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