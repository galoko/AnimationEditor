#include "Form.hpp"

#include "shader.hpp"

// OpenGL

#define floatsizeofmember(s, m) (sizeof((((s*)0)->m)) / sizeof(GLfloat))

#define SOLID_ID ((void*)1)

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
	Projection = perspective(radians(45.0f), AspectRatio, 0.1f, 100.0f);
	// Camera matrix
	View = lookAt(
		vec3(2.5f, -2.5, 1.264f),
		vec3(0, 0, 0.264f), // and looks at the origin
		vec3(0, 0, 1)  // Bone is up (set to 0,-1,0 to look upside-down)
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
	if (FloorSize.length() > 0)
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
		glUniform1f(ColorIntencityID, 1.0f - min((float)Bone->Depth / 6.0f, 1.0f));
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

vec3 BulletToGLM(btVector3 v) {

	return vec3(v.getX(), v.getY(), v.getZ());
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

void Form::Tick(double dt) {

	PhysicsTime += dt;

	uint64 StepCount = (uint64)(PhysicsTime * PHYSICS_FPS);
	for (; DoneStepCount < StepCount; DoneStepCount++) {

		const double dt = 1.0 / (double)PHYSICS_FPS;

		World->stepSimulation(dt, 0, dt);
	}
	
	// apply changes to bones
	for (Bone* Bone : Char->Bones) {

		btTransform BulletTransfrom;
		Bone->PhysicBody->getMotionState()->getWorldTransform(BulletTransfrom);
		
		Bone->WorldTransform = BulletToGLM(BulletTransfrom) * inverse(Bone->MiddleTranslation);
	}

	Char->UpdateRotationsFromWorldTransforms();
	Char->UpdateWorldTranforms();

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
		btCollisionObject* obj0 = static_cast<btCollisionObject*>(proxy0->m_clientObject);
		btCollisionObject* obj1 = static_cast<btCollisionObject*>(proxy1->m_clientObject);

		Bone* Bone0 = (Bone*)obj0->getUserPointer();
		Bone* Bone1 = (Bone*)obj1->getUserPointer();

		// some wierd shit
		if (Bone0 == nullptr || Bone1 == nullptr)
			return false;

		// either one is solid
		if (Bone0 == SOLID_ID || Bone1 == SOLID_ID)
			return true;

		// if we have parent->child collision - filter it out
		if (Bone0->Parent == Bone1 || Bone1->Parent == Bone0)
			return false;

		// non parent bones = collision
		return true;
	}
};

void Switch(float* a, float* b) {

	float temp = *a;
	*a = *b;
	*b = temp;
}

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
	// Solver.m_solverMode = SOLVER_SIMD;
	Solver.m_numIterations = 30;

	// creating physical bodies
	if (FloorSize.length() > 0) {
		btRigidBody* Floor = AddStaticBox(translate(mat4(1.0f), FloorPosition), FloorSize);
		Floor->setUserPointer(SOLID_ID);
	}

	for (Bone* Bone : Char->Bones) {

		float Volume = Bone->Size.x * Bone->Size.y * Bone->Size.z;
		float Density = 1900;
		float Mass = Density * Volume;

		if (Bone->IsLocked)
			Mass = 0.0f;

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

			btTransform BulletChildFrame;
			BulletChildFrame.setIdentity();
			BulletChildFrame.setOrigin(BulletChildLocalPoint);

			btTransform BulletParentFrame;
			BulletParentFrame.setIdentity();
			BulletParentFrame.setOrigin(BulletParentLocalPoint);
			
			Parent->PhysicBody->setActivationState(DISABLE_DEACTIVATION);
			Child->PhysicBody->setActivationState(DISABLE_DEACTIVATION);

			vec3 LowLimit = Child->LowLimit;
			vec3 HighLimit = Child->HighLimit;

			if (Child->IsFixed()) {

				btFixedConstraint* Constraint = new btFixedConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
				World->addConstraint(Constraint, true);
			}
			else
			if (Child->IsOnlyXRotation())
			{
				btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
					btVector3(1, 0, 0), btVector3(1, 0, 0));
				Constraint->setLimit(LowLimit.x, HighLimit.x);
				World->addConstraint(Constraint, true);
			}
			else
			if (Child->IsOnlyYRotation())
			{
				btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
					btVector3(0, 1, 0), btVector3(0, 1, 0));
				Constraint->setLimit(LowLimit.y, HighLimit.y);
				World->addConstraint(Constraint, true);
			}
			else
			if (Child->IsOnlyZRotation())
			{
				btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
					btVector3(0, 0, 1), btVector3(0, 0, 1));
				Constraint->setLimit(LowLimit.z, HighLimit.z);
				World->addConstraint(Constraint, true);
			}
			else
			// generic 6DOF 
			{
				// gimbal lock fix

				vec3 Ranges = vec3(
						max(fabs(LowLimit.x), fabs(HighLimit.x)),
						max(fabs(LowLimit.y), fabs(HighLimit.y)),
						max(fabs(LowLimit.z), fabs(HighLimit.z))
				);

				float MinRange = min(min(Ranges.x, Ranges.y), Ranges.z);
				float MaxRange = max(max(Ranges.x, Ranges.y), Ranges.z);

				if (MinRange > 80.0f)
					throw new runtime_error("Too large angle range");

				// switch Y -> X
				if (MinRange == Ranges.x) {

					BulletParentFrame.getBasis().setEulerZYX(0, 0, M_PI_2);
					BulletChildFrame.getBasis().setEulerZYX(0, 0, M_PI_2);

					Switch(&LowLimit.x, &LowLimit.y);
					Switch(&HighLimit.x, &HighLimit.y);

					Switch(&LowLimit.y, &HighLimit.y);
					LowLimit.y = -LowLimit.y;
					HighLimit.y = -HighLimit.y;
				}
				else
				// switch Y -> Z
				if (MinRange == Ranges.z) {

					BulletParentFrame.getBasis().setEulerZYX(-M_PI_2, 0, 0);
					BulletChildFrame.getBasis().setEulerZYX(-M_PI_2, 0, 0);

					Switch(&LowLimit.z, &LowLimit.y);
					Switch(&HighLimit.z, &HighLimit.y);

					Switch(&LowLimit.y, &HighLimit.y);
					LowLimit.y = -LowLimit.y;
					HighLimit.y = -HighLimit.y;
				}
				else
					assert(Ranges.y == MinRange);

				btGeneric6DofSpring2Constraint* Constraint =
					new btGeneric6DofSpring2Constraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
				Constraint->setLinearLowerLimit(btVector3(0, 0, 0));
				Constraint->setLinearUpperLimit(btVector3(0, 0, 0));

				Constraint->setAngularLowerLimit(btVector3(LowLimit.x, LowLimit.y, LowLimit.z));
				Constraint->setAngularUpperLimit(btVector3(HighLimit.x, HighLimit.y, HighLimit.z));

				for (int i = 0; i < 6; i++)
					Constraint->setStiffness(i, 0);
				World->addConstraint(Constraint, true);
			}

			// btPoint2PointConstraint* Constraint = new btPoint2PointConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint);

			/*
			btGeneric6DofSpring2Constraint* Constraint =
				new btGeneric6DofSpring2Constraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
			Constraint->setLinearLowerLimit(btVector3(0, 0, 0));
			Constraint->setLinearUpperLimit(btVector3(0, 0, 0));
			Constraint->setAngularLowerLimit(btVector3(0, 0, -M_PI * 0.5));
			Constraint->setAngularUpperLimit(btVector3(0, 0, M_PI * 0.5));
			for (int i = 0; i < 6; i++)
				Constraint->setStiffness(i, 0);
			*/
			
			/*
			btConeTwistConstraint* Constraint =
				new btConeTwistConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
			Constraint->setLimit(M_PI * 0.1, M_PI * 0.5, 0);
			*/

			/*
			btHingeConstraint* Constraint =
				new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame, false);
			btVector3 Axis = btVector3(0.0, 1.0, 0.0);
			Constraint->setAxis(Axis);
			Constraint->setLimit(-M_PI * 0.5, M_PI * 0.5);
			*/
		
			// World->addConstraint(Constraint, true);
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
	// Body->setCcdMotionThreshold(MinSize * 0.1);
	// Body->setCcdSweptSphereRadius(MinSize * 0.1f);
	Body->setFriction(1.0);
	Body->setRestitution(0.0);

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

HWND Form::CreateMainWindow(HINSTANCE hInstance) {

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

	return WindowHandle;
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
		Char->FindBone(L"Hand Left")->IsLocked = true;
		// Char->FindBone(L"Hand Right")->IsLocked = true;
		// Char->FindBone(L"Bone")->IsLocked = true;
		// const wstring UnlockedBones[] = { L"Stomach" };
		// Char->LockEverythingBut(UnlockedBones);
		/*
		Bone* Bone = Char->FindBone(L"Lower Leg Left");
		Bone->Rotation = rotate(mat4(1.0f), Bone->LowLimit.y, vec3(0, -1, 0));
		Char->UpdateWorldTranforms();
		*/

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