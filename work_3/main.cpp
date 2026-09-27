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

// A키로 직접 만드는 사각형은 최대 10개, 분리된 조각까지 합치면 최대 20개이다.
const int MAX_CREATE_RECT = 10;
const int MAX_RECT = 20;

struct Rect {
	float x1, y1, x2, y2; // 왼쪽 아래 좌표와 오른쪽 위 좌표
	float r, g, b;        // 사각형의 RGB 색상
	bool selected = false; // 선택된 사각형은 검은 테두리로 표시한다.
};
// -1이면 드래그 중인 사각형이 없다는 뜻이다.
int draggingIndex = -1;
// 클릭한 곳과 사각형 왼쪽 아래 사이의 간격을 저장해서 드래그할 때 사용한다.
float dragOffsetX = 0.0f;
float dragOffsetY = 0.0f;

// 뒤에 추가된 사각형일수록 나중에 그려지므로 화면의 위쪽에 보인다.
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
	//--- GLFW 초기화 및 OpenGL 창 만들기
	if (!glfwInit()) {
		cerr << "glewInit error!" << endl;
		return -1;
	}

	// glRectf와 glBegin을 사용하기 위해 호환 프로파일로 설정한다.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		cerr << "failed create window" << endl;
		glfwTerminate();
		return -1;
	}
	// 생성한 창을 현재 OpenGL 작업 대상으로 지정한 뒤 GLEW를 초기화한다.
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		cerr << "failed Init GLEW" << endl;
		return -1;
	}

	// 창 전체를 OpenGL이 그림을 그릴 영역으로 사용한다.
	glViewport(0, 0, WIDTH, HEIGHT);

	// 실행할 때마다 사각형의 위치, 크기, 색상이 달라지게 한다.
	srand(static_cast<unsigned int>(time(nullptr)));

	//--- 메인 루프: 입력 처리 -> 그리기 -> 화면 출력 -> 이벤트 갱신
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
	// 키나 마우스를 계속 누르고 있어도 한 번만 동작시키기 위한 이전 상태이다.
	static bool aPressed = false;
	static bool leftPressed = false;
	static bool rightPressed = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// A키를 새로 누른 순간에만 랜덤 사각형 한 개를 만든다.
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		if (!aPressed) {
			CreateRandomRect();
			aPressed = true;
		}
	}
	else {
		aPressed = false;
	}

	// 왼쪽 버튼을 처음 누를 때 선택하고, 누르고 있는 동안 계속 위치를 옮긴다.
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (!leftPressed) {
			SelectRect(window);
			leftPressed = true;
		}
		DragRect(window);
	}
	else {
		// 버튼을 놓는 순간 드래그를 끝내고 겹친 사각형이 있는지 검사한다.
		if (leftPressed) {
			StopDrag();
			leftPressed = false;
		}
	}

	// 오른쪽 버튼은 새로 누른 순간에만 선택한 사각형을 둘로 나눈다.
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
	// vector의 앞에서부터 그려서 나중에 만든 사각형이 가장 위에 보이게 한다.
	for (const Rect& rect : rects) {
		DrawRect(rect);
	}
}

void DrawRect(const Rect& rect) {
	// 먼저 사각형의 안쪽을 저장된 색으로 채운다.
	glColor3f(rect.r, rect.g, rect.b);
	glRectf(rect.x1, rect.y1, rect.x2, rect.y2);
	// 선택된 사각형은 알아보기 쉽도록 굵은 검은색 테두리를 덧그린다.
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
	// A키로 만드는 사각형이 이미 10개 이상이면 더 만들지 않는다.
	if (rects.size() >= MAX_CREATE_RECT)
		return;
	Rect rect;
	// 기본 크기에 0.4~1.4 사이의 값을 곱해서 가로와 세로를 다르게 만든다.
	float w_scale = 0.4f + static_cast<float>(rand()) / RAND_MAX;
	float h_scale = 0.4f + static_cast<float>(rand()) / RAND_MAX;
	float width = 0.3f * w_scale;
	float height = 0.3f * h_scale;

	// 사각형 전체가 NDC 화면 범위인 -1~1 안에 들어오는 위치를 정한다.
	rect.x1 = -1.0f
		+ static_cast<float>(rand()) / RAND_MAX * (2.0f - width);

	rect.y1 = -1.0f
		+ static_cast<float>(rand()) / RAND_MAX * (2.0f - height);

	rect.x2 = rect.x1 + width;
	rect.y2 = rect.y1 + height;

	// OpenGL 색상 범위인 0.0~1.0 사이에서 RGB 값을 각각 뽑는다.
	rect.r = static_cast<float>(rand()) / RAND_MAX;
	rect.g = static_cast<float>(rand()) / RAND_MAX;
	rect.b = static_cast<float>(rand()) / RAND_MAX;

	rects.push_back(rect);
}

