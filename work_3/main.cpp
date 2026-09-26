#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

const int MAX_CREATE_RECT = 10; // A키로 생성할 수 있는 현재 개수 제한
const int MAX_RECT = 20;        // 분리를 포함한 전체 개수 제한

struct Rect {
	float x1, y1, x2, y2, r, g, b;
	bool selected = false;
};
int draggingIndex = -1;
float dragOffsetX = 0.0f;
float dragOffsetY = 0.0f;

vector<Rect> rects;

void InputProcess(GLFWwindow* window);
void DrawScene();
void DrawRect(const Rect& rect);
void CreateRandomRect();
void SelectRect(GLFWwindow* window);
void DragRect(GLFWwindow* window);
void StopDrag();
void MergeRects(float mouseX, float mouseY);
void SplitRect(GLFWwindow* window);

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
	static bool rightPressed = false;

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
		DragRect(window);
	}
	else {
		if (leftPressed) {
			StopDrag();
			leftPressed = false;
		}
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (!rightPressed) {
			SplitRect(window);
			rightPressed = true;
		}
	}
	else {
		rightPressed = false;
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
	if (rects.size() >= MAX_CREATE_RECT)
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

	draggingIndex = -1;

	for (Rect& rect : rects)
		rect.selected = false;

	for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
		if (real_x >= rects[i].x1 && real_x <= rects[i].x2 &&
			real_y >= rects[i].y1 && real_y <= rects[i].y2) {

			rects[i].selected = true;
			draggingIndex = i;

			dragOffsetX = real_x - rects[i].x1;
			dragOffsetY = real_y - rects[i].y1;

			break;
		}
	}
}

void DragRect(GLFWwindow* window)
{
	if (draggingIndex == -1)
		return;

	double x, y;
	glfwGetCursorPos(window, &x, &y);

	float real_x = static_cast<float>(x) / WIDTH * 2.0f - 1.0f;
	float real_y = 1.0f - static_cast<float>(y) / HEIGHT * 2.0f;

	Rect& rect = rects[draggingIndex];

	float width = rect.x2 - rect.x1;
	float height = rect.y2 - rect.y1;

	// 1. 마우스 위치에 따라 일단 이동
	rect.x1 = real_x - dragOffsetX;
	rect.y1 = real_y - dragOffsetY;

	rect.x2 = rect.x1 + width;
	rect.y2 = rect.y1 + height;


	// 2. 이동한 사각형이 화면 밖으로 나갔는지 검사

	// 왼쪽
	if (rect.x1 < -1.0f) {
		rect.x1 = -1.0f;
		rect.x2 = rect.x1 + width;
	}

	// 오른쪽
	if (rect.x2 > 1.0f) {
		rect.x2 = 1.0f;
		rect.x1 = rect.x2 - width;
	}

	// 아래
	if (rect.y1 < -1.0f) {
		rect.y1 = -1.0f;
		rect.y2 = rect.y1 + height;
	}

	// 위
	if (rect.y2 > 1.0f) {
		rect.y2 = 1.0f;
		rect.y1 = rect.y2 - height;
	}
	//MergeRects(real_x, real_y);

}

void StopDrag()
{
	if (draggingIndex != -1) {
		float mouseX = rects[draggingIndex].x1 + dragOffsetX;
		float mouseY = rects[draggingIndex].y1 + dragOffsetY;

		MergeRects(mouseX, mouseY);
	}
	draggingIndex = -1;
}

