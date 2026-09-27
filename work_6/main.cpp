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

// 화면에 처음 만드는 사각형은 5~10개이다.
const int MIN_RECT = 5;
const int MAX_RECT = 10;

struct Rect
{
    float x, y;             // 사각형의 중심 위치
    float halfSize;         // 사각형 한 변 길이의 절반
    float r, g, b;          // 현재 RGB 색상
    float dx, dy;           // 퍼져 나갈 방향
    bool moving;            // false이면 처음 사각형, true이면 분리된 조각
    bool brighten;          // true이면 밝아지고 false이면 어두워진다.
};

// 처음 사각형과 분리된 조각을 같은 vector에서 관리한다.
vector<Rect> rects;

void InputProcess(GLFWwindow* window);
void DrawScene();
void ResetScene();
void CreateRandomRect();
void SplitRect(GLFWwindow* window);
void AddPiece(const Rect& source, float x, float y,
    float halfSize, float dx, float dy);
void UpdateRects(float deltaTime);
bool KeyPressed(GLFWwindow* window, int key);
float RandomFloat();

int main()
{
    //--- GLFW 초기화
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // glRectf를 사용하기 위해 OpenGL 호환 프로파일을 사용한다.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "work_6 - Spreading Rectangles", nullptr, nullptr);
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

    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    });

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // 실행할 때마다 사각형의 위치, 크기, 색상이 달라지게 한다.
    srand(static_cast<unsigned int>(time(nullptr)));
    ResetScene();

    // 이전 프레임 시간을 저장해서 프레임 사이에 지난 시간을 구한다.
    double previousTime = glfwGetTime();

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        // 창을 움직이거나 멈춘 뒤 조각이 갑자기 멀리 이동하지 않도록 제한한다.
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

//--- 키보드와 마우스 입력 처리
void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 왼쪽 버튼을 계속 누르고 있어도 처음 누른 순간에만 한 번 분리한다.
    static bool oldLeft = false;
    bool nowLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (nowLeft && !oldLeft)
        SplitRect(window);
    oldLeft = nowLeft;

    // R키를 누르면 처음의 랜덤 사각형 5~10개를 다시 만든다.
    if (KeyPressed(window, GLFW_KEY_R))
        ResetScene();
}

//--- 현재 사각형들을 화면에 그리기
void DrawScene()
{
    // 밝은 조각과 어두운 조각이 모두 잘 보이도록 짙은 회색 배경을 사용한다.
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const Rect& rect : rects) {
        glColor3f(rect.r, rect.g, rect.b);
        glRectf(rect.x - rect.halfSize, rect.y - rect.halfSize,
            rect.x + rect.halfSize, rect.y + rect.halfSize);
    }
}

//--- 실습을 처음 상태로 되돌리기
void ResetScene()
{
    rects.clear();

    // rand() % 6은 0~5이므로 전체 개수는 5~10개가 된다.
    int count = MIN_RECT + rand() % (MAX_RECT - MIN_RECT + 1);
    for (int i = 0; i < count; ++i)
        CreateRandomRect();
}

//--- 랜덤한 위치, 크기, 색상의 원본 사각형 만들기
void CreateRandomRect()
{
    Rect rect;

    // halfSize를 0.06~0.13 사이로 정하여 사각형마다 크기를 다르게 한다.
    rect.halfSize = 0.06f + RandomFloat() * 0.07f;

    // 사각형 전체가 화면의 -1~1 범위 안에 들어오도록 중심 위치를 정한다.
    float range = 2.0f - rect.halfSize * 2.0f;
    rect.x = -1.0f + rect.halfSize + RandomFloat() * range;
    rect.y = -1.0f + rect.halfSize + RandomFloat() * range;

    rect.r = RandomFloat();
    rect.g = RandomFloat();
    rect.b = RandomFloat();
    rect.dx = 0.0f;
    rect.dy = 0.0f;
    rect.moving = false;
    rect.brighten = false;
    rects.push_back(rect);
}

