#define NOMINMAX
#include <Windows.h>
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

//--- 세 가지 도형 메시를 한 VAO/VBO에 차례대로 넣고 시작 위치만 기억한다.
constexpr int CUBE_FIRST = 0;
constexpr int CUBE_COUNT = 36;
constexpr int SPHERE_STACKS = 20;
constexpr int SPHERE_SLICES = 32;
constexpr int SPHERE_FIRST = CUBE_FIRST + CUBE_COUNT;
constexpr int SPHERE_COUNT = SPHERE_STACKS * SPHERE_SLICES * 6;
constexpr int CIRCLE_SEGMENTS = 128;
constexpr int ORBIT_CIRCLE_FIRST = SPHERE_FIRST + SPHERE_COUNT;
constexpr int SCREEN_CIRCLE_FIRST = ORBIT_CIRCLE_FIRST + CIRCLE_SEGMENTS;

constexpr float PI = 3.1415926535f;
constexpr float PLANET_ORBIT_SPEED[3] = { 0.70f, 0.48f, 0.34f };
constexpr float PLANET_TILT[3] = { 0.0f, 45.0f, -45.0f };
constexpr float PLANET_ORBIT_RADIUS[3] = { 1.35f, 1.95f, 2.55f };
constexpr float MOON_ORBIT_RADIUS = 0.34f;
constexpr glm::vec3 PLANET_COLOR[3] = {
    glm::vec3(0.95f, 0.22f, 0.14f), glm::vec3(0.12f, 0.52f, 0.95f), glm::vec3(0.18f, 0.78f, 0.32f)
};
constexpr glm::vec3 MOON_COLOR[3] = {
    glm::vec3(1.0f, 0.72f, 0.42f), glm::vec3(0.68f, 0.82f, 1.0f), glm::vec3(0.78f, 1.0f, 0.68f)
};

glm::vec3 sceneOffset(0.0f);
float planetAngle[3] = { 0.0f, 2.0f, 4.0f };
float moonAngle[3] = { 0.0f, 2.1f, 4.2f };
float moonPathRotation[3] = { 0.0f, 1.3f, 2.6f };
float animationTime = 0.0f;
float previousFrameTime = 0.0f;
float centerPulseTime = 0.0f;
bool orbitSizeAnimation = false;
bool centerPulseAnimation = false;
bool moonOrbitAnimation = false;
bool perspectiveProjection = true;
bool wireframe = false;

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

    GLint result = GL_FALSE;
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

    GLint result = GL_FALSE;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
    if (!result) {
        char log[2048];
        glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log);
        cerr << "Shader link failed: " << log << endl;
    }
    return result == GL_TRUE;
}

void AddVertex(vector<GLfloat>& positions, vector<GLfloat>& colors,
    const glm::vec3& position, const glm::vec3& color)
{
    positions.insert(positions.end(), { position.x, position.y, position.z });
    colors.insert(colors.end(), { color.r, color.g, color.b });
}

void AddTriangle(vector<GLfloat>& positions, vector<GLfloat>& colors,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color)
{
    AddVertex(positions, colors, a, color);
    AddVertex(positions, colors, b, color);
    AddVertex(positions, colors, c, color);
}

