#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

// 마우스로 만들 수 있는 사각형은 최대 5개이다.
const int MAX_RECT = 5;

struct Rect {
    float x, y;                 // 현재 중심
    float startX, startY;       // 처음 클릭한 중심
    float halfSize = 0.08f;     // 사각형 크기의 절반
    float r, g, b;              // 현재 RGB 색상
    float dx, dy;              // 이동 방향: -1 또는 1
    float zigzagY;             // 지그재그 이동의 기준 높이
    float edge = 0.0f;         // 가장자리 경로의 위치 (0~8)
    float sizeDirection = 1.0f; // 1이면 커지고, -1이면 작아진다.
};

// 현재 화면에 그려진 사각형들을 생성 순서대로 저장한다.
vector<Rect> rects;
int moveMode = 0;              // 0: 정지, 1: 대각선, 2: 지그재그, 3: 가장자리
bool sizeAnimation = false;
bool colorAnimation = false;
float colorTime = 0.0f;

void InputProcess(GLFWwindow* window);
void DrawScene();
void CreateRect(GLFWwindow* window);
void UpdateRects(float deltaTime);
void RandomColor(Rect& rect);
bool KeyPressed(GLFWwindow* window, int key);

int main()
{
    //--- GLFW 초기화 및 OpenGL 창 만들기
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // glRectf를 사용하기 위해 OpenGL 호환 프로파일을 사용한다.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_4 - OpenGL", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

    // 생성한 창을 현재 OpenGL 작업 대상으로 지정한 뒤 GLEW를 초기화한다.
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 모니터 화면 갱신 주기에 맞춰 출력하여 불필요하게 너무 빠르게 그리지 않는다.
    glfwSwapInterval(1);
    // 창 크기가 바뀌면 OpenGL이 그리는 영역도 같은 크기로 변경한다.
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    });
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    // 실행할 때마다 사각형의 색상과 처음 이동 방향이 달라지게 한다.
    srand(static_cast<unsigned int>(time(nullptr)));
    // 이전 프레임 시간을 기억해 두었다가 프레임 사이의 시간 차이를 계산한다.
    double previousTime = glfwGetTime();

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
        // 창을 이동하거나 멈췄다가 돌아와도 한 번에 멀리 이동하지 않도록 제한
        deltaTime = (min)(deltaTime, 0.05f);
        // 입력을 처리하고, 경과 시간만큼 상태를 갱신한 다음 화면에 그린다.
        glfwPollEvents();
        InputProcess(window);
        UpdateRects(deltaTime);
        DrawScene();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void InputProcess(GLFWwindow* window)
{
    // Q 또는 ESC를 누르면 메인 루프가 끝나도록 종료 상태를 설정한다.
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 마우스를 누르고 있는 동안 여러 개가 만들어지지 않도록 처음 누른 순간만 확인한다.
    static bool leftPressed = false;
    bool leftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftNow && !leftPressed)
        CreateRect(window);
    leftPressed = leftNow;

    // 1, 2, 3은 각각 대각선, 지그재그, 가장자리 이동 모드이다.
    // 현재 실행 중인 번호를 다시 누르면 moveMode를 0으로 바꿔 멈춘다.
    for (int mode = 1; mode <= 3; ++mode) {
        if (KeyPressed(window, GLFW_KEY_1 + mode - 1)) {
            moveMode = (moveMode == mode) ? 0 : mode;
            for (int i = 0; i < static_cast<int>(rects.size()); ++i) {
                rects[i].zigzagY = rects[i].y;
                // 생성 순서대로 경로 위에 일정한 간격을 배정
                if (moveMode == 3)
                    rects[i].edge = 8.0f * i / rects.size();
            }
        }
    }
    // 4와 5는 이동 모드와 별개이므로 크기와 색상 변화가 동시에 실행될 수도 있다.
    if (KeyPressed(window, GLFW_KEY_4))
        sizeAnimation = !sizeAnimation;
    if (KeyPressed(window, GLFW_KEY_5)) {
        colorAnimation = !colorAnimation;
        colorTime = 0.0f;
    }
    // S는 현재 실행 중인 모든 애니메이션을 한꺼번에 멈춘다.
    if (KeyPressed(window, GLFW_KEY_S)) {
        moveMode = 0;
        sizeAnimation = false;
        colorAnimation = false;
    }
    // M은 이동을 멈추고 각 사각형을 처음 마우스로 만든 위치로 돌려놓는다.
    if (KeyPressed(window, GLFW_KEY_M)) {
        moveMode = 0;
        for (Rect& rect : rects) {
            rect.x = rect.startX;
            rect.y = rect.startY;
        }
    }
    // R은 사각형을 모두 지우고 애니메이션 상태도 처음으로 되돌린다.
    if (KeyPressed(window, GLFW_KEY_R)) {
        rects.clear();
        moveMode = 0;
        sizeAnimation = false;
        colorAnimation = false;
        colorTime = 0.0f;
    }
}

