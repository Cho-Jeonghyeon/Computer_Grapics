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
#include <random>
#include <algorithm>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};
int screenWidth = WIDTH, screenHeight = HEIGHT;

void InputProcess(GLFWwindow* window);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

//--- 정점을 매번 바꾸는 대신, 지금 보여 줄 면만 기억해 두면 된다.
//--- 처음에는 육면체 전체를 보여 주고 키를 누르면 선택한 면으로 바꾼다.
bool drawCube = true;
bool cubeFaces[6] = { true, true, true, true, true, true };
bool pyramidFaces[5] = {}; // 앞, 뒤, 왼쪽, 오른쪽, 바닥
mt19937 randomEngine(random_device{}());

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

//--- 한 VAO에 육면체 36개, 사각뿔 18개, 좌표축 6개의 정점을 순서대로 넣는다.
//--- 그릴 때는 각 면의 시작 번호와 정점 개수만 골라서 넘기면 된다.
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
    const GLfloat faceColors[6][3] = {
        {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 1.0f},
        {0.2f, 0.8f, 0.2f}, {0.2f, 0.4f, 1.0f},
        {1.0f, 0.8f, 0.2f}, {0.8f, 0.2f, 1.0f}
    };
    GLfloat colors[60][3];
    for (int i = 0; i < 36; ++i)
        for (int j = 0; j < 3; ++j)
            colors[i][j] = faceColors[i / 6][j];

    //--- 같은 면의 정점은 같은 색으로 채워야 면 단위로 구분하기 쉽다.
    for (int i = 0; i < 18; ++i) {
        int face = i < 12 ? i / 3 : 5;
        for (int j = 0; j < 3; ++j)
            colors[36 + i][j] = faceColors[face][j];
    }
    //--- x는 빨강, y는 초록, z는 파랑으로 구분한다.
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
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    //--- 정면만 보면 입체가 잘 안 보여서 x, y축으로 각각 30도 기울인다.
    //--- 행렬은 오른쪽부터 적용되므로 정점에는 y 회전 후 x 회전이 적용된다.
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::mat4(1.0f);
    //--- 실습의 [-1, 1] 범위를 그대로 보려고 직교 투영을 사용한다.
    //--- 정사각형 뷰포트를 중앙에 두면 창 크기가 바뀌어도 도형이 늘어나지 않는다.
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -2.0f, 2.0f);
    int side = min(screenWidth, screenHeight);
    glViewport((screenWidth - side) / 2, (screenHeight - side) / 2, side, side);

    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(vao);
    //--- 축도 같은 방향으로 기울이면 도형과 좌표축의 관계를 볼 수 있다.
    glDrawArrays(GL_LINES, 54, 6);
    //--- 선택한 면만 그린다. 겹치는 부분은 깊이 검사가 앞뒤를 구분한다.
    if (drawCube) {
        for (int i = 0; i < 6; ++i)
            if (cubeFaces[i]) glDrawArrays(GL_TRIANGLES, i * 6, 6);
    } else {
        for (int i = 0; i < 4; ++i)
            if (pyramidFaces[i]) glDrawArrays(GL_TRIANGLES, 36 + i * 3, 3);
        if (pyramidFaces[4]) glDrawArrays(GL_TRIANGLES, 48, 6);
    }
}

//--- 면 선택은 키를 누르는 순간 한 번만 바뀌어야 하므로 콜백으로 처리한다.
void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    //--- REPEAT까지 받으면 키를 길게 누르는 동안 랜덤 면이 계속 바뀐다.
    if (action != GLFW_PRESS) return;
    bool cubeKey = key >= GLFW_KEY_1 && key <= GLFW_KEY_6;
    bool pyramidKey = (key >= GLFW_KEY_7 && key <= GLFW_KEY_9) || key == GLFW_KEY_0;
    if (!cubeKey && !pyramidKey && key != GLFW_KEY_C && key != GLFW_KEY_T) return;

    //--- 이번에 고른 면만 보여 주기 위해 이전 선택부터 지운다.
    for (bool& face : cubeFaces) face = false;
    for (bool& face : pyramidFaces) face = false;
    drawCube = cubeKey || key == GLFW_KEY_C;
    if (cubeKey) {
        cubeFaces[key - GLFW_KEY_1] = true;
    } else if (pyramidKey) {
        int face = key == GLFW_KEY_0 ? 3 : key - GLFW_KEY_7;
        pyramidFaces[face] = true;
    } else if (key == GLFW_KEY_C) {
        //--- 두 번째 번호는 첫 번째 면을 건너뛰게 해서 서로 다른 두 면을 고른다.
        int first = uniform_int_distribution<int>(0, 5)(randomEngine);
        int second = uniform_int_distribution<int>(0, 4)(randomEngine);
        if (second >= first) ++second;
        cubeFaces[first] = cubeFaces[second] = true;
    } else if (key == GLFW_KEY_T) {
        //--- 바닥은 항상 켜고, 네 옆면 중 하나만 추가한다.
        pyramidFaces[4] = true;
        pyramidFaces[uniform_int_distribution<int>(0, 3)(randomEngine)] = true;
    }
}

void InputProcess(GLFWwindow* window)
{
    //--- 종료는 현재 눌려 있는지만 확인하면 되므로 폴링으로 처리한다.
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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_13 - Face Selection", nullptr, nullptr);
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
    cout << "1~6: cube front/back/left/right/top/bottom\n"
         << "7/8/9/0: pyramid front/back/left/right\n"
         << "C: two random cube faces | T: pyramid base + one random side\n"
         << "Axes: X red, Y green, Z blue | Esc: exit\n";

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
