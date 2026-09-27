#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

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
    bool moving;
};

vector<Rect> rects;

void InputProcess(GLFWwindow* window);
void DrawScene();
void ResetScene();
void CreateRandomRect();
void SplitRect(GLFWwindow* window);
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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        InputProcess(window);
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

        float pieceSize = source.halfSize * 0.5f;
        const float offsets[4][2] = {
            {-1.0f, -1.0f}, {1.0f, -1.0f},
            {-1.0f,  1.0f}, {1.0f,  1.0f}
        };

        for (int part = 0; part < 4; ++part) {
            Rect piece = source;
            piece.x = source.x + offsets[part][0] * pieceSize;
            piece.y = source.y + offsets[part][1] * pieceSize;
            piece.halfSize = pieceSize;
            piece.moving = true;
            rects.push_back(piece);
        }
        break;
    }
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