void DrawScene()
{
    // 매 프레임 짙은 회색으로 화면을 지운 뒤 현재 사각형들을 다시 그린다.
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    for (const Rect& rect : rects) {
        glColor3f(rect.r, rect.g, rect.b);
        glRectf(rect.x - rect.halfSize, rect.y - rect.halfSize,
            rect.x + rect.halfSize, rect.y + rect.halfSize);
    }
}

// 키를 계속 누르고 있어도 처음 누른 순간에만 true
bool KeyPressed(GLFWwindow* window, int key)
{
    // 각 키의 바로 전 상태를 저장하여 RELEASE -> PRESS로 바뀐 순간을 찾는다.
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = now && !previous[key];
    previous[key] = now;
    return pressed;
}

void RandomColor(Rect& rect)
{
    // OpenGL 색상 범위인 0.0~1.0 사이의 RGB 값을 각각 만든다.
    rect.r = static_cast<float>(rand()) / RAND_MAX;
    rect.g = static_cast<float>(rand()) / RAND_MAX;
    rect.b = static_cast<float>(rand()) / RAND_MAX;
}

void CreateRect(GLFWwindow* window)
{
    // 이미 5개를 만들었다면 마우스를 눌러도 더 추가하지 않는다.
    if (rects.size() >= MAX_RECT)
        return;

    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;

    Rect rect;
    // 마우스의 픽셀 좌표를 OpenGL 좌표인 -1~1 범위로 바꾼다.
    // 마우스 y축은 아래 방향이므로 OpenGL 좌표에 맞게 위아래를 뒤집는다.
    rect.x = static_cast<float>(mouseX) / width * 2.0f - 1.0f;
    rect.y = 1.0f - static_cast<float>(mouseY) / height * 2.0f;
    // M키를 눌렀을 때 돌아갈 수 있도록 처음 만든 위치를 따로 저장한다.
    rect.startX = rect.x;
    rect.startY = rect.y;
    rect.zigzagY = rect.y;
    // 처음 움직일 가로와 세로 방향을 각각 랜덤하게 정한다.
    rect.dx = (rand() % 2 == 0) ? -1.0f : 1.0f;
    rect.dy = (rand() % 2 == 0) ? -1.0f : 1.0f;
    RandomColor(rect);
    rects.push_back(rect);
    // 가장자리 이동 도중 추가된 경우 기존 사각형과 다시 일정한 간격을 맞춘다.
    if (moveMode == 3) {
        for (int i = 0; i < static_cast<int>(rects.size()); ++i)
            rects[i].edge = 8.0f * i / rects.size();
    }
}

