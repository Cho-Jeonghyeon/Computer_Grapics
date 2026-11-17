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
constexpr int SPHERE_STACKS = 20;
constexpr int SPHERE_SLICES = 32;
constexpr int SPHERE_COUNT = SPHERE_STACKS * SPHERE_SLICES * 6;
//--- 두 번째(초록) 구를 중심으로 돌며, 오른쪽의 세 번째 구와의 틈을 지난다.
constexpr float lightRadius = 1.0f;
int lightColorIndex = 0;
const glm::vec3 LIGHT_COLORS[3] = {
    {1.0f, 1.0f, 1.0f}, {1.0f, 0.35f, 0.20f}, {0.25f, 0.50f, 1.0f}
};
const glm::vec3 cameraPos(0.0f, 1.5f, 7.0f);

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

//--- work_18의 위도/경도 구 생성 방식을 사용하되, work_14의 VAO/VBO 흐름을 유지한다.
void InitBuffer()
{
    constexpr int vertexCount = SPHERE_STACKS * SPHERE_SLICES * 6;
    GLfloat positions[vertexCount][3] = {};
    GLfloat colors[vertexCount][3] = {};
    GLfloat normals[vertexCount][3] = {};
    int index = 0;
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
            glm::vec3 triangle[6] = { p00, p10, p11, p00, p11, p01 };
            for (const glm::vec3& position : triangle) {
                //--- 단위 구는 중심에서 표면으로 향하는 벡터가 곧 정점 법선이다.
                //--- 정점 법선을 보간하면 삼각형 경계에서도 부드럽게 조명 계산이 된다.
                for (int j = 0; j < 3; ++j) {
                    positions[index][j] = position[j];
                    colors[index][j] = 1.0f;
                    normals[index][j] = position[j];
                }
                ++index;
            }
        }
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(3, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
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

    //--- 하나의 구 메시를 재사용한다. 각 구의 위치/크기/재질색만 따로 전달한다.
    const glm::vec3 positions[3] = {
        {0.0f, 0.0f, 0.0f}, {-1.5f, 0.0f, 0.0f}, {2.2f, 0.0f, 0.0f}
    };
    const glm::vec3 colors[3] = {
        {0.15f, 0.85f, 0.20f}, {0.15f, 0.25f, 0.95f}, {0.95f, 0.12f, 0.10f}
    };
    const float sizes[3] = {0.50f, 0.35f, 0.70f};
    glUniform1i(lightingLoc, true);
    for (int i = 0; i < 3; ++i) {
        //--- T * S: 로컬 구를 확대/축소한 다음 각 위치로 옮긴다.
        glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]);
        model = glm::scale(model, glm::vec3(sizes[i]));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glUniform3fv(colorLoc, 1, &colors[i][0]);
        glDrawArrays(GL_TRIANGLES, 0, SPHERE_COUNT);
    }

    //--- 작은 구로 조명 위치를 표시한다. 색상 변경도 즉시 표시 도형에 반영한다.
    glUniform1i(lightingLoc, false);
    glUniform3fv(colorLoc, 1, &lightColor[0]);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), lightPos);
    model = glm::scale(model, glm::vec3(0.10f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, SPHERE_COUNT);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    bool upper = (mods & GLFW_MOD_SHIFT) != 0;
    switch (key) {
    case GLFW_KEY_C:
        lightColorIndex = (lightColorIndex + 1) % 3;
        lightColor = LIGHT_COLORS[lightColorIndex];
        break;
    case GLFW_KEY_R: {
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
}

void UpdateAnimation(float dt)
{
    lightAngle = fmod(lightAngle + lightRotation * 45.0f * dt, 360.0f);
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_27 - Phong Lighting", nullptr, nullptr);
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
    cout << "C: light color (white / warm red / blue)\nR / Shift+R: light orbit +/- (same key: stop) | Q / Esc: exit\n";

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
