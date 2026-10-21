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

//--- 상자 다섯 개가 한 경로에서 서로 같은 간격으로 움직이도록 상태를 둔다.
constexpr float PI = 3.1415926535f;
constexpr float BELT_Y = 0.78f;
constexpr float BOX_SIZE = 0.58f;
constexpr float TURN_RADIUS = 0.65f;
constexpr float STRAIGHT_IN = 4.7f;
constexpr float STRAIGHT_OUT = 5.0f;
constexpr float TURN_LENGTH = PI * TURN_RADIUS * 0.5f;
constexpr float PATH_LENGTH = STRAIGHT_IN + TURN_LENGTH + STRAIGHT_OUT;
constexpr float BOX_SPEED = 0.9f;
float beltTime = 0.0f;
float previousFrameTime = 0.0f;
float cameraAngle = 0.65f;
float cameraDistance = 13.0f;
bool beltMoving = false;
bool perspectiveProjection = true;
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
        {1, 1, 1}, {1, 1, 1}, {1, 1, 1},
        {1, 1, 1}, {1, 1, 1}, {1, 1, 1}
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

void DrawCube(const glm::mat4& model, const glm::vec3& color)
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &color[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

//--- 이동 거리 하나를 경로의 어느 구간에 놓을지 계산해서 위치와 진행 방향을 돌려준다.
void GetParcelPose(float distance, glm::vec3& position, float& yaw)
{
    distance = fmodf(distance, PATH_LENGTH);
    if (distance < 0.0f) distance += PATH_LENGTH;

    if (distance < STRAIGHT_IN) {
        position = glm::vec3(-STRAIGHT_IN + distance, BELT_Y + 0.06f + BOX_SIZE * 0.5f, 0.0f);
        yaw = glm::radians(90.0f); //--- 첫 구간은 +X 방향으로 간다.
        return;
    }

    distance -= STRAIGHT_IN;
    if (distance < TURN_LENGTH) {
        //--- 코너는 사분원으로 돌아서 상자가 위치와 방향을 자연스럽게 바꾼다.
        float t = distance / TURN_LENGTH;
        float angle = glm::radians(-90.0f + 90.0f * t);
        position = glm::vec3(TURN_RADIUS * cosf(angle), BELT_Y + 0.06f + BOX_SIZE * 0.5f,
            TURN_RADIUS + TURN_RADIUS * sinf(angle));
        yaw = -angle;
        return;
    }

    distance -= TURN_LENGTH;
    position = glm::vec3(TURN_RADIUS, BELT_Y + 0.06f + BOX_SIZE * 0.5f, TURN_RADIUS + distance);
    yaw = 0.0f; //--- 두 번째 구간은 +Z 방향으로 간다.
}

//--- 상자 앞면에 7세그먼트 숫자를 큐브 조각으로 그려서 번호를 알아볼 수 있게 한다.
void DrawPackageNumber(const glm::mat4& boxRoot, int number)
{
    static const bool digits[5][7] = {
        { false, true, true, false, false, false, false },
        { true, true, false, true, true, false, true },
        { true, true, true, true, false, false, true },
        { false, true, true, false, false, true, true },
        { true, false, true, true, false, true, true }
    };
    const glm::vec3 ink(0.08f, 0.08f, 0.09f);
    const glm::vec2 centers[7] = {
        {0, 0.14f}, {0.10f, 0.07f}, {0.10f, -0.07f}, {0, -0.14f},
        {-0.10f, -0.07f}, {-0.10f, 0.07f}, {0, 0}
    };
    for (int segment = 0; segment < 7; ++segment) {
        if (!digits[number - 1][segment]) continue;
        bool horizontal = segment == 0 || segment == 3 || segment == 6;
        glm::mat4 mark = glm::translate(boxRoot,
            glm::vec3(centers[segment].x, centers[segment].y, BOX_SIZE * 0.5f + 0.008f));
        mark = glm::scale(mark, horizontal ? glm::vec3(0.19f, 0.035f, 0.018f)
            : glm::vec3(0.035f, 0.12f, 0.018f));
        DrawCube(mark, ink);
    }
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    //--- 눌렀을 때 한 번만 바뀌는 명령은 콜백에서 처리한다.
    switch (key) {
    case GLFW_KEY_Q: case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true); break;
    case GLFW_KEY_B:
        beltMoving = !beltMoving; break;
    case GLFW_KEY_P:
        perspectiveProjection = !perspectiveProjection; break;
    case GLFW_KEY_R:
        resetRequested = true; break;
    case GLFW_KEY_EQUAL:
    case GLFW_KEY_KP_ADD:
        if (key == GLFW_KEY_KP_ADD || (mods & GLFW_MOD_SHIFT)) cameraDistance = max(7.0f, cameraDistance - 1.0f);
        break;
    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
        cameraDistance = min(24.0f, cameraDistance + 1.0f); break;
    }
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(0.79f, 0.86f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    //--- 카메라는 높이와 거리 값을 고정하고 y축 각도만 바꾸어 쿼터뷰를 유지한다.
    glm::vec3 cameraPosition(cameraDistance * 0.62f, cameraDistance * 0.72f,
        cameraDistance * 0.62f);
    glm::mat4 cameraRotation = glm::rotate(glm::mat4(1.0f), cameraAngle, glm::vec3(0, 1, 0));
    cameraPosition = glm::vec3(cameraRotation * glm::vec4(cameraPosition, 0.0f));
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0, 1, 0));
    float aspect = float(screenWidth) / float(screenHeight);
    glm::mat4 projection;
    if (perspectiveProjection)
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    else {
        float halfHeight = cameraDistance * 0.48f;
        projection = glm::ortho(-halfHeight * aspect, halfHeight * aspect,
            -halfHeight, halfHeight, 0.1f, 100.0f);
    }
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);

    //--- 바닥은 xz 평면에 놓고, 벨트는 그 위 일정한 높이에 세 구간으로 만든다.
    glm::mat4 floor = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.14f, 0.25f));
    floor = glm::scale(floor, glm::vec3(14.0f, 0.25f, 12.0f));
    DrawCube(floor, glm::vec3(0.40f, 0.46f, 0.48f));

    const glm::vec3 beltColor(0.12f, 0.15f, 0.17f);
    glm::mat4 firstBelt = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, BELT_Y, 0));
    firstBelt = glm::scale(firstBelt, glm::vec3(5.0f, 0.12f, 1.3f));
    DrawCube(firstBelt, beltColor);
    glm::mat4 corner = glm::translate(glm::mat4(1.0f), glm::vec3(0.325f, BELT_Y, 0.325f));
    corner = glm::scale(corner, glm::vec3(1.3f, 0.12f, 1.3f));
    DrawCube(corner, beltColor);
    glm::mat4 secondBelt = glm::translate(glm::mat4(1.0f), glm::vec3(TURN_RADIUS, BELT_Y, 3.4f));
    secondBelt = glm::scale(secondBelt, glm::vec3(1.3f, 0.12f, 5.5f));
    DrawCube(secondBelt, beltColor);

    //--- 벨트 표면의 줄무늬도 얇은 큐브로 만들고, 시작할 때 벨트와 함께 흐르게 한다.
    float stripeShift = beltMoving ? fmodf(beltTime * BOX_SPEED, 0.7f) : 0.0f;
    for (int i = 0; i < 8; ++i) {
        float x = -4.9f + fmodf(i * 0.7f + stripeShift, 4.9f);
        glm::mat4 stripe = glm::translate(glm::mat4(1.0f), glm::vec3(x, BELT_Y + 0.075f, 0));
        stripe = glm::scale(stripe, glm::vec3(0.075f, 0.035f, 1.18f));
        DrawCube(stripe, glm::vec3(0.72f, 0.75f, 0.73f));
    }
    for (int i = 0; i < 9; ++i) {
        float z = TURN_RADIUS + fmodf(i * 0.7f + stripeShift, 5.35f);
        glm::mat4 stripe = glm::translate(glm::mat4(1.0f), glm::vec3(TURN_RADIUS, BELT_Y + 0.075f, z));
        stripe = glm::scale(stripe, glm::vec3(1.18f, 0.035f, 0.075f));
        DrawCube(stripe, glm::vec3(0.72f, 0.75f, 0.73f));
    }

    //--- 다섯 상자의 경로 거리를 일정 간격으로 벌려서 순서가 유지되게 한다.
    const glm::vec3 packageColors[5] = {
        {0.92f, 0.25f, 0.18f}, {0.95f, 0.66f, 0.12f}, {0.20f, 0.62f, 0.32f},
        {0.18f, 0.48f, 0.86f}, {0.68f, 0.30f, 0.78f}
    };
    for (int i = 0; i < 5; ++i) {
        float distance = beltMoving ? beltTime * BOX_SPEED + i * PATH_LENGTH / 5.0f
            : i * PATH_LENGTH / 5.0f;
        glm::vec3 position;
        float yaw = 0.0f;
        GetParcelPose(distance, position, yaw);
        glm::mat4 boxRoot = glm::translate(glm::mat4(1.0f), position);
        boxRoot = glm::rotate(boxRoot, yaw, glm::vec3(0, 1, 0));
        glm::mat4 box = glm::scale(boxRoot, glm::vec3(BOX_SIZE));
        DrawCube(box, packageColors[i]);
        DrawPackageNumber(boxRoot, i + 1);
    }
}

void InputProcess(GLFWwindow* window)
{
    float now = float(glfwGetTime());
    float dt = previousFrameTime == 0.0f ? 0.0f : min(now - previousFrameTime, 0.05f);
    previousFrameTime = now;

    //--- 방향키는 누르고 있는 동안 카메라가 중심을 바라보며 좌우로 회전하게 한다.
    float turn = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ? 1.0f : 0.0f)
        - (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ? 1.0f : 0.0f);
    cameraAngle += turn * 0.9f * dt;
    if (beltMoving) beltTime += dt;

    if (resetRequested) {
        //--- r을 누르면 벨트, 상자, 카메라를 처음 배치로 되돌린다.
        beltMoving = false;
        beltTime = 0.0f;
        cameraAngle = 0.65f;
        cameraDistance = 13.0f;
        perspectiveProjection = true;
        resetRequested = false;
    }
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_20 - Parcel Logistics Center", nullptr, nullptr);
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
    glfwSetKeyCallback(window, KeyCallback);

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
