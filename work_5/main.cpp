#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

#define WIDTH 800
#define HEIGHT 600

using namespace std;

// 모든 일반 사각형은 가로와 세로가 20인 같은 크기로 그린다.
const float RECT_SIZE = 20.0f;
// 오른쪽 클릭으로 추가할 수 있는 사각형은 최대 10개이다.
const int MAX_NEW_RECT = 10;

struct Rect
{
    float x, y;       // 사각형 중심의 화면 좌표
    float r, g, b;    // 사각형의 RGB 색상
    bool visible;     // false이면 지우개에 닿은 상태이므로 화면에 그리지 않는다.
};

// 화면에 존재하는 모든 사각형을 저장한다.
vector<Rect> rects;
// 지우개는 왼쪽 마우스 버튼을 누르는 동안에만 화면에 보인다.
bool eraserVisible = false;
// 지우개 중심 위치와 한 변의 길이
float eraserX = 0.0f;
float eraserY = 0.0f;
float eraserSize = RECT_SIZE * 2.0f;
// 처음에는 검은색이고, 충돌하면 부딪힌 사각형의 색으로 바뀐다.
float eraserR = 0.0f;
float eraserG = 0.0f;
float eraserB = 0.0f;
// 오른쪽 클릭으로 새로 만든 사각형의 개수를 센다.
int newRectCount = 0;

void InputProcess(GLFWwindow* window);
void DrawScene();
void ResetScene();
void AddRect(float x, float y);
void CheckCollision();
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

int main()
{
    //--- GLFW 초기화
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // glRectf 같은 기존 OpenGL 그리기 함수를 사용하기 위해 호환 프로파일을 사용한다.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_5 - Eraser", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

    // 이 창을 현재 OpenGL 작업 대상으로 지정한 뒤 GLEW를 초기화한다.
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    //--- 콜백 함수 등록
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // 처음 실행할 때도 뷰포트와 좌표계를 창 크기에 맞게 한 번 설정한다.
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    FramebufferSizeCallback(window, width, height);

    // 실행할 때마다 사각형 위치와 색상이 달라지도록 난수 시드를 설정한다.
    // time()은 난수 초기화에만 사용하며 애니메이션 타이머로 사용하지 않는다.
    srand(static_cast<unsigned int>(time(nullptr)));
    ResetScene();
    glfwSwapInterval(1);

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        // 마우스와 키보드 이벤트를 먼저 처리한 후 현재 상태를 다시 그린다.
        glfwPollEvents();
        InputProcess(window);
        DrawScene();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

//--- 키보드 입력 처리
void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 키를 누르고 있는 동안 계속 초기화되지 않도록 이전 프레임 상태와 비교한다.
    static bool oldR = false;
    bool nowR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

    if (nowR && !oldR)
        ResetScene();

    oldR = nowR;
}

//--- 화면에 사각형 그리기
void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // 지우개와 충돌해 visible이 false가 된 사각형은 건너뛴다.
    for (const Rect& rect : rects) {
        if (!rect.visible)
            continue;

        glColor3f(rect.r, rect.g, rect.b);
        glRectf(rect.x - RECT_SIZE / 2.0f, rect.y - RECT_SIZE / 2.0f,
            rect.x + RECT_SIZE / 2.0f, rect.y + RECT_SIZE / 2.0f);
    }

    // 일반 사각형을 먼저 그리고 지우개를 마지막에 그려서 위에 보이게 한다.
    if (eraserVisible) {
        glColor3f(eraserR, eraserG, eraserB);
        glRectf(eraserX - eraserSize / 2.0f, eraserY - eraserSize / 2.0f,
            eraserX + eraserSize / 2.0f, eraserY + eraserSize / 2.0f);
    }
}

