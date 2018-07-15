#include "Form.hpp"

#include "shader.hpp"

// OpenGL

#define floatsizeofmember(s, m) (sizeof((((s*)0)->m)) / sizeof(GLfloat))

Form::Form(void){ 
}

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

	DrawCharacter(Char);
	DrawFloor();

	glFlush();
	SwapBuffers(WindowDC);
}

void Form::DrawCharacter(Character* Char) {

	for (Bone* Bone : Char->Bones) {

		mat4 SizeMatrix = scale(mat4(1.0f), Bone->Size);
		mat4 Model = Bone->WorldTransform * Bone->MiddleTranslation;

		mat4 FinalMatrix = Projection * View * Model * SizeMatrix;

		// setup matrix
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(FinalMatrix));
		glUniform1f(ColorIntencityID, 1.0f - min((float)Bone->Depth / 4.0f, 1.0f));
		// draw cube
		glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
	}
}

void Form::DrawFloor(void)
{
	mat4 Translation = translate(mat4(1.0f), FloorPosition);
	mat4 SizeMatrix = scale(mat4(1.0f), FloorSize);

	mat4 FinalMatrix = Projection * View * Translation * SizeMatrix;

	// setup matrix
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(FinalMatrix));
	glUniform1f(ColorIntencityID, 0.1f);
	// draw cube
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
}

// Integration

mat4 BulletToGLM(btTransform t) {

	btScalar matrix[16];

	t.getOpenGLMatrix(matrix);

	return mat4(
		matrix[0], matrix[1], matrix[2], matrix[3],
		matrix[4], matrix[5], matrix[6], matrix[7],
		matrix[8], matrix[9], matrix[10], matrix[11],
		matrix[12], matrix[13], matrix[14], matrix[15]);
}

btTransform GLMToBullet(mat4 m) {

	btScalar matrix[16];

	matrix[ 0] = m[0][0];
	matrix[ 1] = m[0][1];
	matrix[ 2] = m[0][2];
	matrix[ 3] = m[0][3];
	matrix[ 4] = m[1][0];
	matrix[ 5] = m[1][1];
	matrix[ 6] = m[1][2];
	matrix[ 7] = m[1][3];
	matrix[ 8] = m[2][0];
	matrix[ 9] = m[2][1];
	matrix[10] = m[2][2];
	matrix[11] = m[2][3];
	matrix[12] = m[3][0];
	matrix[13] = m[3][1];
	matrix[14] = m[3][2];
	matrix[15] = m[3][3];

	btTransform Result;
	Result.setFromOpenGLMatrix(matrix);

	return Result;
}

btVector3 GLMToBullet(vec3 v) {

	return btVector3(v.x, v.y, v.z);
}

static int Started;

void Form::Tick(double dt) {

	PhysicsTime += dt;
	if (Started || PhysicsTime >= 1.0) {
		
		if (!Started) {
			Started = true;
			PhysicsTime = 0.0;
		}

		uint64 StepCount = (uint64)(PhysicsTime * PHYSICS_FPS);
		for (; DoneStepCount < StepCount; DoneStepCount++) {

			double dt = 1.0 / (double)PHYSICS_FPS;

			// Char->Spine->PhysicBody->applyForce(btVector3(0, 0, 90000 * dt), btVector3(0, 0, 0));

			World->stepSimulation(dt, 0, dt);
		}
	}
	
	for (Bone* Bone : Char->Bones) {

		btTransform BulletTransfrom;
		Bone->PhysicBody->getMotionState()->getWorldTransform(BulletTransfrom);
		
		Bone->WorldTransform = BulletToGLM(BulletTransfrom) * inverse(Bone->MiddleTranslation);
	}

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

// Floor

void Form::CreateFloor(float FloorSize2D, float FloorHeight, float FloorZ)
{
	FloorPosition = vec3(0, 0, -FloorHeight * 0.5f + FloorZ);
	FloorSize = vec3(FloorSize2D, FloorSize2D, FloorHeight);
}

// Physics

struct YourOwnFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
	{
		btRigidBody* body0 = static_cast<btRigidBody*>(proxy0->m_clientObject);
		btRigidBody* body1 = static_cast<btRigidBody*>(proxy1->m_clientObject);

		Bone* Bone0 = (Bone*)body0->getUserPointer();
		Bone* Bone1 = (Bone*)body1->getUserPointer();

		if (Bone0 == nullptr || Bone1 == nullptr)
			return true;

		return true;
	}
};

