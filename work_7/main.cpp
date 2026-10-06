#define NOMINMAX
#include <Windows.h>
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

//--- 도형은 중심, 반너비/반높이, 색상을 저장한다.
struct Shape {
    int type; //--- 0: 점(작은 사각형), 1: 선, 2: 삼각형, 3: 사각형
    float x, y, w, h;
    float r, g, b;
};
Shape shapes[50];
int shapeCount = 0;
int selected = -1;
bool selectAll = false;
GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};

void InputProcess(GLFWwindow* window);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

//--- 실행 파일 옆의 셰이더를 읽는다. VS 작업 폴더에서도 읽을 수 있다.
string filetobuf(const char* name)
{
    wchar_t exePath[32768];
    DWORD length = GetModuleFileNameW(nullptr, exePath, 32768);
    ifstream file;
    if (length > 0 && length < 32768)
        file.open(filesystem::path(exePath).parent_path() / name);
    if (!file.is_open()) {
        file.clear();
        file.open(name);
    }
    if (!file) {
        cerr << "Cannot open shader: " << name << endl;
        return "";
    }
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint MakeShader(const char* name, GLenum type)
{
    string sourceText = filetobuf(name);
    if (sourceText.empty()) return 0;
    const char* source = sourceText.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        cerr << name << ": " << log << endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool InitShader()
{
    GLuint vertexShader = MakeShader("vertex.glsl", GL_VERTEX_SHADER);
    GLuint fragmentShader = MakeShader("fragment.glsl", GL_FRAGMENT_SHADER);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }
    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vertexShader);
    glAttachShader(shaderProgramID, fragmentShader);
    glLinkProgram(shaderProgramID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint result;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
    if (!result) {
        char log[2048];
        glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log);
        cerr << "Shader link failed: " << log << endl;
    }
    return result == GL_TRUE;
}

//--- 강의 예제처럼 위치와 색상을 각각의 VBO에 저장한다.
void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(i);
    }
}

float RandomFloat(float low, float high)
{
    return low + (high - low) * (float(rand()) / RAND_MAX);
}

void AddShape(int type)
{
    if (shapeCount == 50) {
        cout << "Maximum 50 shapes. Press C to clear.\n";
        return;
    }
    Shape& s = shapes[shapeCount++];
    s.type = type;
    s.w = type == 0 ? 0.012f : RandomFloat(0.05f, 0.14f);
    s.h = type == 0 ? 0.018f : RandomFloat(0.06f, 0.18f);
    s.x = RandomFloat(-0.8f, 0.8f);
    s.y = RandomFloat(-0.75f, 0.75f);
    s.r = RandomFloat(0.1f, 0.8f);
    s.g = RandomFloat(0.1f, 0.8f);
    s.b = RandomFloat(0.1f, 0.8f);
}

