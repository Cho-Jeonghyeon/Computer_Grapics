#include <gl/glew.h>
#include <gl/glfw3.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

struct Rectangle
{
	float x;
	float y;
	float halfSize;
	float r;
	float g;
	float b;
};

Rectangle rectangles[4][5];
int rectangleCount[4] = { 0, 0, 0, 0 };
float areaColors[4][3];
bool previousKeyState[GLFW_KEY_LAST + 1] = {};
int selectedArea = -1;
int selectedRectangle = -1;

float RandomFloat();
void SetRandomRectangleColor(Rectangle& rectangle);
void SetRandomAreaColors();
void CreateRectangle(int area);
bool KeyPressedOnce(GLFWwindow* window, int key);
void ChangeSelectedSize(float amount);
void InputProcess(GLFWwindow* window);
void DrawScene();
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

int main()
{
	//--- GLFW 초기화
	if (!glfwInit())
	{
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	//--- OpenGL 버전 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	//--- 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Practice 2", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}

	//--- 컨텍스트 설정
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	//--- GLEW 초기화
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	//--- 뷰포트와 콜백 설정
	glViewport(0, 0, 800, 600);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	SetRandomAreaColors();

	//--- 메인 루프
	while (!glfwWindowShouldClose(window))
	{
		InputProcess(window);
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

float RandomFloat()
{
	return static_cast<float>(std::rand()) / RAND_MAX;
}

void SetRandomRectangleColor(Rectangle& rectangle)
{
	rectangle.r = RandomFloat();
	rectangle.g = RandomFloat();
	rectangle.b = RandomFloat();
}

void SetRandomAreaColors()
{
	for (int i = 0; i < 4; ++i)
	{
		bool differentColor = false;

		while (!differentColor)
		{
			areaColors[i][0] = 0.2f + RandomFloat() * 0.6f;
			areaColors[i][1] = 0.2f + RandomFloat() * 0.6f;
			areaColors[i][2] = 0.2f + RandomFloat() * 0.6f;
			differentColor = true;

			for (int j = 0; j < i; ++j)
			{
				float difference = 0.0f;
				difference += std::abs(areaColors[i][0] - areaColors[j][0]);
				difference += std::abs(areaColors[i][1] - areaColors[j][1]);
				difference += std::abs(areaColors[i][2] - areaColors[j][2]);

				if (difference < 0.35f)
					differentColor = false;
			}
		}
	}
}

void CreateRectangle(int area)
{
	if (rectangleCount[area] >= 5)
		return;

	float centerX[4] = { -0.5f, 0.5f, -0.5f, 0.5f };
	float centerY[4] = { 0.5f, 0.5f, -0.5f, -0.5f };
	Rectangle& rectangle = rectangles[area][rectangleCount[area]];

	rectangle.x = centerX[area];
	rectangle.y = centerY[area];
	rectangle.halfSize = 0.10f + RandomFloat() * 0.25f;
	SetRandomRectangleColor(rectangle);
	++rectangleCount[area];
}

bool KeyPressedOnce(GLFWwindow* window, int key)
{
	bool keyDown = glfwGetKey(window, key) == GLFW_PRESS;
	bool pressedOnce = keyDown && !previousKeyState[key];
	previousKeyState[key] = keyDown;
	return pressedOnce;
}

void ChangeSelectedSize(float amount)
{
	if (selectedArea == -1 || selectedRectangle == -1)
		return;

	Rectangle& rectangle = rectangles[selectedArea][selectedRectangle];
	rectangle.halfSize += amount;

	if (rectangle.halfSize < 0.05f)
		rectangle.halfSize = 0.05f;
	if (rectangle.halfSize > 0.45f)
		rectangle.halfSize = 0.45f;
}

//--- 키보드 입력 처리 함수
void InputProcess(GLFWwindow* window)
{
	bool number1 = KeyPressedOnce(window, GLFW_KEY_1);
	bool number2 = KeyPressedOnce(window, GLFW_KEY_2);
	bool number3 = KeyPressedOnce(window, GLFW_KEY_3);
	bool number4 = KeyPressedOnce(window, GLFW_KEY_4);
	bool keypad1 = KeyPressedOnce(window, GLFW_KEY_KP_1);
	bool keypad2 = KeyPressedOnce(window, GLFW_KEY_KP_2);
	bool keypad3 = KeyPressedOnce(window, GLFW_KEY_KP_3);
	bool keypad4 = KeyPressedOnce(window, GLFW_KEY_KP_4);

	if (number1 || keypad1)
		CreateRectangle(0);
	if (number2 || keypad2)
		CreateRectangle(1);
	if (number3 || keypad3)
		CreateRectangle(2);
	if (number4 || keypad4)
		CreateRectangle(3);

	bool plusKey = KeyPressedOnce(window, GLFW_KEY_EQUAL);
	bool keypadPlus = KeyPressedOnce(window, GLFW_KEY_KP_ADD);
	bool minusKey = KeyPressedOnce(window, GLFW_KEY_MINUS);
	bool keypadMinus = KeyPressedOnce(window, GLFW_KEY_KP_SUBTRACT);

	if (plusKey || keypadPlus)
		ChangeSelectedSize(0.05f);
	if (minusKey || keypadMinus)
		ChangeSelectedSize(-0.05f);

	if (KeyPressedOnce(window, GLFW_KEY_C))
	{
		if (selectedArea != -1 && selectedRectangle != -1)
			SetRandomRectangleColor(rectangles[selectedArea][selectedRectangle]);
	}

	if (KeyPressedOnce(window, GLFW_KEY_R))
	{
		for (int i = 0; i < 4; ++i)
			rectangleCount[i] = 0;

		selectedArea = -1;
		selectedRectangle = -1;
		SetRandomAreaColors();
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

//--- 렌더링 함수
void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(areaColors[0][0], areaColors[0][1], areaColors[0][2]);
	glRectf(-1.0f, 0.0f, 0.0f, 1.0f);
	glColor3f(areaColors[1][0], areaColors[1][1], areaColors[1][2]);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glColor3f(areaColors[2][0], areaColors[2][1], areaColors[2][2]);
	glRectf(-1.0f, -1.0f, 0.0f, 0.0f);
	glColor3f(areaColors[3][0], areaColors[3][1], areaColors[3][2]);
	glRectf(0.0f, -1.0f, 1.0f, 0.0f);

	for (int area = 0; area < 4; ++area)
	{
		for (int i = 0; i < rectangleCount[area]; ++i)
		{
			Rectangle& rectangle = rectangles[area][i];

			if (area == selectedArea && i == selectedRectangle)
			{
				glColor3f(1.0f, 1.0f, 1.0f);
				glRectf(rectangle.x - rectangle.halfSize - 0.03f,
					rectangle.y - rectangle.halfSize - 0.03f,
					rectangle.x + rectangle.halfSize + 0.03f,
					rectangle.y + rectangle.halfSize + 0.03f);

				glColor3f(0.0f, 0.0f, 0.0f);
				glRectf(rectangle.x - rectangle.halfSize - 0.015f,
					rectangle.y - rectangle.halfSize - 0.015f,
					rectangle.x + rectangle.halfSize + 0.015f,
					rectangle.y + rectangle.halfSize + 0.015f);
			}

			glColor3f(rectangle.r, rectangle.g, rectangle.b);
			glRectf(rectangle.x - rectangle.halfSize,
				rectangle.y - rectangle.halfSize,
				rectangle.x + rectangle.halfSize,
				rectangle.y + rectangle.halfSize);
		}
	}
}

//--- 마우스 입력 처리 함수
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;

	double mouseX;
	double mouseY;
	int width;
	int height;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	glfwGetWindowSize(window, &width, &height);

	if (width <= 0 || height <= 0)
		return;

	float x = 2.0f * static_cast<float>(mouseX) / width - 1.0f;
	float y = 1.0f - 2.0f * static_cast<float>(mouseY) / height;
	selectedArea = -1;
	selectedRectangle = -1;

	for (int area = 3; area >= 0; --area)
	{
		for (int i = rectangleCount[area] - 1; i >= 0; --i)
		{
			Rectangle& rectangle = rectangles[area][i];

			if (x >= rectangle.x - rectangle.halfSize &&
				x <= rectangle.x + rectangle.halfSize &&
				y >= rectangle.y - rectangle.halfSize &&
				y <= rectangle.y + rectangle.halfSize)
			{
				selectedArea = area;
				selectedRectangle = i;
				return;
			}
		}
	}
}

//--- 프레임버퍼 크기 변경 함수
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
