#define NOMINMAX
#include <Windows.h>
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

void InputProcess(GLFWwindow* window);
void DrawScene();

//--- 셰이더 파일 읽기, 컴파일, 프로그램 연결
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

//--- 사각뿔과 좌표축의 정점을 하나의 VAO/VBO에 저장한다.
void InitBuffer()
{
    const GLfloat positions[][3] = {
        //--- 사각뿔은 바닥 모서리 두 점과 꼭대기를 연결해서 옆면을 만든다.
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, {0.0f,0.5f,0.0f},
        { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {0.0f,0.5f,0.0f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {0.0f,0.5f,0.0f},
        { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, {0.0f,0.5f,0.0f},
        //--- 사각뿔 바닥은 사각형이므로 삼각형 두 개가 필요하다.
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        //--- 원점을 지나는 x, y, z축을 -1부터 1까지 그린다.
        {-1.0f,0.0f,0.0f}, {1.0f,0.0f,0.0f},
        {0.0f,-1.0f,0.0f}, {0.0f,1.0f,0.0f},
        {0.0f,0.0f,-1.0f}, {0.0f,0.0f,1.0f}
    };
    const GLfloat faceColors[5][3] = {
        {1.0f,0.1f,0.1f}, {0.1f,1.0f,0.1f}, {1.0f,0.9f,0.1f},
        {0.1f,0.2f,1.0f}, {1.0f,0.1f,1.0f}
    };
    GLfloat colors[24][3];
    //--- 옆면은 각각 3개, 바닥은 6개 정점이다. 같은 면에는 같은 색을 넣는다.
    for (int i = 0; i < 18; ++i) {
        int face = i < 12 ? i / 3 : 4;
        for (int j = 0; j < 3; ++j) colors[i][j] = faceColors[face][j];
    }
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            colors[18 + i][j] = (i / 2 == j) ? 1.0f : 0.0f;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);

    //--- 위치와 색상을 각각의 VBO에 저장한다.
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    //--- 모델링 변환: 원점에 육면체 하나를 배치한다.
    glm::mat4 model = glm::mat4(1.0f);
    //--- 뷰잉 변환: 세 면이 보이는 위치에서 원점을 바라본다.
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 1.5f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //--- 투영 변환: 현재 창의 종횡비로 원근 투영한다.
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        float(screenWidth) / float(screenHeight), 0.1f, 100.0f);

    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 18, 6);
    glDrawArrays(GL_TRIANGLES, 12, 6);
    glDrawArrays(GL_TRIANGLES, 0, 12);
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_16 - Pyramid Animation", nullptr, nullptr);
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
    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);

    //--- 창 크기가 바뀌면 뷰포트와 투영에 사용할 크기를 갱신한다.
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        screenWidth = width;
        screenHeight = height;
        glViewport(0, 0, width, height);
    });
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);

    //--- 메인 루프
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
