#define NOMINMAX
#include <Windows.h>
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
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

glm::vec3 tankPosition(0.0f, 0.0f, 0.0f);
float tankHeading = 0.0f;
float turretAngle = 0.0f;
float barrelAngle[2] = { 0.0f, 0.0f };
float flagAngle[2] = { 0.0f, 0.0f };
float turretSwap = 0.0f;
float animationTime = 0.0f;
float cameraMoveX = 0.0f, cameraMoveZ = 0.0f;
float cameraYaw = 0.0f, cameraOrbit = 0.0f;
float previousFrameTime = 0.0f;
bool treadAnimation = false, turretAnimation = false, swapAnimation = false;
bool barrelAnimation = false, flagAnimation = false;
bool cameraYawAnimation = false, cameraOrbitAnimation = false;
bool reverseYaw = false, reverseOrbit = false;
int colorSet = 0;
bool resetRequested = false;


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

//--- 육면체의 여섯 면을 각각 삼각형 두 개로 구성한다.
void InitBuffer()
{
    const GLfloat positions[][3] = {
        //--- 앞면
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        //--- 뒷면
        { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        { 0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
        //--- 왼쪽 면
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},
        //--- 오른쪽 면
        { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
        { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
        //--- 윗면
        {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
        {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        //--- 아랫면
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}
    };
    const GLfloat faceColors[6][3] = {
        {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 1.0f},
        {0.2f, 0.8f, 0.2f}, {0.2f, 0.4f, 1.0f},
        {1.0f, 0.8f, 0.2f}, {0.8f, 0.2f, 1.0f}
    };
    GLfloat colors[36][3];
    for (int i = 0; i < 36; ++i)
        for (int j = 0; j < 3; ++j)
            colors[i][j] = faceColors[i / 6][j];

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

void DrawCube(const glm::mat4& model, const glm::vec3& tint)
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &tint[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(0.48f, 0.68f, 0.82f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    //--- 카메라의 위치와 바라보는 방향을 나누어 제자리 회전과 공전을 표현한다.
    glm::vec3 cameraPosition(cameraMoveX, 3.5f, 7.0f + cameraMoveZ);
    const glm::vec3 target(0.0f, 0.25f, 0.0f);
    if (cameraOrbitAnimation) {
        glm::mat4 orbit = glm::rotate(glm::mat4(1.0f), cameraOrbit, glm::vec3(0, 1, 0));
        cameraPosition = glm::vec3(orbit * glm::vec4(cameraPosition, 1.0f));
    }
    glm::mat4 yaw = glm::rotate(glm::mat4(1.0f), cameraYaw, glm::vec3(0, 1, 0));
    glm::vec3 lookDirection = glm::normalize(target - cameraPosition);
    lookDirection = glm::vec3(yaw * glm::vec4(lookDirection, 0.0f));
    glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + lookDirection, glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        float(screenWidth) / float(screenHeight), 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);

    const glm::vec3 palette[3][5] = {
        { {0.18f,0.22f,0.25f}, {0.28f,0.38f,0.20f}, {0.38f,0.48f,0.24f}, {0.48f,0.55f,0.30f}, {0.85f,0.78f,0.42f} },
        { {0.20f,0.22f,0.30f}, {0.25f,0.32f,0.55f}, {0.35f,0.45f,0.78f}, {0.48f,0.58f,0.88f}, {0.95f,0.80f,0.35f} },
        { {0.24f,0.20f,0.18f}, {0.52f,0.28f,0.18f}, {0.72f,0.40f,0.22f}, {0.85f,0.54f,0.30f}, {0.92f,0.83f,0.55f} }
    };
    const glm::vec3* colors = palette[colorSet];

    //--- 바닥 큐브의 윗면 높이를 y=0으로 두어 탱크가 바닥 위에 놓이게 한다.
    glm::mat4 floor = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.12f, 0));
    floor = glm::scale(floor, glm::vec3(10.0f, 0.2f, 14.0f));
    DrawCube(floor, glm::vec3(0.28f, 0.34f, 0.25f));

    glm::mat4 root = glm::translate(glm::mat4(1.0f), tankPosition);
    root = glm::rotate(root, tankHeading, glm::vec3(0, 1, 0));
    float treadHeight = 0.48f + (treadAnimation ? 0.12f * sinf(animationTime * 3.0f) : 0.0f);
    //--- 좌우 두 개의 긴 아래 몸체를 큐브로 만든다.
    for (int side = -1; side <= 1; side += 2) {
        glm::mat4 track = glm::translate(root, glm::vec3(0.58f * side, treadHeight * 0.5f, 0));
        track = glm::scale(track, glm::vec3(0.34f, treadHeight, 1.65f));
        DrawCube(track, colors[0]);
    }

    //--- 중앙 몸체는 y축 회전하고, 위쪽 부품은 그 회전을 이어받는다.
    glm::mat4 center = glm::translate(root, glm::vec3(0, 0.54f, 0));
    center = glm::rotate(center, turretAngle, glm::vec3(0, 1, 0));
    DrawCube(glm::scale(center, glm::vec3(1.05f, 0.42f, 1.45f)), colors[1]);

    //--- 위쪽 몸체 두 개는 l 애니메이션으로 앞뒤 위치가 교차한다.
    for (int side = 0; side < 2; ++side) {
        float x = side == 0 ? -0.30f : 0.30f;
        float z = (side == 0 ? -0.28f : 0.28f) * (1.0f - 2.0f * turretSwap);
        glm::mat4 upper = glm::translate(root, glm::vec3(x, 0.88f, z));
        upper = glm::rotate(upper, turretAngle, glm::vec3(0, 1, 0));
        DrawCube(glm::scale(upper, glm::vec3(0.52f, 0.30f, 0.72f)), colors[2]);

        //--- 포신 둘은 y축 회전 방향을 반대로 한다.
        glm::mat4 barrel = glm::translate(upper, glm::vec3(0, 0, 0.53f));
        barrel = glm::rotate(barrel, barrelAngle[side], glm::vec3(0, 1, 0));
        barrel = glm::translate(barrel, glm::vec3(0, 0, 0.39f));
        DrawCube(glm::scale(barrel, glm::vec3(0.12f, 0.12f, 0.82f)), colors[3]);

        //--- 깃대와 작은 깃발도 각 상부 몸체에 붙여 x축으로 반대로 흔든다.
        glm::mat4 pole = glm::translate(upper, glm::vec3(0, 0.30f, -0.20f));
        pole = glm::rotate(pole, flagAngle[side], glm::vec3(1, 0, 0));
        DrawCube(glm::scale(pole, glm::vec3(0.035f, 0.52f, 0.035f)), colors[4]);
        glm::mat4 flag = glm::translate(upper, glm::vec3(0.12f, 0.49f, -0.20f));
        flag = glm::rotate(flag, flagAngle[side], glm::vec3(1, 0, 0));
        DrawCube(glm::scale(flag, glm::vec3(0.25f, 0.15f, 0.035f)), colors[4]);
    }
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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_19 - Tank on the Ground", nullptr, nullptr);
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
