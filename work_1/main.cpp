#include <gl/glew.h>
#include <gl/glfw3.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

float rColor = 1.0f;
float gColor = 1.0f;
float bColor = 1.0f;
bool timerRunning = false;
double lastColorChangeTime = 0.0;

void SetRandomColor();
void InputProcess(GLFWwindow* window);
void DrawScene();

int main() {
	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}
	//--- OpenGL 버전 설정 (예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//--- 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}
	//--- 컨텍스트 설정
	glfwMakeContextCurrent(window);
	//--- GLEW 초기화e
	glewExperimental = GL_TRUE; // 최신 기능 사용
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}

	//--- 뷰포트 설정
	glViewport(0, 0, 800, 600);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {
		// 입력 처리
		InputProcess(window);
		// 화면 지우기 (파란색)
		DrawScene();
		// 버퍼 교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

//--- 키보드 입력 처리 함수
void InputProcess(GLFWwindow* window)
{
	static bool aKeyPressed = false;
	static bool tKeyPressed = false;
	static bool sKeyPressed = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
	{
		rColor = 0.0f;
		gColor = 1.0f;
		bColor = 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
	{
		rColor = 1.0f;
		gColor = 0.0f;
		bColor = 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		rColor = 1.0f;
		gColor = 1.0f;
		bColor = 0.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		rColor = 0.5f;
		gColor = 0.5f;
		bColor = 0.5f;
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
	{
		rColor = 0.0f;
		gColor = 0.0f;
		bColor = 0.0f;
	}

	bool aKeyDown = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	if (aKeyDown && !aKeyPressed)
		SetRandomColor();
	aKeyPressed = aKeyDown;

	bool tKeyDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
	if (tKeyDown && !tKeyPressed)
	{
		timerRunning = true;
		lastColorChangeTime = glfwGetTime();
	}
	tKeyPressed = tKeyDown;

	bool sKeyDown = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	if (sKeyDown && !sKeyPressed)
		timerRunning = false;
	sKeyPressed = sKeyDown;

	if (timerRunning)
	{
		double currentTime = glfwGetTime();
		if (currentTime - lastColorChangeTime >= 1.0)
		{
			SetRandomColor();
			lastColorChangeTime = currentTime;
		}
	}
}
//--- 렌더링 함수
void DrawScene()
{
	glClearColor(rColor, gColor, bColor, 1.0f); // RGBA (파랑)
	glClear(GL_COLOR_BUFFER_BIT);
}

void SetRandomColor()
{
	rColor = static_cast<float>(std::rand()) / RAND_MAX;
	gColor = static_cast<float>(std::rand()) / RAND_MAX;
	bColor = static_cast<float>(std::rand()) / RAND_MAX;
}