void UpdateRects(float deltaTime)
{
    // 색은 매 프레임 대신 0.5초마다 변경
    bool changeColor = false;
    if (colorAnimation) {
        colorTime += deltaTime;
        if (colorTime >= 0.5f) {
            colorTime -= 0.5f;
            changeColor = true;
        }
    }

    for (Rect& rect : rects) {
        // 4번 애니메이션: 최소 크기와 최대 크기 사이를 반복한다.
        if (sizeAnimation) {
            rect.halfSize += rect.sizeDirection * 0.04f * deltaTime;
            if (rect.halfSize >= 0.14f) {
                rect.halfSize = 0.14f;
                rect.sizeDirection = -1.0f;
            }
            else if (rect.halfSize <= 0.04f) {
                rect.halfSize = 0.04f;
                rect.sizeDirection = 1.0f;
            }
        }
        // 5번 애니메이션이 켜졌고 0.5초가 지난 프레임에만 색을 바꾼다.
        if (changeColor)
            RandomColor(rect);

        // 사각형의 절반 크기를 빼서 도형 전체가 화면 안에 있도록 이동 범위를 정한다.
        float limit = 1.0f - rect.halfSize;
        if (moveMode == 1 || moveMode == 2) {
            // 속도에 deltaTime을 곱하면 컴퓨터의 프레임 속도와 관계없이 비슷하게 움직인다.
            rect.x += rect.dx * 0.45f * deltaTime;
            rect.y += rect.dy * 0.30f * deltaTime;

            // 지그재그는 기준 높이의 위아래로 짧게 왕복
            float bottom = -limit;
            float top = limit;
            if (moveMode == 2) {
                rect.zigzagY = (max)(-limit, (min)(rect.zigzagY, limit));
                bottom = (max)(-limit, rect.zigzagY - 0.12f);
                top = (min)(limit, rect.zigzagY + 0.12f);
            }
            // 화면 경계에 닿으면 좌표를 경계에 맞추고 이동 방향을 반대로 바꾼다.
            if (rect.x >= limit) {
                rect.x = limit;
                rect.dx = -1.0f;
            }
            else if (rect.x <= -limit) {
                rect.x = -limit;
                rect.dx = 1.0f;
            }
            if (rect.y >= top) {
                rect.y = top;
                rect.dy = -1.0f;
            }
            else if (rect.y <= bottom) {
                rect.y = bottom;
                rect.dy = 1.0f;
            }
        }
        else if (moveMode == 3) {
            // edge는 정사각형 둘레 전체를 0~8로 표현한 현재 경로 위치이다.
            rect.edge += 0.5f * deltaTime;
            if (rect.edge >= 8.0f)
                rect.edge -= 8.0f;

            // 위쪽 오른쪽 이동 -> 오른쪽 아래 -> 아래쪽 왼쪽 -> 왼쪽 위
            // edge가 속한 구간에 따라 위, 오른쪽, 아래, 왼쪽 변의 목표점을 구한다.
            float targetX, targetY;
            if (rect.edge < 2.0f) {
                targetX = (-1.0f + rect.edge) * limit;
                targetY = limit;
            }
            else if (rect.edge < 4.0f) {
                targetX = limit;
                targetY = (3.0f - rect.edge) * limit;
            }
            else if (rect.edge < 6.0f) {
                targetX = (5.0f - rect.edge) * limit;
                targetY = -limit;
            }
            else {
                targetX = -limit;
                targetY = (rect.edge - 7.0f) * limit;
            }
            // 처음에는 지정된 가장자리 위치까지 이동하고 이후 경로를 따라감
            float diffX = targetX - rect.x;
            float diffY = targetY - rect.y;
            float distance = sqrt(diffX * diffX + diffY * diffY);
            float step = 1.5f * deltaTime;
            // 한 프레임 이동 거리보다 목표점이 가까우면 목표점에 정확히 놓는다.
            if (distance <= step) {
                rect.x = targetX;
                rect.y = targetY;
            }
            else {
                rect.x += diffX / distance * step;
                rect.y += diffY / distance * step;
            }
        }
    }
}
