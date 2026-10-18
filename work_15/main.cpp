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
#include <random>

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

//--- 도형 전체의 움직임과 각 면의 움직임은 따로 기억해야 한다.
bool drawCube = true, depthTest = true, spinObject = false;
float objectAngle = 0.0f;
bool spinTop = false, spinSide = false, frontOpen = false, backHidden = false;
float topAngle = 0.0f, sideAngle = 0.0f, frontAngle = 0.0f, backScale = 1.0f;
int groupMode = 0; // 0: 개별 면, 1: 모서리 회전, 2: 자리 교환, 4/5: 네 면 공전
bool groupRunning = false;
float groupAngle = 0.0f;
int pairFirst = 0, pairSecond = 3;
glm::vec3 edgePivot(0.5f, 0.0f, 0.5f);
const glm::vec3 faceCenters[6] = {
    {0,0,0.5f}, {0,0,-0.5f}, {-0.5f,0,0}, {0.5f,0,0}, {0,0.5f,0}, {0,-0.5f,0}
};
mt19937 randomEngine(random_device{}());

void ResetFaces()
{
    spinTop = spinSide = frontOpen = backHidden = false;
    topAngle = sideAngle = frontAngle = 0.0f;
    backScale = 1.0f;
    groupMode = 0;
    groupRunning = false;
    groupAngle = 0.0f;
}

//--- 회전축이 원점을 지나지 않으면 기준점을 원점으로 옮겨서 회전해야 한다.
//--- 정점에는 오른쪽부터 적용되므로 T(기준점) * R * T(-기준점) 순서다.
glm::mat4 RotateAt(glm::vec3 pivot, float angle, glm::vec3 axis)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pivot);
    transform = glm::rotate(transform, glm::radians(angle), axis);
    return glm::translate(transform, -pivot);
}

//--- 목표 각도나 크기를 넘지 않게 조금씩 접근시키면 열기/닫기에도 쓸 수 있다.
float Approach(float value, float target, float step)
{
    return value < target ? min(value + step, target) : max(value - step, target);
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

//--- 육면체 36개, 사각뿔 18개, 좌표축 6개를 한 버퍼에 저장한다.
//--- 면마다 같은 색을 배정하고, 그릴 때 해당 면의 정점 구간만 골라 쓴다.
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

//--- 각 면의 변환만 계산한다. 전체 도형의 회전은 DrawScene에서 나중에 곱한다.
glm::mat4 FaceTransform(int face)
{
    if (groupMode == 1 && (face == pairFirst || face == pairSecond))
        return RotateAt(edgePivot, groupAngle, glm::vec3(0,1,0));
    if (groupMode == 2 && (face == pairFirst || face == pairSecond)) {
        //--- 앞->오른쪽->뒤->왼쪽 순서가 +y 회전 방향이다.
        //--- 첫 면은 +90도, 다음 면은 -90도로 움직이면 서로의 자리에 도착한다.
        float angle = face == pairFirst ? groupAngle : -groupAngle;
        return RotateAt(glm::vec3(0), angle, glm::vec3(0,1,0));
    }
    if (groupMode == 4 && face < 4)
        return RotateAt(glm::vec3(0), groupAngle, glm::vec3(0,1,0));
    //--- x축 공전에는 윗면, 아랫면, 앞면, 뒷면을 사용한다.
    if (groupMode == 5 && (face == 0 || face == 1 || face == 4 || face == 5))
        return RotateAt(glm::vec3(0), groupAngle, glm::vec3(1,0,0));
    if (face == 4) return RotateAt(faceCenters[4], topAngle, glm::vec3(0,0,1));
    if (face == 0) return RotateAt(glm::vec3(0,0.5f,0.5f), -frontAngle, glm::vec3(1,0,0));
    if (face == 3) return RotateAt(faceCenters[3], sideAngle, glm::vec3(0,1,0));
    if (face == 1) {
        //--- 원점 기준으로 축소하면 뒷면이 중앙으로 이동한다. 면 중심을 기준으로 줄인다.
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), faceCenters[1]);
        transform = glm::scale(transform, glm::vec3(backScale));
        return glm::translate(transform, -faceCenters[1]);
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

    //--- 면을 펼치거나 모서리를 중심으로 돌려도 잘 보이도록 공간에 여유를 둔다.
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(-1.8f, 1.8f, -1.8f, 1.8f, -4.0f, 4.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);

    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(30.0f), glm::vec3(1,0,0));
    base = glm::rotate(base, glm::radians(30.0f), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &base[0][0]);
    glDrawArrays(GL_LINES, 54, 6);
    glm::mat4 object = glm::rotate(base, glm::radians(objectAngle), glm::vec3(0,1,0));
    if (drawCube) {
        for (int face = 0; face < 6; ++face) {
            //--- 면의 움직임을 먼저 적용한 뒤 전체 도형을 회전시킨다.
            glm::mat4 model = object * FaceTransform(face);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }
    } else {
        //--- 사각뿔은 전체 자전만 적용한다. 각 면의 색상은 버퍼에 이미 들어 있다.
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &object[0][0]);
        glDrawArrays(GL_TRIANGLES, 36, 18);
    }
}

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    //--- 토글 명령은 누르는 순간만 처리해서 길게 눌러도 여러 번 뒤집히지 않게 한다.
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_H) { depthTest = !depthTest; return; }
    if (key == GLFW_KEY_P) { drawCube = !drawCube; return; }
    if (key == GLFW_KEY_Y) { spinObject = !spinObject; return; }
    if (key == GLFW_KEY_R) {
        ResetFaces();
        drawCube = depthTest = true;
        spinObject = false;
        objectAngle = 0.0f;
        return;
    }
    if (!drawCube) return; //--- 아래 명령은 육면체의 면에만 적용한다.
    if (key == GLFW_KEY_T || key == GLFW_KEY_F || key == GLFW_KEY_S || key == GLFW_KEY_B) {
        //--- 그룹 변환 상태에서 개별 면을 조작하면 기준점이 달라져서 먼저 원상복구한다.
        if (groupMode != 0) ResetFaces();
        if (key == GLFW_KEY_T) spinTop = !spinTop;
        if (key == GLFW_KEY_F) frontOpen = !frontOpen;
        if (key == GLFW_KEY_S) spinSide = !spinSide;
        if (key == GLFW_KEY_B) backHidden = !backHidden;
    } else if (key == GLFW_KEY_1 || key == GLFW_KEY_2 || key == GLFW_KEY_4 || key == GLFW_KEY_5) {
        int mode = key - GLFW_KEY_0;
        if ((mode == 4 || mode == 5) && groupMode == mode) {
            groupRunning = !groupRunning;
            return;
        }
        ResetFaces();
        groupMode = mode;
        groupRunning = true;
        if (mode == 1 || mode == 2) {
            //--- 옆면을 둘레 순서로 놓고 연속한 두 면을 고르면 항상 모서리를 공유한다.
            const int ring[4] = {0, 3, 1, 2};
            int index = uniform_int_distribution<int>(0, 3)(randomEngine);
            pairFirst = ring[index];
            pairSecond = ring[(index + 1) % 4];
            //--- 인접한 두 면 중심을 더하면 공통 세로 모서리의 중점이 된다.
            edgePivot = faceCenters[pairFirst] + faceCenters[pairSecond];
            cout << "Selected faces: " << pairFirst << ", " << pairSecond << endl;
        }
    }
}