void SelectRect(GLFWwindow* window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	// 마우스의 픽셀 좌표를 OpenGL 좌표인 -1~1 범위로 바꾼다.
	// 마우스 y축은 아래 방향이고 OpenGL y축은 위 방향이므로 y값은 뒤집는다.
	float real_x = x / WIDTH * 2.0 - 1.0;
	float real_y = 1.0 - y / HEIGHT * 2.0;

	draggingIndex = -1;

	// 새로운 사각형을 고르기 전에 이전 선택 표시를 모두 지운다.
	for (Rect& rect : rects)
		rect.selected = false;

	// 뒤에서부터 검사하면 겹쳐 있을 때 화면에서 가장 위에 보이는 사각형이 선택된다.
	for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
		if (real_x >= rects[i].x1 && real_x <= rects[i].x2 &&
			real_y >= rects[i].y1 && real_y <= rects[i].y2) {

			rects[i].selected = true;
			draggingIndex = i;

			// 사각형이 클릭한 지점에 갑자기 붙지 않도록 클릭한 간격을 기억한다.
			dragOffsetX = real_x - rects[i].x1;
			dragOffsetY = real_y - rects[i].y1;

			break;
		}
	}
}

void DragRect(GLFWwindow* window)
{
	// 선택한 사각형이 없으면 이동시킬 대상도 없다.
	if (draggingIndex == -1)
		return;

	double x, y;
	glfwGetCursorPos(window, &x, &y);

	float real_x = static_cast<float>(x) / WIDTH * 2.0f - 1.0f;
	float real_y = 1.0f - static_cast<float>(y) / HEIGHT * 2.0f;

	Rect& rect = rects[draggingIndex];

	// 이동 중에도 사각형 크기가 바뀌지 않도록 현재 가로와 세로를 저장한다.
	float width = rect.x2 - rect.x1;
	float height = rect.y2 - rect.y1;

	// 1. 마우스 위치에 따라 일단 이동
	rect.x1 = real_x - dragOffsetX;
	rect.y1 = real_y - dragOffsetY;

	rect.x2 = rect.x1 + width;
	rect.y2 = rect.y1 + height;


	// 2. 이동한 사각형이 NDC 화면 범위(-1~1) 밖으로 나갔는지 검사

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
	// 마우스를 놓은 위치에서 다른 사각형과 겹쳤다면 하나의 큰 사각형으로 합친다.
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
	// 오른쪽 클릭 위치도 사각형 좌표와 비교할 수 있도록 NDC로 바꾼다.
	float real_x = static_cast<float>(x) / WIDTH * 2.0f - 1.0f;
	float real_y = 1.0f - static_cast<float>(y) / HEIGHT * 2.0f;

	// 화면에서 가장 위에 보이는 사각형부터 검사
	for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
		if (real_x < rects[i].x1 || real_x > rects[i].x2 ||
			real_y < rects[i].y1 || real_y > rects[i].y2)
			continue;

		// 삽입 시 vector의 저장 공간이 바뀔 수 있으므로 복사해서 사용
		// 원본을 두 번 복사한 뒤 각각의 경계를 줄여 두 조각으로 만든다.
		Rect first = rects[i];
		Rect second = rects[i];
		float ratio = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.6f;

		// 세로로 나눌지 가로로 나눌지를 랜덤하게 선택한다.
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

		// 분리된 두 조각은 서로 다른 랜덤 색상을 갖는다.
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

	// 드래그한 사각형과 나머지 사각형을 하나씩 비교한다.
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

		// 두 사각형의 최소 좌표와 최대 좌표를 사용해 둘을 모두 감싸는 사각형을 만든다.
		a.x1 = (min)(a.x1, b.x1);
		a.y1 = (min)(a.y1, b.y1);
		a.x2 = (max)(a.x2, b.x2);
		a.y2 = (max)(a.y2, b.y2);

		// 색상을 랜덤하게 변경
		a.r = static_cast<float>(rand()) / RAND_MAX;
		a.g = static_cast<float>(rand()) / RAND_MAX;
		a.b = static_cast<float>(rand()) / RAND_MAX;

		// 큰 사각형 안에 포함된 상대 사각형은 vector에서 제거한다.
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
