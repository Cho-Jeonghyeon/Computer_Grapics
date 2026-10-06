/*
    실습 9 전체 흐름부터 보기

    1. main(): GLFW 창과 OpenGL 3.3 Core 환경을 만든다.
    2. InitShader(): GLSL 두 파일을 읽고 컴파일해서 프로그램으로 연결한다.
    3. InitBuffer(): 위치/색상 VBO와 읽기 설정을 기억할 VAO를 만든다.
    4. ResetScene(): 삼각형 4개에 위치, 색상, 서로 다른 속도와 방향을 준다.
    5. 반복문: 입력 -> 경과 시간으로 이동 -> 그리기 -> 화면 교체.

    왼쪽 클릭 -> CreateTriangle: 클릭한 사분면 번호의 삼각형을 새로 만든다.
    오른쪽 클릭 -> ResizeTriangle: 해당 번호의 삼각형을 확대/축소한다.
    1/2/3/4 -> SetMovementMode: 튕기기/좌우 지그재그/상하 지그재그/나선.
    A/B: 면/테두리, C: 재생성, Q/Esc: 종료. 처음에는 1번 이동으로 시작한다.

    실습 8과 달라진 핵심은 UpdateTriangles(deltaTime)이다.
    CPU 배열의 위치를 매 프레임 바꾸고, DrawScene에서 VBO로 전달한다.
    이동 중에도 배열의 0~3번은 최초 사분면 번호를 유지한다.
    따라서 클릭은 현재 그곳에 지나가는 도형이 아닌 그 번호의 삼각형을 바꾼다.
*/
#define NOMINMAX // Windows의 min/max 매크로와 표준 함수 충돌 방지.

#include <Windows.h>

#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

//--- 실습 8의 도형 정보에 이동 상태를 추가한다. 구조체와 고정 배열을 그대로 사용한다.
struct Triangle {
    float x, y;
    float size;
    float r, g, b;
    bool growing;
    float speed, dx, dy; // 초당 속도, 가로/세로 방향 부호(+1 또는 -1).
    float headingX, headingY; // 꼭짓점이 바라볼 방향. 길이 1인 벡터.
    float turnRemaining; // 지그재그에서 세로로 더 이동할 거리.
    float radius, angle, spin, radialDirection; // 나선의 반지름, 각도, 회전/확대 방향.
};
Triangle triangles[4];
const float MIN_SIZE = 0.05f;
const float MAX_SIZE = 0.30f;
bool filled = true;
int movementMode = 1;
float windowAspect = float(WIDTH) / HEIGHT;
GLuint shaderProgramID = 0;
GLuint vao = 0, vbo[2] = {};

void InputProcess(GLFWwindow* window);
void DrawScene();

float CenterLimit(const Triangle& t)
{
    // 중심에서 가장 먼 정점까지 약 1.17*size. 회전해도 도형 전체가 보일 여유를 둔다.
    return 1.0f - t.size * 1.17f;
}

void KeepInside(Triangle& t)
{
    // 생성/확대로 가장자리를 넘으면 중심을 보정한다. clamp는 값을 구간 안으로 제한한다.
    float limit = CenterLimit(t);
    t.x = clamp(t.x, -limit, limit);
    t.y = clamp(t.y, -limit, limit);
}

void SetHeading(Triangle& t, float x, float y)
{
    // 길이로 나눠 방향을 정규화한다. 이동량이 0이면 이전 방향을 유지한다.
    x *= windowAspect; // 화면상의 실제 가로/세로 비율로 바라볼 방향을 구한다.
    float length = sqrt(x * x + y * y);
    if (length > 0.000001f) {
        t.headingX = x / length;
        t.headingY = y / length;
    }
}

void SetSpiralPosition(Triangle& t)
{
    // 가로로 긴 창에서도 나선이 타원이 되지 않도록 종횡비를 보정한다.
    float scaleX = min(1.0f, 1.0f / windowAspect);
    float scaleY = min(1.0f, windowAspect);
    t.x = t.radius * cos(t.angle) * scaleX;
    t.y = t.radius * sin(t.angle) * scaleY;
}

void StartSpiral(Triangle& t)
{
    // 화면 중앙이 나선 중심이다. 모드 전환 시 바깥 도형은 안전한 원 안으로 맞춘다.
    float x = t.x / min(1.0f, 1.0f / windowAspect);
    float y = t.y / min(1.0f, windowAspect);
    t.angle = atan2(y, x);
    t.radius = clamp(sqrt(x * x + y * y), 0.06f, CenterLimit(t));
    SetSpiralPosition(t);
}

string filetobuf(const char* name)
{
    // 실행 파일 옆의 GLSL을 우선 읽고, 실패하면 현재 작업 폴더에서 찾는다.
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
    buffer << file.rdbuf(); // 파일 전체를 문자열로 모은다.
    return buffer.str();
}

