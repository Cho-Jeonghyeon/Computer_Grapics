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
#include <algorithm>
#include <cmath>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

void InputProcess(GLFWwindow* window);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UpdateAnimation(float dt);

//--- 모드는 어떤 기준점으로 회전할지, opening은 열지 닫을지를 기억한다.
int animationMode = 0;
bool opening = false, depthTest = true;
float faceProgress[4] = {}; // 앞, 뒤, 왼쪽, 오른쪽: 0은 닫힘, 1은 완전히 열림
float swapAngle = 0.0f;
const glm::vec3 apex(0.0f, 0.5f, 0.0f);
const glm::vec3 hinges[4] = {
    {0.0f,-0.5f,0.5f}, {0.0f,-0.5f,-0.5f}, {-0.5f,-0.5f,0.0f}, {0.5f,-0.5f,0.0f}
};
//--- 같은 양의 각도로도 네 면이 모두 바깥쪽으로 열리도록 축의 부호를 정한다.
const glm::vec3 hingeAxes[4] = {{1,0,0}, {-1,0,0}, {0,0,1}, {0,0,-1}};

//--- 사각뿔 옆면은 처음부터 기울어져 있으므로 90도를 그대로 더하면 평평하지 않다.
//--- 높이 1, 밑면 반폭 0.5에서 옆면의 경사각은 atan2(1, 0.5)이다.
//--- 바깥쪽 평면까지는 180 - 경사각, 아래쪽 꼭대기에서 만나려면 그 두 배가 필요하다.
//--- 자료의 90/180도는 펼침 단계로 해석하고 실제 회전은 도형의 경사에 맞춰 계산한다.
const float flatAngle = 180.0f - glm::degrees(atan2(1.0f, 0.5f));

void ResetAnimation()
{
    animationMode = 0;
    opening = false;
    for (float& progress : faceProgress) progress = 0.0f;
    swapAngle = 0.0f;
}

float Approach(float value, float target, float step)
{
    //--- 목표를 넘지 않게 제한해야 마지막 면이 떨리거나 각도가 지나치지 않는다.
    return value < target ? min(value + step, target) : max(value - step, target);
}

glm::mat4 RotateAt(glm::vec3 pivot, float angle, glm::vec3 axis)
{
    //--- 기준점을 원점으로 옮긴 뒤 회전하고 다시 돌려놓는다. 적용은 오른쪽부터다.
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pivot);
    transform = glm::rotate(transform, glm::radians(angle), axis);
    return glm::translate(transform, -pivot);
}


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

//--- 사각뿔 옆면 12개, 바닥 6개, 좌표축 6개 정점을 순서대로 저장한다.
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

glm::mat4 FaceTransform(int face)
{
    if (animationMode == 6)
        return RotateAt(hinges[face], faceProgress[face] * 2.0f * flatAngle, hingeAxes[face]);
    if (animationMode == 7)
        return RotateAt(hinges[face], faceProgress[face] * flatAngle, hingeAxes[face]);
    if (animationMode == 8 && face < 2) {
        //--- 앞면과 뒷면을 꼭대기를 지나는 y축으로 서로 반대 방향 180도 회전한다.
        //--- 꼭대기는 고정된 채 두 면의 바닥 모서리가 서로의 자리로 이동한다.
        float angle = face == 0 ? swapAngle : -swapAngle;
        return RotateAt(apex, angle, glm::vec3(0,1,0));
    }
    return glm::mat4(1.0f);
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (depthTest) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);
    int side = min(screenWidth, screenHeight);
    glViewport((screenWidth-side)/2, (screenHeight-side)/2, side, side);

    //--- 아래로 뒤집히거나 밖으로 펼쳐진 면까지 보이도록 투영 범위에 여유를 둔다.
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(-2.1f, 2.1f, -2.1f, 2.1f, -4.0f, 4.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(30.0f), glm::vec3(1,0,0));
    base = glm::rotate(base, glm::radians(30.0f), glm::vec3(0,1,0));

    //--- 축과 바닥에는 면 애니메이션을 적용하지 않는다.
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &base[0][0]);
    glDrawArrays(GL_LINES, 18, 6);
    glDrawArrays(GL_TRIANGLES, 12, 6);
    for (int face = 0; face < 4; ++face) {
        //--- 각 면을 움직인 다음 전체 사각뿔의 초기 기울기를 적용한다.
        glm::mat4 model = base * FaceTransform(face);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLES, face * 3, 3);
    }
}

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    //--- 열기/닫기는 누를 때 한 번만 바뀌어야 하므로 REPEAT는 무시한다.
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_R) { ResetAnimation(); depthTest = true; return; }
    if (key == GLFW_KEY_H) { depthTest = !depthTest; return; }
    if (key < GLFW_KEY_6 || key > GLFW_KEY_8) return;
    int mode = key - GLFW_KEY_0;
    if (animationMode != mode) {
        //--- 기준점이 다른 동작으로 전환할 때는 원래 사각뿔에서 다시 시작한다.
        ResetAnimation();
        animationMode = mode;
    }
    //--- 진행 중에 다시 눌러도 현재 위치에서 자연스럽게 반대 방향으로 움직인다.
    opening = !opening;
}

void InputProcess(GLFWwindow* window)
{
    //--- 종료는 현재 키 상태를 확인하는 폴링으로 처리한다.
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void UpdateAnimation(float dt)
{
    float target = opening ? 1.0f : 0.0f;
    if (animationMode == 6 || animationMode == 9) {
        for (float& progress : faceProgress)
            progress = Approach(progress, target, 0.5f * dt);
    } else if (animationMode == 7) {
        //--- 앞->오른쪽->뒤->왼쪽 순서로 하나가 끝나야 다음 면을 움직인다.
        //--- 닫을 때는 반대로 돌아가면 중간에 방향을 바꿔도 자연스럽다.
        const int order[4] = {0, 3, 1, 2};
        float remaining = 0.7f * dt;
        for (int i = 0; i < 4 && remaining > 0.0f; ++i) {
            int face = opening ? order[i] : order[3-i];
            float distance = abs(target - faceProgress[face]);
            float step = min(distance, remaining);
            faceProgress[face] = Approach(faceProgress[face], target, step);
            remaining -= step;
        }
    } else if (animationMode == 8) {
        swapAngle = Approach(swapAngle, opening ? 180.0f : 0.0f, 90.0f * dt);
    }
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
    glfwSetKeyCallback(window, KeyCallback);
    cout << "6: open/close all sides | 7: open/close sides in sequence\n"
         << "8: swap opposite sides | 9: open/close around apex\n"
         << "R: reset | H: depth test | Q/Esc: exit\n";

    //--- 창 크기가 바뀌면 뷰포트와 투영에 사용할 크기를 갱신한다.
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        screenWidth = width;
        screenHeight = height;
        glViewport(0, 0, width, height);
    });
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);

    //--- 메인 루프
    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        //--- 프레임 수가 아니라 경과 시간에 맞춰 움직이고, 긴 정지 뒤의 튐은 제한한다.
        float dt = min(float(now - previousTime), 0.05f);
        previousTime = now;
        glfwPollEvents();
        InputProcess(window);
        UpdateAnimation(dt);
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