//--- 처음 상태로 다시 만들기
void ResetScene()
{
    rects.clear();
    eraserVisible = false;
    newRectCount = 0;

    // rand() % 21은 0~20이므로 전체 개수는 20~40개가 된다.
    int count = 20 + rand() % 21;
    for (int i = 0; i < count; ++i) {
        // 사각형 전체가 창 안에 들어오도록 가장자리에서 조금 떨어진 위치를 뽑는다.
        float x = RECT_SIZE + rand() % (WIDTH - static_cast<int>(RECT_SIZE * 2.0f));
        float y = RECT_SIZE + rand() % (HEIGHT - static_cast<int>(RECT_SIZE * 2.0f));
        AddRect(x, y);
    }
}

//--- 사각형 한 개 추가
void AddRect(float x, float y)
{
    Rect rect;
    rect.x = x;
    rect.y = y;
    // OpenGL 색상 범위인 0.0~1.0 사이의 값을 무작위로 만든다.
    rect.r = static_cast<float>(rand()) / RAND_MAX;
    rect.g = static_cast<float>(rand()) / RAND_MAX;
    rect.b = static_cast<float>(rand()) / RAND_MAX;
    rect.visible = true;
    rects.push_back(rect);
}

//--- 지우개와 사각형의 충돌 검사
void CheckCollision()
{
    for (Rect& rect : rects) {
        if (!rect.visible)
            continue;

        // 두 사각형 중심 사이의 가로·세로 거리를 각각 구한다.
        float distanceX = fabs(rect.x - eraserX);
        float distanceY = fabs(rect.y - eraserY);
        // 두 사각형의 반지름(한 변 길이의 절반)을 더한 값보다 가까우면 겹친다.
        float limit = RECT_SIZE / 2.0f + eraserSize / 2.0f;

        if (distanceX <= limit && distanceY <= limit) {
            // 벡터에서 실제로 삭제하지 않고 잠시 보이지 않게 만든다.
            rect.visible = false;
            // 사각형 하나를 지울 때마다 지우개를 키우고 그 사각형의 색을 가져온다.
            eraserSize += 5.0f;
            eraserR = rect.r;
            eraserG = rect.g;
            eraserB = rect.b;
        }
    }
}

//--- 마우스 버튼 입력 처리
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // 왼쪽 버튼을 새로 누를 때마다 지우개를 기본 크기와 검은색으로 되돌린다.
        eraserVisible = true;
        eraserX = static_cast<float>(xpos);
        eraserY = static_cast<float>(ypos);
        eraserSize = RECT_SIZE * 2.0f;
        eraserR = 0.0f;
        eraserG = 0.0f;
        eraserB = 0.0f;
        // 누른 위치에 사각형이 있을 수 있으므로 이동 전에도 충돌을 검사한다.
        CheckCollision();
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        eraserVisible = false;

        // 지우는 동안 숨겼던 사각형들을 원래 위치에 다시 나타나게 한다.
        for (Rect& rect : rects)
            rect.visible = true;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // 처음 만들어진 사각형 개수와 별개로 새 사각형은 최대 10개만 추가한다.
        if (newRectCount < MAX_NEW_RECT) {
            AddRect(static_cast<float>(xpos), static_cast<float>(ypos));
            ++newRectCount;

            // 왼쪽 버튼도 함께 누른 상태라면 현재 지우개 크기를 줄인다.
            if (eraserVisible && eraserSize > RECT_SIZE)
                eraserSize -= 5.0f;
        }
    }
}

//--- 마우스를 누른 채 움직이면 지우개 이동
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!eraserVisible)
        return;

    // GLFW가 알려 주는 현재 커서 좌표를 그대로 지우개 중심 좌표로 사용한다.
    eraserX = static_cast<float>(xpos);
    eraserY = static_cast<float>(ypos);
    CheckCollision();
}

//--- 창 크기에 맞춰 좌표 설정
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // 실제 프레임버퍼 전체를 OpenGL이 그리는 영역으로 지정한다.
    glViewport(0, 0, width, height);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // 왼쪽 위를 (0, 0)으로 두고 창의 픽셀 좌표처럼 사용할 수 있게 설정한다.
    // 이렇게 하면 GLFW의 마우스 좌표와 사각형 좌표를 바로 비교할 수 있다.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, windowWidth, windowHeight, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}
