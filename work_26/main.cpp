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
GLuint vao = 0, vbo[3] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

void InputProcess(GLFWwindow* window);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UpdateAnimation(float deltaTime);

//--- work_14처럼 입력은 상태를 바꾸고, DrawScene은 현재 상태를 읽어서 그린다.
constexpr float PI = 3.1415926535f;
float deltaTime = 0.0f;
float lightAngle = 0.0f;
int lightRotation = 0; // 0: 정지, +1/-1: y축 공전 방향
glm::vec3 lightColor(1.0f);
constexpr int CIRCLE_SEGMENTS = 128;
int objectType = 1; // n을 누르면 육면체(1) / 사각뿔(2) 전환
bool lightingEnabled = true;
float angleY = 0.0f;
int rotateY = 0;
float lightRadius = 1.8f; // 처음에는 물체 앞쪽 +Z에 조명을 둔다.
const glm::vec3 cameraPos(3.0f, 2.0f, 5.0f);

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

//--- work_14의 육면체 36개, 사각뿔 18개, 좌표축 6개를 그대로 사용한다.
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
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
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
    constexpr int vertexCount = 60 + CIRCLE_SEGMENTS;
    GLfloat allPositions[vertexCount][3] = {};
    GLfloat colors[vertexCount][3] = {};
    GLfloat normals[vertexCount][3] = {};
    for (int i = 0; i < 60; ++i)
        for (int j = 0; j < 3; ++j) {
            allPositions[i][j] = positions[i][j];
            colors[i][j] = 1.0f; // 도형의 재질색은 objectColor uniform으로 설정
        }

    //--- 각 삼각형의 두 변을 외적하면 면에 수직인 법선이 나온다.
    //--- 같은 면의 정점에는 같은 법선을 넣어 육면체/사각뿔의 모서리를 유지한다.
    for (int i = 0; i < 54; i += 3) {
        glm::vec3 a(positions[i][0], positions[i][1], positions[i][2]);
        glm::vec3 b(positions[i+1][0], positions[i+1][1], positions[i+1][2]);
        glm::vec3 c(positions[i+2][0], positions[i+2][1], positions[i+2][2]);
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        for (int vertex = 0; vertex < 3; ++vertex)
            for (int j = 0; j < 3; ++j)
                normals[i + vertex][j] = normal[j];
    }
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            colors[54 + i][j] = (i / 2 == j) ? 1.0f : 0.0f;

    //--- 반지름 1인 XZ 평면 원을 저장하고 DrawScene에서 현재 조명 거리만큼 확대한다.
    for (int i = 0; i < CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * float(i) / CIRCLE_SEGMENTS;
        allPositions[60+i][0] = sinf(angle);
        allPositions[60+i][2] = cosf(angle);
        colors[60+i][1] = 0.35f;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(3, vbo);

    //--- work_14의 위치/색상 VBO에 법선 VBO(location 2) 하나를 추가한다.
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(allPositions), allPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    int side = min(screenWidth, screenHeight);
    glViewport((screenWidth - side) / 2, (screenHeight - side) / 2, side, side);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::ortho(-3.4f, 3.4f, -3.4f, 3.4f, 0.1f, 20.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "objectColor");
    GLint lightingLoc = glGetUniformLocation(shaderProgramID, "lightingEnabled");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glBindVertexArray(vao);
    glEnable(GL_DEPTH_TEST);

    //--- 위치, 법선, 조명, 관찰자 모두 월드 좌표계를 기준으로 계산한다.
    float radians = glm::radians(lightAngle);
    glm::vec3 lightPos(lightRadius * sinf(radians), 0.0f, lightRadius * cosf(radians));
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightPos"), 1, &lightPos[0]);
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(shaderProgramID, "viewPos"), 1, &cameraPos[0]);

    //--- 좌표축과 공전 궤도는 조명 영향을 받지 않는 색으로 그린다.
    glUniform1i(lightingLoc, false);
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_LINES, 54, 6);
    model = glm::scale(glm::mat4(1.0f), glm::vec3(lightRadius));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_LINE_LOOP, 60, CIRCLE_SEGMENTS);

    //--- work_14와 동일하게 model을 uniform으로 보내고 도형의 범위만 선택한다.
    //--- 원점에 있는 물체만 y축으로 돌리므로 물체는 제자리에서 자전한다.
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniform3f(colorLoc, 0.15f, 0.85f, 0.12f);
    glUniform1i(lightingLoc, lightingEnabled);
    if (objectType == 1) glDrawArrays(GL_TRIANGLES, 0, 36);
    else glDrawArrays(GL_TRIANGLES, 36, 18);

    //--- 같은 육면체 정점을 작게 그려 점광원의 위치를 표시한다.
    //--- 표시 도형은 자체 색으로 그려 조명 계산과 구분한다.
    glUniform1i(lightingLoc, false);
    glm::vec3 markerColor = lightingEnabled ? lightColor : glm::vec3(0.25f);
    glUniform3fv(colorLoc, 1, &markerColor[0]);
    model = glm::translate(glm::mat4(1.0f), lightPos);
    model = glm::scale(model, glm::vec3(0.16f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    bool upper = (mods & GLFW_MOD_SHIFT) != 0;
    switch (key) {
    case GLFW_KEY_N: objectType = objectType == 1 ? 2 : 1; break;
    case GLFW_KEY_M: lightingEnabled = !lightingEnabled; break;
    case GLFW_KEY_Y: rotateY = rotateY == 0 ? 1 : 0; break;
    case GLFW_KEY_R: {
        //--- r: 양의 공전, Shift+R: 음의 공전. 같은 방향을 다시 누르면 정지.
        int direction = upper ? -1 : 1;
        lightRotation = lightRotation == direction ? 0 : direction;
        break;
    }
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, true); break;
    }
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    //--- z/Z는 누르는 동안 연속 이동. 물체 내부로 들어가거나 화면 밖으로 나가지 않게 제한한다.
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        bool upper = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                  || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        lightRadius = clamp(lightRadius + (upper ? 1.0f : -1.0f) * deltaTime, 1.1f, 2.8f);
    }
}

void UpdateAnimation(float dt)
{
    angleY = fmod(angleY + rotateY * 60.0f * dt, 360.0f);
    lightAngle = fmod(lightAngle + lightRotation * 45.0f * dt, 360.0f);
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_26 - Phong Lighting", nullptr, nullptr);
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
    cout << "N: cube/pyramid | M: lighting on/off | Y: spin/stop\nR / Shift+R: light orbit +/- (same key: stop)\nHold Z / Shift+Z: light closer/farther | Q / Esc: exit\n";

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
        double currentTime = glfwGetTime();
        //--- 창을 끌거나 중단했다 돌아왔을 때 한 번에 멀리 움직이지 않도록 제한한다.
        deltaTime = min(float(currentTime - previousTime), 0.05f);
        previousTime = currentTime;
        glfwPollEvents();
        InputProcess(window);
        UpdateAnimation(deltaTime);
        DrawScene();
        glfwSwapBuffers(window);
    }
    glDeleteBuffers(3, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
