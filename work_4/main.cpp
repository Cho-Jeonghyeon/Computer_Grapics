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

const int MAX_RECT = 5;

struct Rect {
    float x, y;                 // 현재 중심
    float startX, startY;       // 처음 클릭한 중심
    float halfSize = 0.08f;     // 사각형 크기의 절반
    float r, g, b;
    float dx, dy;              // 이동 방향: -1 또는 1
    float zigzagY;             // 지그재그 이동의 기준 높이
    float edge = 0.0f;         // 가장자리 경로의 위치 (0~8)
    float sizeDirection = 1.0f;
};

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
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_4 - OpenGL", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    });
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    srand(static_cast<unsigned int>(time(nullptr)));
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
        // 창을 이동하거나 멈췄다가 돌아와도 한 번에 멀리 이동하지 않도록 제한
        deltaTime = (min)(deltaTime, 0.05f);
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
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    static bool leftPressed = false;
    bool leftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftNow && !leftPressed)
        CreateRect(window);
    leftPressed = leftNow;

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
    if (KeyPressed(window, GLFW_KEY_4))
        sizeAnimation = !sizeAnimation;
    if (KeyPressed(window, GLFW_KEY_5)) {
        colorAnimation = !colorAnimation;
        colorTime = 0.0f;
    }
    if (KeyPressed(window, GLFW_KEY_S)) {
        moveMode = 0;
        sizeAnimation = false;
        colorAnimation = false;
    }
    if (KeyPressed(window, GLFW_KEY_M)) {
        moveMode = 0;
        for (Rect& rect : rects) {
            rect.x = rect.startX;
            rect.y = rect.startY;
        }
    }
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
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = now && !previous[key];
    previous[key] = now;
    return pressed;
}

void RandomColor(Rect& rect)
{
    rect.r = static_cast<float>(rand()) / RAND_MAX;
    rect.g = static_cast<float>(rand()) / RAND_MAX;
    rect.b = static_cast<float>(rand()) / RAND_MAX;
}

void CreateRect(GLFWwindow* window)
{
    if (rects.size() >= MAX_RECT)
        return;

    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;

    Rect rect;
    rect.x = static_cast<float>(mouseX) / width * 2.0f - 1.0f;
    rect.y = 1.0f - static_cast<float>(mouseY) / height * 2.0f;
    rect.startX = rect.x;
    rect.startY = rect.y;
    rect.zigzagY = rect.y;
    rect.dx = (rand() % 2 == 0) ? -1.0f : 1.0f;
    rect.dy = (rand() % 2 == 0) ? -1.0f : 1.0f;
    RandomColor(rect);
    rects.push_back(rect);
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
        if (changeColor)
            RandomColor(rect);

        float limit = 1.0f - rect.halfSize;
        if (moveMode == 1 || moveMode == 2) {
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
            rect.edge += 0.5f * deltaTime;
            if (rect.edge >= 8.0f)
                rect.edge -= 8.0f;

            // 위쪽 오른쪽 이동 -> 오른쪽 아래 -> 아래쪽 왼쪽 -> 왼쪽 위
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