GLuint MakeShader(const char* name, GLenum type)
{
    string sourceText = filetobuf(name);
    if (sourceText.empty()) return 0;
    const char* source = sourceText.c_str(); // OpenGL에 넘길 C 문자열 주소.
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr); // 객체에 소스 문자열 1개를 넣는다.

    glCompileShader(shader); // GPU용 코드로 컴파일한다.

    GLint result;
    // 실패하면 오류 로그를 출력하고 실패한 객체를 정리한다.
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
    // 정점/프래그먼트 셰이더를 따로 컴파일한 다음 하나의 프로그램으로 링크한다.
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

    // 개별 셰이더는 삭제 예약해도 링크된 프로그램은 사용할 수 있다.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint result;
    // 개별 컴파일 성공과 링크 성공은 별개이므로 둘 다 검사한다.
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);

    if (!result) {
        char log[2048];
        glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log);
        cerr << "Shader link failed: " << log << endl;
    }
    return result == GL_TRUE;
}

void InitBuffer()
{
    // VAO는 VBO 읽기 설정을 기억한다. VBO 0은 위치, VBO 1은 색상.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);

        // 좌표축에 필요한 4정점 * 3성분 공간 확보. nullptr는 아직 내용이 없다는 뜻.
        glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        // i번 입력에서 float 3개씩 읽는다. GLSL의 layout(location=i)와 연결된다.
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(i);
    }
}

float RandomFloat(float low, float high)
{
    // rand()를 0~1 실수로 바꾼 다음 low~high 범위로 늘린다.
    return low + (high - low) * (float(rand()) / RAND_MAX);
}

int GetQuadrant(float x, float y)
{
    // 우상=0, 좌상=1, 좌하=2, 우하=3. 축 위는 오른쪽/위로 분류한다.
    if (y >= 0) return x >= 0 ? 0 : 1;
    return x < 0 ? 2 : 3;
}

void CreateTriangle(int index, float x, float y)
{
    Triangle& t = triangles[index]; // 참조(&)이므로 t를 바꾸면 배열 원본이 바뀐다.

    t.x = x;
    t.y = y;
    t.size = RandomFloat(0.10f, 0.22f);

    t.r = RandomFloat(0.1f, 0.85f);
    t.g = RandomFloat(0.1f, 0.85f);
    t.b = RandomFloat(0.1f, 0.85f);
    t.growing = true;
    // 속도 구간이 겹치지 않게 정해 네 삼각형의 속도를 서로 다르게 만든다.
    t.speed = 0.22f + index * 0.08f + RandomFloat(0.0f, 0.03f);
    // 처음 방향은 우상/좌상/좌하/우하. 새 도형의 이동 상태도 다시 설정한다.
    t.dx = (index == 0 || index == 3) ? 1.0f : -1.0f;
    t.dy = index < 2 ? 1.0f : -1.0f;
    t.headingX = 0;
    t.headingY = 1;
    t.turnRemaining = 0;
    t.spin = index % 2 == 0 ? 1.0f : -1.0f;
    t.radialDirection = index % 2 == 0 ? 1.0f : -1.0f;
    KeepInside(t);
    if (movementMode == 4) StartSpiral(t);
}

void ResetScene()
{
    // 사분면 중앙에 재생성한다. 현재 이동 모드와 면/선 선택은 유지한다.
    CreateTriangle(0,  0.5f,  0.5f);
    CreateTriangle(1, -0.5f,  0.5f);
    CreateTriangle(2, -0.5f, -0.5f);
    CreateTriangle(3,  0.5f, -0.5f);
}

void ResizeTriangle(int index)
{
    // 클릭마다 크기를 바꾸고 상한/하한을 만나면 증감 방향을 뒤집는다.
    Triangle& t = triangles[index];

    t.size += t.growing ? 0.025f : -0.025f;
    if (t.size >= MAX_SIZE) {
        t.size = MAX_SIZE;
        t.growing = false;
    }
    if (t.size <= MIN_SIZE) {
        t.size = MIN_SIZE;
        t.growing = true;
    }
    KeepInside(t);
    if (movementMode == 4) StartSpiral(t);
}

void BounceAxis(float& position, float& direction, float limit)
{
    // 벽을 넘은 거리만큼 되돌린다. limit=0.8, position=0.82라면 0.78.
    // 벽에 붙이는 것보다 FPS에 따른 이동 거리 손실이 적다.
    if (position > limit) {
        position = 2.0f * limit - position;
        direction = -abs(direction);
    } else if (position < -limit) {
        position = -2.0f * limit - position;
        direction = abs(direction);
    }
}

