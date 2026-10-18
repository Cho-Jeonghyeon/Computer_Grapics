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
#include <vector>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

constexpr int CUBE_FIRST = 0;
constexpr int CUBE_COUNT = 36;
constexpr int SPHERE_FIRST = CUBE_FIRST + CUBE_COUNT;
constexpr int SPHERE_STACKS = 16;
constexpr int SPHERE_SLICES = 24;
constexpr int SPHERE_COUNT = SPHERE_STACKS * SPHERE_SLICES * 6;
constexpr int AXIS_FIRST = SPHERE_FIRST + SPHERE_COUNT;
constexpr int AXIS_COUNT = 6;

struct ObjectState {
    glm::vec2 startPosition;
    float selfX = 0.0f;
    float selfY = 0.0f;
    float orbitY = 0.0f;
    float localScale = 1.0f;
    float originScale = 1.0f;
    glm::vec2 move = glm::vec2(0.0f);
};

ObjectState objects[2];
int selectedObjects = 3; // 1: 왼쪽, 2: 오른쪽, 3: 둘 다
bool swappedShapes = false;
float lastFrameTime = 0.0f;
float specialTime = 0.0f;
int specialAnimation = 0; // 0: 없음, 1: 원점을 지나 자리 바꾸기, 2: 위아래로 교차, 3: 크기/회전


void InputProcess(GLFWwindow* window);
void ResetScene();
float HeldDirection(GLFWwindow* window, int key);
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
void AddTriangle(vector<GLfloat>& positions, vector<GLfloat>& colors,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color)
{
    for (const glm::vec3& p : { a, b, c }) {
        positions.insert(positions.end(), { p.x, p.y, p.z });
        colors.insert(colors.end(), { color.r, color.g, color.b });
    }
}

void InitBuffer()
{
    vector<GLfloat> positions;
    vector<GLfloat> colors;
    const glm::vec3 faceColors[6] = {
        {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 1.0f}, {0.2f, 0.8f, 0.2f},
        {0.2f, 0.4f, 1.0f}, {1.0f, 0.8f, 0.2f}, {0.8f, 0.2f, 1.0f}
    };
    const glm::vec3 cubeFaces[6][6] = {
        {{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}},
        {{.5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f}},
        {{-.5f,-.5f,-.5f},{-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f}},
        {{.5f,-.5f,.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,-.5f,.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f}},
        {{-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}},
        {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,.5f}}
    };
    for (int face = 0; face < 6; ++face)
        for (int i = 0; i < 6; ++i) {
            const glm::vec3& p = cubeFaces[face][i];
            positions.insert(positions.end(), { p.x, p.y, p.z });
            colors.insert(colors.end(), { faceColors[face].r, faceColors[face].g, faceColors[face].b });
        }

    //--- 구는 위도/경도 칸마다 삼각형 두 개를 만들어 정점 버퍼에 이어 붙인다.
    const float pi = 3.1415926535f;
    for (int stack = 0; stack < SPHERE_STACKS; ++stack) {
        float phi0 = pi * (-0.5f + float(stack) / SPHERE_STACKS);
        float phi1 = pi * (-0.5f + float(stack + 1) / SPHERE_STACKS);
        for (int slice = 0; slice < SPHERE_SLICES; ++slice) {
            float theta0 = 2.0f * pi * float(slice) / SPHERE_SLICES;
            float theta1 = 2.0f * pi * float(slice + 1) / SPHERE_SLICES;
            auto point = [](float phi, float theta) {
                return glm::vec3(0.5f * cosf(phi) * cosf(theta), 0.5f * sinf(phi), 0.5f * cosf(phi) * sinf(theta));
            };
            glm::vec3 p00 = point(phi0, theta0), p01 = point(phi0, theta1);
            glm::vec3 p10 = point(phi1, theta0), p11 = point(phi1, theta1);
            glm::vec3 color(0.25f + 0.65f * float(stack) / SPHERE_STACKS,
                0.35f + 0.5f * float(slice) / SPHERE_SLICES, 0.9f);
            AddTriangle(positions, colors, p00, p10, p11, color);
            AddTriangle(positions, colors, p00, p11, p01, color);
        }
    }

    //--- 축은 로컬 원점에서 각 방향으로 뻗게 만들고, 그릴 때 도형과 같은 30도 기울기를 준다.
    const glm::vec3 axes[6] = {
        {-1.6f,0,0},{1.6f,0,0},{0,-1.6f,0},{0,1.6f,0},{0,0,-1.6f},{0,0,1.6f}
    };
    const glm::vec3 axisColors[3] = {{0.85f,0.1f,0.1f},{0.1f,0.65f,0.1f},{0.1f,0.2f,0.85f}};
    for (int axis = 0; axis < 3; ++axis)
        for (int end = 0; end < 2; ++end) {
            const auto& p = axes[axis * 2 + end];
            positions.insert(positions.end(), { p.x, p.y, p.z });
            colors.insert(colors.end(), { axisColors[axis].r, axisColors[axis].g, axisColors[axis].b });
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

    objects[0].startPosition = glm::vec2(-0.8f, 0.0f);
    objects[1].startPosition = glm::vec2(0.8f, 0.0f);
}