//--- 화면 가장자리에서는 이동량을 제한하여 도형 전체가 화면에 남도록 한다.
void MoveShapes(float dx, float dy)
{
    if (shapeCount == 0 || (!selectAll && selected < 0)) return;
    float left = 1.0f, right = -1.0f, bottom = 1.0f, top = -1.0f;
    for (int i = 0; i < shapeCount; ++i) {
        if (!selectAll && i != selected) continue;
        const Shape& s = shapes[i];
        left = min(left, s.x - s.w); right = max(right, s.x + s.w);
        bottom = min(bottom, s.y - s.h); top = max(top, s.y + s.h);
    }
    dx = clamp(dx, -1.0f - left, 1.0f - right);
    dy = clamp(dy, -1.0f - bottom, 1.0f - top);
    for (int i = 0; i < shapeCount; ++i) {
        if (!selectAll && i != selected) continue;
        shapes[i].x += dx;
        shapes[i].y += dy;
    }
}

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_P) AddShape(0);
        if (key == GLFW_KEY_E) AddShape(1);
        if (key == GLFW_KEY_T) AddShape(2);
        if (key == GLFW_KEY_R) AddShape(3);
        if (key == GLFW_KEY_C) {
            shapeCount = 0; selected = -1; selectAll = false;
        }
    }
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4) {
        selectAll = true;
        if (key == GLFW_KEY_1) MoveShapes(-0.02f, 0);
        if (key == GLFW_KEY_2) MoveShapes(0.02f, 0);
        if (key == GLFW_KEY_3) MoveShapes(0, 0.02f);
        if (key == GLFW_KEY_4) MoveShapes(0, -0.02f);
        return;
    }
    //--- 문자 이동 명령은 마우스로 선택한 한 도형에만 적용한다.
    if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S ||
        key == GLFW_KEY_D || key == GLFW_KEY_I || key == GLFW_KEY_J ||
        key == GLFW_KEY_K || key == GLFW_KEY_L) {
        selectAll = false;
        if (key == GLFW_KEY_W) MoveShapes(0, 0.02f);
        if (key == GLFW_KEY_A) MoveShapes(-0.02f, 0);
        if (key == GLFW_KEY_S) MoveShapes(0, -0.02f);
        if (key == GLFW_KEY_D) MoveShapes(0.02f, 0);
        if (key == GLFW_KEY_I) MoveShapes(-0.02f, 0.02f);
        if (key == GLFW_KEY_J) MoveShapes(0.02f, 0.02f);
        if (key == GLFW_KEY_K) MoveShapes(-0.02f, -0.02f);
        if (key == GLFW_KEY_L) MoveShapes(0.02f, -0.02f);
    }
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    int width, height;
    double mx, my;
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    glfwGetCursorPos(window, &mx, &my);
    float x = float(2.0 * mx / width - 1.0);
    float y = float(1.0 - 2.0 * my / height);
    selected = -1;
    selectAll = false;
    //--- 나중에 그린 도형부터 검사해서 맨 위의 도형을 선택한다.
    for (int i = shapeCount - 1; i >= 0; --i) {
        const Shape& s = shapes[i];
        float px = x - s.x, py = y - s.y;
        bool hit = false;
        if (s.type == 1) {
            //--- 선분까지의 거리를 화면 픽셀 단위로 계산한다(허용 거리 6px).
            float vx = s.w * width, vy = s.h * height;
            float qx = (px + s.w) * width * 0.5f;
            float qy = (py + s.h) * height * 0.5f;
            float t = clamp((qx * vx + qy * vy) / (vx * vx + vy * vy), 0.0f, 1.0f);
            float ex = qx - t * vx, ey = qy - t * vy;
            hit = ex * ex + ey * ey <= 36.0f;
        } else if (s.type == 2) {
            hit = py >= -s.h && py <= s.h &&
                abs(px) <= s.w * (s.h - py) / (2.0f * s.h);
        } else {
            hit = abs(px) <= s.w && abs(py) <= s.h;
        }
        if (hit) { selected = i; break; }
    }
}

void UploadAndDraw(const float* positions, int count, GLenum mode, float r, float g, float b)
{
    float colors[6][3];
    for (int i = 0; i < count; ++i) {
        colors[i][0] = r; colors[i][1] = g; colors[i][2] = b;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), positions);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), colors);
    glDrawArrays(mode, 0, count);
}

void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);
    for (int i = 0; i < shapeCount; ++i) {
        const Shape& s = shapes[i];
        float l = s.x - s.w, r = s.x + s.w;
        float b = s.y - s.h, t = s.y + s.h;
        float rectangle[] = { l,b,0, r,b,0, r,t,0, l,b,0, r,t,0, l,t,0 };
        float triangle[] = { l,b,0, r,b,0, s.x,t,0 };
        float line[] = { l,b,0, r,t,0 };
        if (s.type == 1) UploadAndDraw(line, 2, GL_LINES, s.r, s.g, s.b);
        else if (s.type == 2) UploadAndDraw(triangle, 3, GL_TRIANGLES, s.r, s.g, s.b);
        else UploadAndDraw(rectangle, 6, GL_TRIANGLES, s.r, s.g, s.b);
    }
    //--- 선택 표시: 검정 사각 테두리를 마지막에 그려 겹쳐도 확인 가능하다.
    for (int i = 0; i < shapeCount; ++i) {
        if (!selectAll && i != selected) continue;
        const Shape& s = shapes[i];
        float l = s.x - s.w, r = s.x + s.w;
        float b = s.y - s.h, t = s.y + s.h;
        float outline[] = { l,b,0, r,b,0, r,t,0, l,t,0 };
        UploadAndDraw(outline, 4, GL_LINE_LOOP, 0, 0, 0);
    }
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    srand(unsigned(time(nullptr)));
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_7 - Shader Shapes", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!InitShader()) {
        if (shaderProgramID) glDeleteProgram(shaderProgramID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    InitBuffer();
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    });
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    cout << "P: point | E: line | T: triangle | R: rectangle (max 50)\n"
        << "Left click: select | W/A/S/D: up/left/down/right\n"
        << "I/J/K/L: upper-left/upper-right/lower-left/lower-right\n"
        << "1/2/3/4: move all left/right/up/down | C: clear | Esc: exit\n";
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        InputProcess(window);
        DrawScene();
        glfwSwapBuffers(window);
    }
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