void MoveBounce(Triangle& t, float distance)
{
    // x/y를 함께 이동하고 벽을 만나면 해당 축의 방향만 반전한다.
    t.x += t.dx * distance * 0.8f;
    t.y += t.dy * distance * 0.6f;
    BounceAxis(t.x, t.dx, CenterLimit(t));
    BounceAxis(t.y, t.dy, CenterLimit(t));
    SetHeading(t, t.dx * 0.8f, t.dy * 0.6f);
}

void MoveHorizontalZigzag(Triangle& t, float distance)
{
    // 가로 이동 -> 벽에서 세로로 0.18 이동 -> 반대편으로 가로 이동을 반복한다.
    float limit = CenterLimit(t);
    // 한 프레임 도중 꺾여도 남은 거리는 다음 구간에서 이어서 소비한다.
    while (distance > 0.000001f) {
        if (t.turnRemaining > 0.000001f) {
            float room = t.dy > 0 ? limit - t.y : t.y + limit;
            // 이번 거리/남은 세로 구간/벽까지 거리 중 가장 작은 만큼만 움직인다.
            float step = min(distance, min(t.turnRemaining, room));
            t.y += t.dy * step;
            t.turnRemaining -= step;
            distance -= step;
            SetHeading(t, 0, t.dy);
            if (room <= step + 0.000001f) {
                // 위아래 끝에서는 세로 방향도 바꾸고 가로 이동으로 넘어간다.
                t.dy = -t.dy;
                t.turnRemaining = 0;
            }
        } else {
            float room = t.dx > 0 ? limit - t.x : t.x + limit;
            float step = min(distance, room);
            t.x += t.dx * step;
            distance -= step;
            SetHeading(t, t.dx, 0);
            if (room <= step + 0.000001f) {
                t.dx = -t.dx;
                t.turnRemaining = 0.18f;
            }
        }
    }
}

void MoveVerticalZigzag(Triangle& t, float distance)
{
    // 가로는 작게, 세로는 크게 이동해서 위아래로 뾰족한 경로를 만든다.
    t.x += t.dx * distance * 0.3f;
    t.y += t.dy * distance * 0.95f;
    BounceAxis(t.x, t.dx, CenterLimit(t));
    BounceAxis(t.y, t.dy, CenterLimit(t));
    SetHeading(t, t.dx * 0.3f, t.dy * 0.95f);
}

void MoveSpiral(Triangle& t, float distance)
{
    // 각도만 바꾸면 원 운동. 반지름도 함께 바꾸면 안팎으로 감기는 나선이 된다.
    float oldX = t.x, oldY = t.y;
    t.angle += t.spin * distance * 4.0f;
    t.radius += t.radialDirection * distance * 0.25f;
    float limit = CenterLimit(t);
    if (t.radius >= limit) {
        t.radius = 2.0f * limit - t.radius;
        // 화면에 들어가는 바깥 원을 만나면 안쪽으로 감고 회전 방향도 뒤집는다.
        t.radialDirection = -1;
        t.spin = -t.spin;
    } else if (t.radius <= 0.06f) {
        t.radius = 0.12f - t.radius;
        // 중심 가까이에서는 다시 바깥으로 펼친다.
        t.radialDirection = 1;
    }
    t.angle = fmod(t.angle, 6.2831853f); // 2*pi로 나눈 나머지로 각도의 무한 증가 방지.
    SetSpiralPosition(t);
    SetHeading(t, t.x - oldX, t.y - oldY);
}

void SetMovementMode(int mode)
{
    // 이전 꺾기 상태를 비우고, 필요하면 나선 좌표를 준비한다.
    movementMode = mode;
    for (Triangle& t : triangles) {
        t.turnRemaining = 0;
        if (mode == 4) StartSpiral(t);
    }
    cout << "Movement mode: " << mode << '\n';
}

void UpdateTriangles(float deltaTime)
{
    // 이동 거리 = 초당 속도 * 경과 시간. FPS가 달라도 비슷한 속도로 움직인다.
    for (Triangle& t : triangles) {
        float distance = t.speed * deltaTime;
        if (movementMode == 1) MoveBounce(t, distance);
        else if (movementMode == 2) MoveHorizontalZigzag(t, distance);
        else if (movementMode == 3) MoveVerticalZigzag(t, distance);
        else if (movementMode == 4) MoveSpiral(t, distance);
    }
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    // 누른 이벤트만 처리해서 버튼을 놓을 때 같은 동작이 다시 실행되지 않게 한다.
    if (action != GLFW_PRESS) return;
    if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT) return;
    int width, height;
    double mx, my;

    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    glfwGetCursorPos(window, &mx, &my);
    if (mx < 0 || mx >= width || my < 0 || my >= height) return;

    // 마우스 좌표를 중앙 기준 -1~1로 바꾸고 y 방향을 뒤집는다.
    float x = float(2.0 * mx / width - 1.0);
    float y = float(1.0 - 2.0 * my / height);
    int index = GetQuadrant(x, y);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
        CreateTriangle(index, x, y);
    else
        ResizeTriangle(index);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    // 실습 8의 키에 1~4 이동 선택을 추가한다. 길게 누를 때의 반복 이벤트는 무시한다.
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_A) filled = true;
    if (key == GLFW_KEY_B) filled = false;
    if (key == GLFW_KEY_C) ResetScene();
    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4)
        SetMovementMode(key - GLFW_KEY_1 + 1);
}