void SplitRect(GLFWwindow* window)
{
	// 하나가 둘이 되므로 전체 개수는 하나 증가
	if (rects.size() >= MAX_RECT)
		return;

	double x, y;
	glfwGetCursorPos(window, &x, &y);
	float real_x = static_cast<float>(x) / WIDTH * 2.0f - 1.0f;
	float real_y = 1.0f - static_cast<float>(y) / HEIGHT * 2.0f;

	// 화면에서 가장 위에 보이는 사각형부터 검사
	for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
		if (real_x < rects[i].x1 || real_x > rects[i].x2 ||
			real_y < rects[i].y1 || real_y > rects[i].y2)
			continue;

		// 삽입 시 vector의 저장 공간이 바뀔 수 있으므로 복사해서 사용
		Rect first = rects[i];
		Rect second = rects[i];
		float ratio = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.6f;

		bool splitLeftRight = rand() % 2 == 0;
		if (splitLeftRight) {
			float width = first.x2 - first.x1;
			// 약 8픽셀 간격. 작은 사각형은 너비의 10%로 제한
			float gap = (min)(16.0f / WIDTH, width * 0.1f);
			float splitX = first.x1 + (width - gap) * ratio;
			if (splitX <= first.x1 || splitX + gap >= first.x2)
				return;
			first.x2 = splitX;
			second.x1 = splitX + gap;
		}
		else {
			float height = first.y2 - first.y1;
			float gap = (min)(16.0f / HEIGHT, height * 0.1f);
			float splitY = first.y1 + (height - gap) * ratio;
			if (splitY <= first.y1 || splitY + gap >= first.y2)
				return;
			first.y2 = splitY;
			second.y1 = splitY + gap;
		}

		// 각 조각의 가로/세로를 분리 직후 크기의 50~150%로 변경
		// 맞닿는 쪽 경계를 유지해서 크기가 커져도 두 조각은 겹치지 않음
		for (int part = 0; part < 2; ++part) {
			Rect& piece = (part == 0) ? first : second;
			float width = (piece.x2 - piece.x1) *
				(0.5f + static_cast<float>(rand()) / RAND_MAX);
			float height = (piece.y2 - piece.y1) *
				(0.5f + static_cast<float>(rand()) / RAND_MAX);

			if (splitLeftRight) {
				if (part == 0)
					piece.x1 = (max)(-1.0f, piece.x2 - width);
				else
					piece.x2 = (min)(1.0f, piece.x1 + width);
				height = (min)(height, 2.0f);
				float centerY = (piece.y1 + piece.y2) * 0.5f;
				piece.y1 = (max)(-1.0f, (min)(centerY - height * 0.5f, 1.0f - height));
				piece.y2 = piece.y1 + height;
			}
			else {
				if (part == 0)
					piece.y1 = (max)(-1.0f, piece.y2 - height);
				else
					piece.y2 = (min)(1.0f, piece.y1 + height);
				width = (min)(width, 2.0f);
				float centerX = (piece.x1 + piece.x2) * 0.5f;
				piece.x1 = (max)(-1.0f, (min)(centerX - width * 0.5f, 1.0f - width));
				piece.x2 = piece.x1 + width;
			}
		}

		first.r = static_cast<float>(rand()) / RAND_MAX;
		first.g = static_cast<float>(rand()) / RAND_MAX;
		first.b = static_cast<float>(rand()) / RAND_MAX;
		second.r = static_cast<float>(rand()) / RAND_MAX;
		second.g = static_cast<float>(rand()) / RAND_MAX;
		second.b = static_cast<float>(rand()) / RAND_MAX;
		first.selected = false;
		second.selected = false;

		rects[i] = first;
		// 원래 레이어 위치에 두 조각을 나란히 저장
		rects.insert(rects.begin() + i + 1, second);

		if (draggingIndex == i)
			draggingIndex = -1;
		else if (draggingIndex > i)
			++draggingIndex;

		break;
	}
}

void MergeRects(float mouseX, float mouseY) {
	if (draggingIndex == -1)
		return;

	int i = 0;

	while (i < static_cast<int>(rects.size())) {
		// 자기 자신은 검사하지 않음
		if (i == draggingIndex) {
			++i;
			continue;
		}

		Rect& a = rects[draggingIndex];
		const Rect& b = rects[i];

		// x축과 y축 모두 겹치는지 검사
		bool overlap =
			a.x1 < b.x2 && a.x2 > b.x1 &&
			a.y1 < b.y2 && a.y2 > b.y1;

		if (!overlap) {
			++i;
			continue;
		}

		// 두 사각형을 포함하는 큰 사각형으로 변경
		a.x1 = (min)(a.x1, b.x1);
		a.y1 = (min)(a.y1, b.y1);
		a.x2 = (max)(a.x2, b.x2);
		a.y2 = (max)(a.y2, b.y2);

		// 색상을 랜덤하게 변경
		a.r = static_cast<float>(rand()) / RAND_MAX;
		a.g = static_cast<float>(rand()) / RAND_MAX;
		a.b = static_cast<float>(rand()) / RAND_MAX;

		// 합쳐진 상대 사각형 제거
		rects.erase(rects.begin() + i);

		// 삭제로 밀린 인덱스 보정
		if (i < draggingIndex)
			--draggingIndex;

		// 병합 후에도 자연스럽게 드래그하도록 간격 보정
		dragOffsetX = mouseX - rects[draggingIndex].x1;
		dragOffsetY = mouseY - rects[draggingIndex].y1;

		break;
		// 커진 사각형을 기준으로 다시 검사
		//i = 0;
	}
}