//--- 클릭한 사각형을 작은 조각으로 나누기
void SplitRect(GLFWwindow* window)
{
    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;

    // 마우스 픽셀 좌표를 OpenGL의 -1~1 좌표로 바꾼다.
    float x = static_cast<float>(mouseX) / width * 2.0f - 1.0f;
    float y = 1.0f - static_cast<float>(mouseY) / height * 2.0f;

    // 뒤에서부터 검사하여 사각형이 겹친 경우 화면의 위쪽 사각형을 선택한다.
    for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
        Rect source = rects[i];

        // 이미 퍼져 나가는 조각은 다시 분리하지 않는다.
        if (source.moving)
            continue;

        if (x < source.x - source.halfSize || x > source.x + source.halfSize ||
            y < source.y - source.halfSize || y > source.y + source.halfSize)
            continue;

        // 클릭한 원본은 지우고 그 자리에 작은 조각들을 넣는다.
        rects.erase(rects.begin() + i);

        // 문제의 네 가지 이동 방법 중 하나를 랜덤하게 선택한다.
        int moveType = 1 + rand() % 4;

        if (moveType == 4) {
            // 4번은 상하좌우와 대각선을 합친 8방향으로 8개를 만든다.
            const float directions[8][2] = {
                {-1.0f,  0.0f}, {1.0f,  0.0f},
                { 0.0f, -1.0f}, {0.0f,  1.0f},
                {-1.0f, -1.0f}, {1.0f, -1.0f},
                {-1.0f,  1.0f}, {1.0f,  1.0f}
            };
            float pieceSize = source.halfSize * 0.35f;

            for (int part = 0; part < 8; ++part) {
                float dx = directions[part][0];
                float dy = directions[part][1];
                float length = sqrt(dx * dx + dy * dy);
                dx /= length;
                dy /= length;

                AddPiece(source,
                    source.x + dx * pieceSize,
                    source.y + dy * pieceSize,
                    pieceSize, dx, dy);
            }
        }
        else {
            // 1~3번은 원본을 사등분한 위치에서 네 조각이 시작한다.
            float pieceSize = source.halfSize * 0.5f;
            const float startOffset[4][2] = {
                {-1.0f, -1.0f}, {1.0f, -1.0f},
                {-1.0f,  1.0f}, {1.0f,  1.0f}
            };
            float directions[4][2] = {};

            if (moveType == 1) {
                // 1번: 왼쪽, 오른쪽, 아래, 위로 각각 이동한다.
                directions[0][0] = -1.0f;
                directions[1][0] = 1.0f;
                directions[2][1] = -1.0f;
                directions[3][1] = 1.0f;
            }
            else if (moveType == 2) {
                // 2번: 네 조각이 서로 다른 대각선 방향으로 이동한다.
                for (int part = 0; part < 4; ++part) {
                    directions[part][0] = startOffset[part][0];
                    directions[part][1] = startOffset[part][1];
                }
            }
            else {
                // 3번: 네 조각이 모두 같은 랜덤 방향으로 함께 이동한다.
                const float allDirections[8][2] = {
                    {-1.0f,  0.0f}, {1.0f,  0.0f},
                    { 0.0f, -1.0f}, {0.0f,  1.0f},
                    {-1.0f, -1.0f}, {1.0f, -1.0f},
                    {-1.0f,  1.0f}, {1.0f,  1.0f}
                };
                int direction = rand() % 8;
                for (int part = 0; part < 4; ++part) {
                    directions[part][0] = allDirections[direction][0];
                    directions[part][1] = allDirections[direction][1];
                }
            }

            for (int part = 0; part < 4; ++part) {
                float dx = directions[part][0];
                float dy = directions[part][1];
                float length = sqrt(dx * dx + dy * dy);
                dx /= length;
                dy /= length;

                AddPiece(source,
                    source.x + startOffset[part][0] * pieceSize,
                    source.y + startOffset[part][1] * pieceSize,
                    pieceSize, dx, dy);
            }
        }
        break;
    }
}

//--- 분리된 조각 하나를 vector에 추가하기
void AddPiece(const Rect& source, float x, float y,
    float halfSize, float dx, float dy)
{
    Rect piece;
    piece.x = x;
    piece.y = y;
    piece.halfSize = halfSize;
    // 처음 색상은 원본 사각형의 색상과 같다.
    piece.r = source.r;
    piece.g = source.g;
    piece.b = source.b;
    piece.dx = dx;
    piece.dy = dy;
    piece.moving = true;
    // 각 조각은 밝아지거나 어두워지는 방식 중 하나를 랜덤하게 선택한다.
    piece.brighten = rand() % 2 == 0;
    rects.push_back(piece);
}

//--- 분리된 조각의 위치, 크기, 색상 갱신하기
void UpdateRects(float deltaTime)
{
    const float moveSpeed = 0.45f;
    const float shrinkSpeed = 0.025f;
    const float colorSpeed = 0.45f;

    for (Rect& rect : rects) {
        // 처음 놓여 있는 사각형은 클릭하기 전까지 움직이지 않는다.
        if (!rect.moving)
            continue;

        // deltaTime을 곱하면 컴퓨터의 프레임 속도와 관계없이 비슷한 속도로 이동한다.
        rect.x += rect.dx * moveSpeed * deltaTime;
        rect.y += rect.dy * moveSpeed * deltaTime;

        // 시간이 지날수록 크기를 줄이고 아주 작아지면 아래에서 vector에서 제거한다.
        rect.halfSize -= shrinkSpeed * deltaTime;

        if (rect.brighten) {
            // 각 색상 값을 1에 가깝게 만들면 점점 흰색으로 밝아진다.
            rect.r = (min)(1.0f, rect.r + colorSpeed * deltaTime);
            rect.g = (min)(1.0f, rect.g + colorSpeed * deltaTime);
            rect.b = (min)(1.0f, rect.b + colorSpeed * deltaTime);
        }
        else {
            // 각 색상 값을 0에 가깝게 만들면 점점 검은색으로 어두워진다.
            rect.r = (max)(0.0f, rect.r - colorSpeed * deltaTime);
            rect.g = (max)(0.0f, rect.g - colorSpeed * deltaTime);
            rect.b = (max)(0.0f, rect.b - colorSpeed * deltaTime);
        }
    }

    // 크기가 거의 0이 된 조각은 더 이상 그릴 필요가 없으므로 삭제한다.
    rects.erase(remove_if(rects.begin(), rects.end(),
        [](const Rect& rect) {
            return rect.moving && rect.halfSize <= 0.005f;
        }), rects.end());
}

//--- 키를 계속 누르고 있어도 처음 누른 순간에만 true 반환
bool KeyPressed(GLFWwindow* window, int key)
{
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = now && !previous[key];
    previous[key] = now;
    return pressed;
}

//--- 0.0~1.0 사이의 랜덤 실수 만들기
float RandomFloat()
{
    return static_cast<float>(rand()) / RAND_MAX;
}
