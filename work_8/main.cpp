#define NOMINMAX

#include <Windows.h>

#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

struct Triangle {
    float x, y;
    float size;
    float r, g, b;
};
Triangle triangles[4];
GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};

void InputProcess(GLFWwindow* window);
void DrawScene();

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

void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);

        glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(i);
    }

}

float RandomFloat(float low, float high)
{

    return low + (high - low) * (float(rand()) / RAND_MAX);
}

void CreateTriangle(int index, float x, float y)
{
    Triangle& t = triangles[index];

    t.x = x;
    t.y = y;
    t.size = RandomFloat(0.10f, 0.22f);

    t.r = RandomFloat(0.1f, 0.85f);
    t.g = RandomFloat(0.1f, 0.85f);
    t.b = RandomFloat(0.1f, 0.85f);

}

void ResetScene()
{

    CreateTriangle(0,  0.5f,  0.5f);
    CreateTriangle(1, -0.5f,  0.5f);
    CreateTriangle(2, -0.5f, -0.5f);
    CreateTriangle(3,  0.5f, -0.5f);
}

void UploadAndDraw(const float* positions, int count, GLenum mode, float r, float g, float b)
{

    float colors[4][3];

    for (int i = 0; i < count; ++i) {
        colors[i][0] = r;
        colors[i][1] = g;
        colors[i][2] = b;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), positions);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), colors);
    glDrawArrays(mode, 0, count);

}

void DrawScene()
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);
    for (int i = 0; i < 4; ++i) {
        const Triangle& t = triangles[i];
        float w = t.size * 0.6f;
        float h = t.size;

        float positions[] = {
            t.x - w, t.y - h, 0,
            t.x + w, t.y - h, 0,
            t.x,     t.y + h, 0
        };

        UploadAndDraw(positions, 3, GL_TRIANGLES, t.r, t.g, t.b);
    }

    float axes[] = { -1,0,0, 1,0,0, 0,-1,0, 0,1,0 };

    UploadAndDraw(axes, 4, GL_LINES, 0.55f, 0.65f, 0.8f);
}

void InputProcess(GLFWwindow* window)
{

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    srand(unsigned(time(nullptr)));

    if (!glfwInit()) { cerr << "Failed to initialize GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_8 - Quadrant Triangles", nullptr, nullptr);

    if (!window) {
        cerr << "Failed to create window\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!InitShader()) {
        if (shaderProgramID) glDeleteProgram(shaderProgramID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    InitBuffer();
    ResetScene();
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {

        glViewport(0, 0, width, height);
    });
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    cout << "Quadrant triangles\n"
        << "Esc: exit\n";
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
