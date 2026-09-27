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

const int MIN_RECT = 5;
const int MAX_RECT = 10;

struct Rect
{
    float x, y;
    float halfSize;
    float r, g, b;
    float dx, dy;
    bool moving;
};

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
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

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
    ResetScene();

    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
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

    static bool oldLeft = false;
    bool nowLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (nowLeft && !oldLeft)
        SplitRect(window);
    oldLeft = nowLeft;

    if (KeyPressed(window, GLFW_KEY_R))
        ResetScene();
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

void ResetScene()
{
    rects.clear();

    int count = MIN_RECT + rand() % (MAX_RECT - MIN_RECT + 1);
    for (int i = 0; i < count; ++i)
        CreateRandomRect();
}

void CreateRandomRect()
{
    Rect rect;
    rect.halfSize = 0.06f + RandomFloat() * 0.07f;

    float range = 2.0f - rect.halfSize * 2.0f;
    rect.x = -1.0f + rect.halfSize + RandomFloat() * range;
    rect.y = -1.0f + rect.halfSize + RandomFloat() * range;
    rect.r = RandomFloat();
    rect.g = RandomFloat();
    rect.b = RandomFloat();
    rect.dx = 0.0f;
    rect.dy = 0.0f;
    rect.moving = false;
    rects.push_back(rect);
}

void SplitRect(GLFWwindow* window)
{
    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;

    float x = static_cast<float>(mouseX) / width * 2.0f - 1.0f;
    float y = 1.0f - static_cast<float>(mouseY) / height * 2.0f;

    for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i) {
        Rect source = rects[i];
        if (source.moving)
            continue;

        if (x < source.x - source.halfSize || x > source.x + source.halfSize ||
            y < source.y - source.halfSize || y > source.y + source.halfSize)
            continue;

        rects.erase(rects.begin() + i);

        int moveType = 1 + rand() % 4;

        if (moveType == 4) {
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
            float pieceSize = source.halfSize * 0.5f;
            const float startOffset[4][2] = {
                {-1.0f, -1.0f}, {1.0f, -1.0f},
                {-1.0f,  1.0f}, {1.0f,  1.0f}
            };
            float directions[4][2] = {};

            if (moveType == 1) {
                directions[0][0] = -1.0f;
                directions[1][0] = 1.0f;
                directions[2][1] = -1.0f;
                directions[3][1] = 1.0f;
            }
            else if (moveType == 2) {
                for (int part = 0; part < 4; ++part) {
                    directions[part][0] = startOffset[part][0];
                    directions[part][1] = startOffset[part][1];
                }
            }
            else {
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

void AddPiece(const Rect& source, float x, float y,
    float halfSize, float dx, float dy)
{
    Rect piece;
    piece.x = x;
    piece.y = y;
    piece.halfSize = halfSize;
    piece.r = source.r;
    piece.g = source.g;
    piece.b = source.b;
    piece.dx = dx;
    piece.dy = dy;
    piece.moving = true;
    rects.push_back(piece);
}

void UpdateRects(float deltaTime)
{
    const float moveSpeed = 0.45f;
    const float shrinkSpeed = 0.025f;

    for (Rect& rect : rects) {
        if (!rect.moving)
            continue;

        rect.x += rect.dx * moveSpeed * deltaTime;
        rect.y += rect.dy * moveSpeed * deltaTime;
        rect.halfSize -= shrinkSpeed * deltaTime;
    }

    rects.erase(remove_if(rects.begin(), rects.end(),
        [](const Rect& rect) {
            return rect.moving && rect.halfSize <= 0.005f;
        }), rects.end());
}

bool KeyPressed(GLFWwindow* window, int key)
{
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = now && !previous[key];
    previous[key] = now;
    return pressed;
}

float RandomFloat()
{
    return static_cast<float>(rand()) / RAND_MAX;
}