void UploadAndDraw(const float* positions, int count, GLenum mode, float r, float g, float b)
{
    // 정점마다 같은 색을 넣으면 삼각형 전체가 한 색으로 보인다.
    float colors[4][3];

    for (int i = 0; i < count; ++i) {
        colors[i][0] = r;
        colors[i][1] = g;
        colors[i][2] = b;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    // InitBuffer에서 확보한 공간에 실제로 그릴 정점 수만큼 내용을 덮어쓴다.
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), positions);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), colors);
    glDrawArrays(mode, 0, count); // 현재 VAO/셰이더로 정점 count개를 그린다.
}

void DrawScene()
{
    // 이전 화면을 지우고 셰이더/VAO를 선택한 뒤 현재 배열 상태를 그린다.
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);
    for (int i = 0; i < 4; ++i) {
        const Triangle& t = triangles[i];
        float w = t.size * 0.6f;
        float h = t.size;

        // 원점 기준 이등변삼각형을 진행 방향으로 회전한 뒤 중심(x,y)을 더한다.
        // heading=(0,1)이면 위쪽, (1,0)이면 오른쪽을 향한다.
        float positions[] = { -w,-h,0, w,-h,0, 0,h,0 };
        for (int j = 0; j < 3; ++j) {
            float x = positions[j * 3], y = positions[j * 3 + 1];
            // 회전 후 종횡비를 보정해 기울어져도 화면에서 이등변삼각형을 유지한다.
            positions[j * 3] = t.x + (x * t.headingY + y * t.headingX) * min(1.0f, 1.0f / windowAspect);
            positions[j * 3 + 1] = t.y + (-x * t.headingX + y * t.headingY) * min(1.0f, windowAspect);
        }

        // GL_TRIANGLES는 면, GL_LINE_LOOP는 닫힌 테두리.
        UploadAndDraw(positions, 3, filled ? GL_TRIANGLES : GL_LINE_LOOP, t.r, t.g, t.b);
    }

    // GL_LINES는 2정점씩 묶어 가로/세로축을 그린다. 축은 이동 경계가 아니다.
    float axes[] = { -1,0,0, 1,0,0, 0,-1,0, 0,1,0 };

    UploadAndDraw(axes, 4, GL_LINES, 0.55f, 0.65f, 0.8f);
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // GLFW -> 창/컨텍스트 -> GLEW -> 셰이더/VBO 순서로 초기화한다.
    srand(unsigned(time(nullptr)));

    if (!glfwInit()) { cerr << "Failed to initialize GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_9 - Moving Triangles", nullptr, nullptr);

    if (!window) {
        cerr << "Failed to create window\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!InitShader()) {
        if (shaderProgramID) glDeleteProgram(shaderProgramID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    InitBuffer();
    ResetScene();
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {

        glViewport(0, 0, width, height);
        if (width > 0 && height > 0) {
            windowAspect = float(width) / height;
            if (movementMode == 4)
                for (Triangle& t : triangles) StartSpiral(t);
        }
    });
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    if (width > 0 && height > 0) windowAspect = float(width) / height;
    cout << "Left click: replace triangle in quadrant | Right click: resize\n"
        << "A: filled | B: outline | C: reset all | Q / Esc: exit\n"
        << "1: bounce | 2: horizontal zigzag | 3: vertical zigzag | 4: spiral\n"
        << "Quadrant slots: upper-right 1, upper-left 2, lower-left 3, lower-right 4\n";
    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        // 창을 오래 끌거나 멈췄다가 돌아와도 한 번에 크게 건너뛰지 않게 제한한다.
        float deltaTime = min(float(currentTime - previousTime), 0.05f);
        previousTime = currentTime;

        glfwPollEvents(); // 입력 이벤트와 등록한 콜백 처리.
        InputProcess(window); // Esc 상태 확인.
        UpdateTriangles(deltaTime); // CPU 배열 위치/방향 갱신.
        DrawScene(); // 정점을 VBO로 보내 그리기.
        glfwSwapBuffers(window); // 완성된 화면 표시.
    }

    // OpenGL 객체는 컨텍스트가 살아 있을 때 지우고, 다음에 창과 GLFW를 정리한다.
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