//--- 메시를 한 번 만들어 GPU에 올리고, 매 프레임은 행렬만 바꿔 여러 번 그린다.
void InitBuffer()
{
    vector<GLfloat> positions;
    vector<GLfloat> colors;

    //--- cube 기본 코드의 면별 정점은 보관해서 같은 버퍼 구조를 유지한다.
    const glm::vec3 cubeFaces[6][6] = {
        {{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}},
        {{.5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f}},
        {{-.5f,-.5f,-.5f},{-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f}},
        {{.5f,-.5f,.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,-.5f,.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f}},
        {{-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}},
        {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,.5f}}
    };
    const glm::vec3 faceColors[6] = {
        {1.0f,.2f,.2f},{.2f,1.0f,1.0f},{.2f,.8f,.2f},
        {.2f,.4f,1.0f},{1.0f,.8f,.2f},{.8f,.2f,1.0f}
    };
    for (int face = 0; face < 6; ++face)
        for (int vertex = 0; vertex < 6; ++vertex)
            AddVertex(positions, colors, cubeFaces[face][vertex], faceColors[face]);

    //--- 구는 위도와 경도 칸마다 삼각형 두 개로 쪼개 표면을 근사한다.
    for (int stack = 0; stack < SPHERE_STACKS; ++stack) {
        float phi0 = -PI * 0.5f + PI * float(stack) / SPHERE_STACKS;
        float phi1 = -PI * 0.5f + PI * float(stack + 1) / SPHERE_STACKS;
        for (int slice = 0; slice < SPHERE_SLICES; ++slice) {
            float theta0 = 2.0f * PI * float(slice) / SPHERE_SLICES;
            float theta1 = 2.0f * PI * float(slice + 1) / SPHERE_SLICES;
            auto point = [](float phi, float theta) {
                return glm::vec3(cosf(phi) * cosf(theta), sinf(phi), cosf(phi) * sinf(theta));
            };
            glm::vec3 p00 = point(phi0, theta0), p01 = point(phi0, theta1);
            glm::vec3 p10 = point(phi1, theta0), p11 = point(phi1, theta1);
            const glm::vec3 white(1.0f);
            AddTriangle(positions, colors, p00, p10, p11, white);
            AddTriangle(positions, colors, p00, p11, p01, white);
        }
    }

    //--- 행성 궤도는 로컬 XZ 원, 달 궤도는 로컬 XY 원으로 따로 준비한다.
    for (int i = 0; i < CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * float(i) / CIRCLE_SEGMENTS;
        AddVertex(positions, colors, glm::vec3(cosf(angle), 0.0f, sinf(angle)), glm::vec3(1.0f));
    }
    for (int i = 0; i < CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * float(i) / CIRCLE_SEGMENTS;
        AddVertex(positions, colors, glm::vec3(cosf(angle), sinf(angle), 0.0f), glm::vec3(1.0f));
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
}

void SetObjectColor(const glm::vec3& color)
{
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "objectColor");
    glUniform3fv(colorLoc, 1, &color[0]);
}

void DrawSphere(const glm::mat4& model, const glm::vec3& color)
{
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    SetObjectColor(color);
    glDrawArrays(GL_TRIANGLES, SPHERE_FIRST, SPHERE_COUNT);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    //--- 켜고 끄는 기능은 키를 누른 순간 한 번만 콜백에서 바꾼다.
    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
    else if (key == GLFW_KEY_P) perspectiveProjection = !perspectiveProjection;
    else if (key == GLFW_KEY_M) wireframe = !wireframe;
    else if (key == GLFW_KEY_1) orbitSizeAnimation = !orbitSizeAnimation;
    else if (key == GLFW_KEY_2) centerPulseAnimation = !centerPulseAnimation;
    else if (key == GLFW_KEY_3) moonOrbitAnimation = !moonOrbitAnimation;
}

