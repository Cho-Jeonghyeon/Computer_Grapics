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
void UpdateAnimation(float deltaTime);

//--- 입력은 상태만 바꾸고, DrawScene은 이 상태를 읽어서 그리도록 나눈다.
//--- 처음에는 좌표축만 보이고 C 또는 P를 눌러 도형을 선택한다.
int objectType = 0; // 0: 선택 전, 1: 육면체, 2: 사각뿔
bool depthTest = true;
bool wireframe = false;
glm::vec3 objectPosition(0.0f);
float angleX = 30.0f, angleY = 30.0f;
int rotateX = 0, rotateY = 0; // 0: 정지, 1: 양의 방향, -1: 음의 방향
float deltaTime = 0.0f;


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

//--- 육면체 36개, 사각뿔 18개, 좌표축 6개 순서로 정점을 저장한다.
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
    GLfloat colors[60][3];
    //--- 이번에는 면 색상이 아니라 꼭짓점 색상이다.
    //--- 좌표 -0.5~0.5를 색상 0~1로 바꾸면 육면체의 8개 꼭짓점 색이 모두 달라진다.
    //--- 같은 꼭짓점이 여러 삼각형에 들어가도 좌표가 같으니 색도 같아진다.
    //--- 사각뿔도 바닥 네 꼭짓점과 꼭대기가 서로 다른 색을 갖는다.
    for (int i = 0; i < 54; ++i)
        for (int j = 0; j < 3; ++j)
            colors[i][j] = positions[i][j] + 0.5f;
    //--- 좌표축은 도형과 구분되도록 x 빨강, y 초록, z 파랑으로 고정한다.
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            colors[54 + i][j] = (i / 2 == j) ? 1.0f : 0.0f;

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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    //--- 뷰포트를 정사각형으로 유지하면 창을 늘려도 도형이 찌그러지지 않는다.
    int side = min(screenWidth, screenHeight);
    glViewport((screenWidth - side) / 2, (screenHeight - side) / 2, side, side);
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -3.0f, 3.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glBindVertexArray(vao);

    if (depthTest) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    //--- 좌표축까지 도형의 행렬로 그리면 같이 이동하고 회전해 버린다.
    //--- 축에는 항상 같은 행렬을 보내서 화면 중앙에 고정한다.
    glm::mat4 axisModel(1.0f);
    axisModel = glm::rotate(axisModel, glm::radians(30.0f), glm::vec3(1, 0, 0));
    axisModel = glm::rotate(axisModel, glm::radians(30.0f), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &axisModel[0][0]);
    glDrawArrays(GL_LINES, 54, 6);

    //--- 먼저 자기 중심에서 회전한 뒤 이동해야 원점 주위를 공전하지 않는다.
    //--- T * Rx * Ry는 정점에 오른쪽부터 적용된다.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), objectPosition);
    model = glm::rotate(model, glm::radians(angleX), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(angleY), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    if (objectType == 1) glDrawArrays(GL_TRIANGLES, 0, 36);
    else if (objectType == 2) glDrawArrays(GL_TRIANGLES, 36, 18);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

//--- 한 번 누를 때 한 번 바뀌는 명령은 콜백으로 처리한다.
void KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    //--- GLFW 키 코드는 대소문자가 같아서 Shift 상태도 함께 확인해야 한다.
    bool upper = (mods & GLFW_MOD_SHIFT) != 0;
    switch (key) {
    case GLFW_KEY_C: objectType = 1; break;
    case GLFW_KEY_P: objectType = 2; break;
    case GLFW_KEY_H: depthTest = !depthTest; break;
    case GLFW_KEY_W: wireframe = !upper; break; // w: 선, Shift+W: 면
    case GLFW_KEY_X: rotateX = upper ? -1 : 1; break;
    case GLFW_KEY_Y: rotateY = upper ? -1 : 1; break;
    case GLFW_KEY_S:
        //--- 위치만 돌려놓으면 다음 프레임에 또 회전하므로 방향 상태도 지운다.
        objectPosition = glm::vec3(0.0f);
        angleX = angleY = 30.0f;
        rotateX = rotateY = 0;
        break;
    }
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (objectType == 0) return;
    //--- 방향키는 누르는 동안 계속 움직여야 하므로 매 프레임 폴링한다.
    //--- 초당 이동량에 경과 시간을 곱하면 컴퓨터 속도에 덜 영향을 받는다.
    float distance = 0.6f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) objectPosition.x -= distance;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) objectPosition.x += distance;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) objectPosition.y += distance;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) objectPosition.y -= distance;
}

void UpdateAnimation(float dt)
{
    if (objectType == 0) return;
    //--- 키를 놓아도 방향을 기억하고 있으므로 회전 애니메이션은 계속된다.
    //--- 각도가 끝없이 커지지 않도록 한 바퀴를 넘으면 정리한다.
    angleX = fmod(angleX + rotateX * 60.0f * dt, 360.0f);
    angleY = fmod(angleY + rotateY * 60.0f * dt, 360.0f);
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_14 - Vertex Colors and Transform", nullptr, nullptr);
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
    cout << "C: cube | P: pyramid | H: depth test on/off\n"
         << "W: wireframe | Shift+W: solid\n"
         << "X/Y: positive rotation | Shift+X/Y: negative rotation\n"
         << "Arrows: move | S: reset position/rotation and stop | Esc: exit\n";

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
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