void Form::SetupBulletWorld(void)
{
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	World = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	World->setGravity(btVector3(0, 0, -9.8));

	btOverlapFilterCallback* filterCallback = new YourOwnFilterCallback();
	World->getPairCache()->setOverlapFilterCallback(filterCallback);

	btContactSolverInfo& Solver = World->getSolverInfo();
	Solver.m_solverMode = SOLVER_SIMD;
	Solver.m_numIterations = 20;

	// creating physical bodies
	btRigidBody* Floor = AddStaticBox(translate(mat4(1.0f), FloorPosition), FloorSize);

	for (Bone* Bone : Char->Bones) {

		float Volume = Bone->Size.x * Bone->Size.y * Bone->Size.z;
		float Density = 1900;
		float Mass = Density * Volume;

		mat4 Transform = Bone->WorldTransform * Bone->MiddleTranslation;

		Bone->PhysicBody = AddDynamicBox(Transform, Bone->Size, Mass);
		Bone->PhysicBody->setUserPointer((void*)Bone);
	}

	for (Bone* Parent : Char->Bones) 
		for (Bone* Child : Parent->Childs) {

			vec4 Zero = vec4(0, 0, 0, 1);

			vec3 ChildHead = Child->WorldTransform * Zero;
			vec3 ChildPosition = Child->WorldTransform * Child->MiddleTranslation * Zero;
			vec3 ParentPosition = Parent->WorldTransform * Parent->MiddleTranslation * Zero;

			vec3 ChildLocalPoint = ChildHead - ChildPosition;
			vec3 ParentLocalPoint = ChildHead - ParentPosition;

			btVector3 BulletChildLocalPoint = GLMToBullet(ChildLocalPoint);
			btVector3 BulletParentLocalPoint = GLMToBullet(ParentLocalPoint);
			
			btPoint2PointConstraint* Constraint = 
				new btPoint2PointConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint);
		
			World->addConstraint(Constraint);
		}
}

btRigidBody* Form::AddDynamicBox(mat4 Transform, vec3 Size, float Mass)
{
	Size *= 0.5f;
	btCollisionShape* Shape = new btBoxShape(btVector3(Size.x, Size.y, Size.z));

	btTransform BulletTransform = GLMToBullet(Transform);

	bool IsDynamic = (Mass > 0);

	btVector3 LocalInertia(0, 0, 0);
	if (IsDynamic)
		Shape->calculateLocalInertia(Mass, LocalInertia);

	btDefaultMotionState* MotionState = new btDefaultMotionState(BulletTransform);
	btRigidBody::btRigidBodyConstructionInfo BodyDef(Mass, MotionState, Shape, LocalInertia);
	btRigidBody* Body = new btRigidBody(BodyDef);

	float MinSize = min(min(Size.x, Size.y), Size.z);
	float MaxSize = max(max(Size.x, Size.y), Size.z);
	Body->setCcdMotionThreshold(MinSize * 0.1);
	Body->setCcdSweptSphereRadius(MinSize * 0.1f);
	Body->setFriction(1.0);

	World->addRigidBody(Body);

	return Body;
}

btRigidBody* Form::AddStaticBox(mat4 Transform, vec3 Size)
{
	return AddDynamicBox(Transform, Size, 0.0f);
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

		Char = new Character();

		CreateFloor(4.0f, 100.0f, Char->FloorZ);

		SetupBulletWorld();

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