float Pressed(GLFWwindow* window, int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS ? 1.0f : 0.0f;
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float now = float(glfwGetTime());
    float dt = previousFrameTime == 0.0f ? 0.0f : min(now - previousFrameTime, 0.05f);
    previousFrameTime = now;
    animationTime += dt;

    //--- 행성은 각자 다른 속도로 계속 공전하고, 달은 3번 명령으로 공전을 켠다.
    for (int i = 0; i < 3; ++i) {
        planetAngle[i] += PLANET_ORBIT_SPEED[i] * dt;
        if (moonOrbitAnimation) {
            float speed = 1.8f + 0.35f * float(i);
            moonAngle[i] += speed * dt;
            moonPathRotation[i] += 0.45f * dt;
        }
    }
    if (centerPulseAnimation) centerPulseTime += dt;

    //--- WASD와 +/-는 누르는 동안 계속 적용하므로 프레임 시간으로 이동량을 맞춘다.
    glm::vec2 moveDirection(
        Pressed(window, GLFW_KEY_D) - Pressed(window, GLFW_KEY_A)
            + Pressed(window, GLFW_KEY_RIGHT) - Pressed(window, GLFW_KEY_LEFT),
        Pressed(window, GLFW_KEY_W) - Pressed(window, GLFW_KEY_S)
            + Pressed(window, GLFW_KEY_UP) - Pressed(window, GLFW_KEY_DOWN));
    if (glm::length(moveDirection) > 1.0f) moveDirection = glm::normalize(moveDirection);
    float zForward = Pressed(window, GLFW_KEY_KP_ADD) + Pressed(window, GLFW_KEY_EQUAL) *
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ? 1.0f : 0.0f);
    float zBackward = Pressed(window, GLFW_KEY_KP_SUBTRACT) + Pressed(window, GLFW_KEY_MINUS);
    glm::vec3 fullMoveDirection(moveDirection, zForward - zBackward);
    if (glm::length(fullMoveDirection) > 1.0f) fullMoveDirection = glm::normalize(fullMoveDirection);
    sceneOffset += fullMoveDirection * 1.5f * dt;
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(0.02f, 0.025f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    //--- 원근/직각 투영을 키로 바꾸되, 화면 비율과 깊이 범위는 창 크기에서 계산한다.
    glm::mat4 projection;
    float aspect = float(screenWidth) / float(screenHeight);
    if (perspectiveProjection)
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    else if (aspect >= 1.0f)
        projection = glm::ortho(-3.6f * aspect, 3.6f * aspect, -3.6f, 3.6f, 0.1f, 100.0f);
    else
        projection = glm::ortho(-3.6f, 3.6f, -3.6f / aspect, 3.6f / aspect, 0.1f, 100.0f);
    glm::vec3 cameraPosition(0.0f, 3.5f, 8.5f);
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glBindVertexArray(vao);

    float orbitScale = orbitSizeAnimation ? 1.0f + 0.28f * sinf(animationTime * 1.2f) : 1.0f;
    float centerScale = centerPulseAnimation ? 0.46f + 0.12f * (0.5f + 0.5f * sinf(centerPulseTime * 2.0f)) : 0.52f;
    glm::vec3 centerColor = centerPulseAnimation
        ? glm::mix(glm::vec3(1.0f, 0.42f, 0.08f), glm::vec3(1.0f, 0.92f, 0.25f), 0.5f + 0.5f * sinf(centerPulseTime * 2.0f))
        : glm::vec3(1.0f, 0.58f, 0.12f);

    //--- 카메라의 오른쪽/위쪽 방향을 구하면 달 궤도를 화면과 평행하게 둘 수 있다.
    glm::vec3 cameraForward = glm::normalize(-cameraPosition);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0, 1, 0)));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraForward));
    glm::mat4 screenBasis(1.0f);
    screenBasis[0] = glm::vec4(cameraRight, 0.0f);
    screenBasis[1] = glm::vec4(cameraUp, 0.0f);
    screenBasis[2] = glm::vec4(-cameraForward, 0.0f);

    //--- 먼저 모든 궤도를 그리면 깊이 버퍼가 구 뒤쪽의 선을 가려 준다.
    for (int i = 0; i < 3; ++i) {
        glm::mat4 orbitModel = glm::translate(glm::mat4(1.0f), sceneOffset);
        orbitModel = glm::rotate(orbitModel, glm::radians(PLANET_TILT[i]), glm::vec3(0, 0, 1));
        orbitModel = glm::scale(orbitModel, glm::vec3(PLANET_ORBIT_RADIUS[i] * orbitScale));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &orbitModel[0][0]);
        SetObjectColor(glm::vec3(0.22f, 0.27f, 0.42f));
        glDrawArrays(GL_LINE_LOOP, ORBIT_CIRCLE_FIRST, CIRCLE_SEGMENTS);
    }

    glm::vec3 planetPositions[3];
    for (int i = 0; i < 3; ++i) {
        float radius = PLANET_ORBIT_RADIUS[i] * orbitScale;
        glm::vec3 localPosition(radius * cosf(planetAngle[i]), 0.0f, radius * sinf(planetAngle[i]));
        glm::mat4 plane = glm::rotate(glm::mat4(1.0f), glm::radians(PLANET_TILT[i]), glm::vec3(0, 0, 1));
        planetPositions[i] = sceneOffset + glm::vec3(plane * glm::vec4(localPosition, 1.0f));
    }

    //--- 달은 행성과 같은 이동량을 받고, 화면에 평행한 원을 따라 돈다.
    for (int i = 0; i < 3; ++i) {
        glm::mat4 moonOrbitModel = glm::translate(glm::mat4(1.0f), planetPositions[i]);
        moonOrbitModel = moonOrbitModel * screenBasis;
        //--- 화면 평행 원을 X축으로 90도 기울여 깊이 방향으로 눕힌 궤도면을 만든다.
        moonOrbitModel = glm::rotate(moonOrbitModel, glm::radians(90.0f), glm::vec3(1, 0, 0));
        moonOrbitModel = glm::rotate(moonOrbitModel, moonPathRotation[i], glm::vec3(0, 0, 1));
        moonOrbitModel = glm::scale(moonOrbitModel, glm::vec3(MOON_ORBIT_RADIUS));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &moonOrbitModel[0][0]);
        SetObjectColor(glm::vec3(0.30f, 0.34f, 0.48f));
        glDrawArrays(GL_LINE_LOOP, SCREEN_CIRCLE_FIRST, CIRCLE_SEGMENTS);
    }

    //--- 중앙별, 행성, 달은 같은 구 메시를 크기와 색만 바꾸어 그린다.
    glm::mat4 centerModel = glm::translate(glm::mat4(1.0f), sceneOffset);
    centerModel = glm::scale(centerModel, glm::vec3(centerScale));
    DrawSphere(centerModel, centerColor);
    for (int i = 0; i < 3; ++i) {
        glm::mat4 planetModel = glm::translate(glm::mat4(1.0f), planetPositions[i]);
        planetModel = glm::scale(planetModel, glm::vec3(0.22f));
        DrawSphere(planetModel, PLANET_COLOR[i]);

        float phase = moonAngle[i] + (moonOrbitAnimation ? moonPathRotation[i] : 0.0f);
        glm::mat4 moonTilt = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
        glm::vec3 localMoonOffset = glm::vec3(moonTilt * glm::vec4(cosf(phase), sinf(phase), 0.0f, 0.0f));
        glm::vec3 screenMoonOffset = glm::vec3(screenBasis * glm::vec4(localMoonOffset, 0.0f));
        glm::vec3 moonPosition = planetPositions[i] + MOON_ORBIT_RADIUS * screenMoonOffset;
        glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), moonPosition);
        moonModel = glm::scale(moonModel, glm::vec3(0.095f));
        DrawSphere(moonModel, MOON_COLOR[i]);
    }
}

int main()
{
    //--- GLFW 창과 OpenGL 3.3 컨텍스트를 만든 뒤 GLEW로 함수 주소를 준비한다.
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_18 - Orbiting Spheres (P: projection, M: mode, 1-3: animation)", nullptr, nullptr);
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
    glEnable(GL_DEPTH_TEST); //--- 깊이값을 비교해서 뒤쪽 구와 선을 가린다.
    glEnable(GL_CULL_FACE);  //--- 구 표면의 카메라 반대쪽 삼각형도 그리지 않는다.
    glCullFace(GL_BACK);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        screenWidth = width;
        screenHeight = height;
        glViewport(0, 0, width, height);
    });
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();  //--- 콜백 입력을 전달한다.
        InputProcess(window); //--- 누르는 동안 계속 적용할 이동과 애니메이션 시간을 갱신한다.
        DrawScene();
        glfwSwapBuffers(window);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