void InputProcess(GLFWwindow* window)
{
    //--- 종료는 현재 키 상태만 확인하면 되므로 폴링으로 처리한다.
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void UpdateAnimation(float dt)
{
    //--- 키 콜백에서 방향/목표만 정하고 실제 값은 매 프레임 조금씩 바꾼다.
    if (spinObject) objectAngle = fmod(objectAngle + 45.0f * dt, 360.0f);
    if (!drawCube) return; //--- 사각뿔을 보는 동안 육면체의 면 애니메이션은 잠시 멈춘다.
    if (spinTop) topAngle = fmod(topAngle + 60.0f * dt, 360.0f);
    if (spinSide) sideAngle = fmod(sideAngle + 60.0f * dt, 360.0f);
    frontAngle = Approach(frontAngle, frontOpen ? 90.0f : 0.0f, 60.0f * dt);
    backScale = Approach(backScale, backHidden ? 0.0f : 1.0f, 0.7f * dt);
    if (groupRunning) {
        if (groupMode == 1 || groupMode == 2) {
            float target = groupMode == 1 ? 360.0f : 90.0f;
            groupAngle = Approach(groupAngle, target, 60.0f * dt);
            if (groupAngle >= target) groupRunning = false;
        } else {
            groupAngle = fmod(groupAngle + 45.0f * dt, 360.0f);
        }
    }
}

int main()
{
    //--- GLFW, OpenGL 컨텍스트, GLEW 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_15 - Face Animation", nullptr, nullptr);
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
    cout << "H: depth | P: cube/pyramid | Y: spin | R: reset | Q/Esc: quit\n"
         << "T: top spin | F: front open/close | S: side spin | B: back shrink/grow\n"
         << "1: adjacent faces hinge 360 | 2: swap adjacent faces\n"
         << "4: four sides orbit Y | 5: top/bottom/front/back orbit X (press again to pause)\n";

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
        //--- 초당 변화량을 사용하고 긴 정지 뒤에는 갑자기 튀지 않도록 시간을 제한한다.
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
