#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

const int MAX_RECT = 10;

struct Rect {
	float x1, y1, x2, y2, r, g, b;
	bool selected = false;
};

vector<Rect> rects;

void InputProcess(GLFWwindow* window);
void DrawScene();
void DrawRect(const Rect& rect);
void CreateRandomRect();
void SelectRect(GLFWwindow* window);

int main() {
	
	if (!glfwInit()) {
		cerr << "glewInit error!" << endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Window", nullptr, nullptr);
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

	glViewport(0, 0, WIDTH, HEIGHT);

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
	static bool leftPressed = false;

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

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (!leftPressed) {
			SelectRect(window);
			leftPressed = true;
		}
	}
	else {
		leftPressed = false;
	}
}

void DrawScene() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	for (const Rect& rect : rects) {
		DrawRect(rect);
	}
}

void DrawRect(const Rect& rect) {
	glColor3f(rect.r, rect.g, rect.b);
	glRectf(rect.x1, rect.y1, rect.x2, rect.y2);
	if (rect.selected)
	{
		glColor3f(0.0f, 0.0f, 0.0f);
		glLineWidth(4.0f);

		glBegin(GL_LINE_LOOP);

		glVertex2f(rect.x1, rect.y1);
		glVertex2f(rect.x2, rect.y1);
		glVertex2f(rect.x2, rect.y2);
		glVertex2f(rect.x1, rect.y2);

		glEnd();
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

void SelectRect(GLFWwindow* window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	float real_x = x / WIDTH * 2.0 - 1.0;
	float real_y = 1.0 - y / HEIGHT * 2.0;

	for (Rect& rect : rects)
		rect.selected = false;

	for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
		if (real_x >= rects[i].x1 && real_x <= rects[i].x2 &&
			real_y >= rects[i].y1 && real_y <= rects[i].y2) {

			rects[i].selected = true;
			break;
		}
	}
}