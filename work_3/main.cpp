#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;


const int MAX_RECT = 10;

struct Rect
{
	float x1;
	float y1;
	float x2;
	float y2;

	float r;
	float g;
	float b;
};

vector<Rect> rects;

void InputProcess(GLFWwindow* window);
void DrawScene();
void DrawRect(const Rect& rect);
void CreateRandomRect();

int main() {
	
	if (!glfwInit()) {
		cerr << "glewInit error!" << endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		cerr << "failed create window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		cerr << "failed Init GLEW" << endl;
		return -1;
	}

	glViewport(0, 0, 1920, 1080);

	srand(static_cast<unsigned int>(time(nullptr)));

	while (!glfwWindowShouldClose(window)) {
		
		InputProcess(window);

		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void InputProcess(GLFWwindow* window)
{
	static bool aPressed = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		if (!aPressed) {
			CreateRandomRect();
			aPressed = true;
		}
	}
	else {
		aPressed = false;
	}
		
}
void DrawRect(const Rect& rect) {
	glColor3f(rect.r, rect.g, rect.b);
	glRectf(rect.x1, rect.y1, rect.x2, rect.y2);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // RGBA (파랑)
	glClear(GL_COLOR_BUFFER_BIT);

	for (const Rect& rect : rects) {
		DrawRect(rect);
	}
}

void CreateRandomRect() {
	if (rects.size() >= MAX_RECT)
		return;
	Rect rect;
	float w_scale = 0.4f + static_cast<float>(rand()) / RAND_MAX;
	float h_scale = 0.4f + static_cast<float>(rand()) / RAND_MAX;
	float width = 0.3f * w_scale;
	float height = 0.3f * h_scale;

	rect.x1 = -1.0f
		+ static_cast<float>(rand()) / RAND_MAX * (2.0f - width);

	rect.y1 = -1.0f
		+ static_cast<float>(rand()) / RAND_MAX * (2.0f - height);

	rect.x2 = rect.x1 + width;
	rect.y2 = rect.y1 + height;

	rect.r = static_cast<float>(rand()) / RAND_MAX;
	rect.g = static_cast<float>(rand()) / RAND_MAX;
	rect.b = static_cast<float>(rand()) / RAND_MAX;

	rects.push_back(rect);
}