void DrawScene()
{
    if (screenWidth <= 0 || screenHeight <= 0) return;
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    float aspect = float(screenWidth) / float(screenHeight);
    glm::mat4 projection = aspect >= 1.0f
        ? glm::ortho(-2.4f * aspect, 2.4f * aspect, -2.4f, 2.4f, 0.1f, 20.0f)
        : glm::ortho(-2.4f, 2.4f, -2.4f / aspect, 2.4f / aspect, 0.1f, 20.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glBindVertexArray(vao);

    //--- 제출 자료의 흐름대로 기준 축과 물체 모두 X/Y축으로 30도 기울여 놓는다.
    glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1, 0, 0));
    tilt = glm::rotate(tilt, glm::radians(30.0f), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &tilt[0][0]);
    glDrawArrays(GL_LINES, AXIS_FIRST, AXIS_COUNT);

    float t = glm::clamp(specialTime / 2.0f, 0.0f, 1.0f);
    for (int i = 0; i < 2; ++i) {
        ObjectState& obj = objects[i];
        glm::vec2 position = obj.startPosition + obj.move;
        float animationScale = 1.0f;
        float animationOrbit = 0.0f;
        float animationSpin = 0.0f;
        float animationY = 0.0f;
        if (specialAnimation == 1 || specialAnimation == 2) {
            glm::vec2 destination = objects[1 - i].startPosition;
            position = glm::mix(obj.startPosition, destination, t) + obj.move;
            if (specialAnimation == 2)
                animationY = (i == 0 ? 1.0f : -1.0f) * sinf(t * 3.14159265f) * 0.7f;
        }
        if (specialAnimation == 3) {
            float wave = sinf(specialTime * 2.0f);
            animationScale = i == 0 ? 1.0f + 0.45f * wave : 1.0f - 0.35f * wave;
            animationOrbit = specialTime * (i == 0 ? 0.8f : -0.8f);
            animationSpin = specialTime * 1.6f;
        }

        //--- 행렬은 오른쪽부터 적용되므로 자기 회전/크기, 위치, 원점 기준 크기, 공전, 공통 기울기 순이다.
        glm::mat4 model = tilt;
        model = glm::rotate(model, glm::radians(obj.orbitY) + animationOrbit, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(obj.originScale)); // 위치까지 원점 기준으로 확대/축소
        model = glm::translate(model, glm::vec3(position.x, position.y + animationY, 0.0f));
        model = glm::rotate(model, glm::radians(obj.selfX), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(obj.selfY) + animationSpin, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(obj.localScale * animationScale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        bool drawCube = (i == 1) != swappedShapes;
        glDrawArrays(GL_TRIANGLES, drawCube ? CUBE_FIRST : SPHERE_FIRST,
            drawCube ? CUBE_COUNT : SPHERE_COUNT);
    }
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float now = float(glfwGetTime());
    float dt = lastFrameTime == 0.0f ? 0.0f : min(now - lastFrameTime, 0.05f);
    lastFrameTime = now;

    //--- 누르고 있는 동안 프레임 시간만큼 조금씩 바꾸면 컴퓨터 속도와 관계없이 조작감이 일정하다.
    for (int i = 0; i < 2; ++i) {
        if (!(selectedObjects & (1 << i))) continue;
        ObjectState& obj = objects[i];
        obj.selfX += HeldDirection(window, GLFW_KEY_X) * 90.0f * dt;
        obj.selfY += HeldDirection(window, GLFW_KEY_Y) * 90.0f * dt;
        obj.orbitY += HeldDirection(window, GLFW_KEY_R) * 90.0f * dt;
        obj.localScale = glm::clamp(obj.localScale + HeldDirection(window, GLFW_KEY_A) * dt, 0.2f, 2.5f);
        obj.originScale = glm::clamp(obj.originScale + HeldDirection(window, GLFW_KEY_B) * dt, 0.2f, 2.5f);
        //--- d는 왼쪽, Shift+d(대문자 D)는 오른쪽으로 움직이게 부호를 뒤집는다.
        obj.move.x -= HeldDirection(window, GLFW_KEY_D) * dt;
        obj.move.y += HeldDirection(window, GLFW_KEY_E) * dt;
    }
    if (specialAnimation == 1 || specialAnimation == 2) {
        specialTime += dt;
        if (specialTime >= 2.0f) {
            swap(objects[0].startPosition, objects[1].startPosition);
            objects[0].move = objects[1].move = glm::vec2(0.0f);
            specialAnimation = 0;
            specialTime = 0.0f;
        }
    }
    if (specialAnimation == 3) specialTime += dt;
}

float HeldDirection(GLFWwindow* window, int key)
{
    if (glfwGetKey(window, key) != GLFW_PRESS) return 0.0f;
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    return shift ? -1.0f : 1.0f;
}

void ResetScene()
{
    objects[0] = ObjectState{};
    objects[1] = ObjectState{};
    objects[0].startPosition = glm::vec2(-0.8f, 0.0f);
    objects[1].startPosition = glm::vec2(0.8f, 0.0f);
    selectedObjects = 3;
    swappedShapes = false;
    specialAnimation = 0;
    specialTime = 0.0f;
}

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    //--- 한번 눌렀을 때 선택/애니메이션 상태만 바꾸고, 계속 움직이는 입력은 아래 폴링에 맡긴다.
    if (key == GLFW_KEY_1) selectedObjects = 1;
    else if (key == GLFW_KEY_2) selectedObjects = 2;
    else if (key == GLFW_KEY_3) selectedObjects = 3;
    else if (key == GLFW_KEY_C) swappedShapes = !swappedShapes;
    else if (key == GLFW_KEY_S) ResetScene();
    else if (key == GLFW_KEY_T) { specialAnimation = 1; specialTime = 0.0f; }
    else if (key == GLFW_KEY_U) { specialAnimation = 2; specialTime = 0.0f; }
    else if (key == GLFW_KEY_V) { specialAnimation = specialAnimation == 3 ? 0 : 3; specialTime = 0.0f; }
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "cube", nullptr, nullptr);
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
    glfwSetKeyCallback(window, KeyCallback);
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
