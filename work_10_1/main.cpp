//--- 실습 10 간단 버전: 조각 16개를 드래그해서 다섯 모양판을 완성한다.
//--- CPU에서 선택과 배치를 처리하고 VAO/VBO와 GLSL로 그린다.
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

const float WIDTH = 1600, HEIGHT = 900;
const float PI = 3.14159265f;
enum { SQUARE, TRIANGLE, RIGHT_TRIANGLE };
struct Point { float x, y; };
struct Piece {
    int type;
    float x, y, angle;
    float r, g, b;
    int slot; // -1이면 모양판 밖에 있는 조각.
};
struct Slot { int type; float x, y, angle; int board, piece; };
Piece pieces[16];
Slot slots[16];
int pieceCount = 0, slotCount = 0, selected = -1;
bool completed[5] = {};
float dragX = 0, dragY = 0;
GLuint shaderProgramID = 0, vao = 0, vbo[2] = {};

string filetobuf(const char* name)
{
    // 작업 폴더의 GLSL 파일을 읽는다.
    ifstream file(name);
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


int Vertices(int type, float x, float y, float angle, Point* points)
{
    // 정삼각형과 직각삼각형의 꼭짓점을 중심 기준으로 정의한다.
    Point local[4];
    int count = type == SQUARE ? 4 : 3;
    if (type == SQUARE) {
        local[0] = {-24,-24}; local[1] = {24,-24};
        local[2] = {24,24}; local[3] = {-24,24};
    } else if (type == TRIANGLE) {
        local[0] = {-24,-13.856406f}; local[1] = {24,-13.856406f};
        local[2] = {0,27.712812f};
    } else {
        local[0] = {-24,-48}; local[1] = {24,-48}; local[2] = {-24,48};
    }
    float c = cos(angle), s = sin(angle);
    for (int i = 0; i < count; ++i)
        points[i] = {x + local[i].x*c - local[i].y*s, y + local[i].x*s + local[i].y*c};
    return count;
}

bool Contains(const Piece& p, float x, float y)
{
    Point v[4]; int n = Vertices(p.type, p.x, p.y, p.angle, v);
    for (int i = 0; i < n; ++i) {
        Point a = v[i], b = v[(i+1)%n];
        if ((b.x-a.x)*(y-a.y) - (b.y-a.y)*(x-a.x) < -0.01f) return false;
    }
    return true;
}

void AddSlot(int board, int type, float x, float y, float angle = 0)
{
    slots[slotCount++] = {type, x, y, angle, board, -1};
}

void ResetScene()
{
    selected = -1; slotCount = 0; pieceCount = 0;
    fill(begin(completed), end(completed), false);
    //--- 과제 예시: 사각형 네 개, 바람개비, 직사각형.
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) AddSlot(0, SQUARE, 1033+x*54.0f, 680+y*54.0f);
    for (int i = 0; i < 4; ++i) {
        float angle = i*PI/2;
        AddSlot(1, TRIANGLE, 1370-42*sin(angle), 710+42*cos(angle), angle+PI);
    }
    AddSlot(2, RIGHT_TRIANGLE, 1060, 440);
    AddSlot(2, RIGHT_TRIANGLE, 1060, 440, PI);
    //--- 추가 모양판 1: 사각형 몸통과 정삼각형 지붕으로 만든 집.
    AddSlot(3, SQUARE, 1370, 425);
    AddSlot(3, TRIANGLE, 1370, 462.856406f);
    //--- 추가 모양판 2: 사각형 세 개와 정삼각형으로 만든 오른쪽 화살표.
    for (int i = 0; i < 3; ++i) AddSlot(4, SQUARE, 1100+i*48.0f, 190);
    AddSlot(4, TRIANGLE, 1233.856406f, 190, -PI/2);
    // 모양판에 필요한 만큼만 생성한다: 사각형 8개, 정삼각형 6개, 직각삼각형 2개.
    pieceCount = slotCount;
    for (int i = 0; i < pieceCount; ++i) {
        int type = slots[i].type;
        float angle = slots[i].angle;
        // 왼쪽 영역 안에서 위치와 색상을 무작위로 정한다.
        pieces[i] = {type, 60.0f+rand()%721,
            60.0f+rand()%781, angle,
            0.2f+(rand()%65)/100.0f, 0.2f+(rand()%65)/100.0f,
            0.2f+(rand()%65)/100.0f, -1};
    }
}

bool Locked(int index)
{
    return pieces[index].slot >= 0 && completed[slots[pieces[index].slot].board];
}

void CheckCompleted()
{
    for (int board = 0; board < 5; ++board) {
        bool full = true;
        for (int i = 0; i < slotCount; ++i)
            if (slots[i].board == board && slots[i].piece < 0) full = false;
        if (full && !completed[board]) cout << "Board " << board+1 << " complete!" << endl;
        completed[board] = full;
    }
}

void DropPiece(int index, float mouseX, float mouseY)
{
    Piece& p = pieces[index];
    int nearest = -1; float distance = 38*38;
    for (int i = 0; i < slotCount; ++i) {
        const Slot& s = slots[i];
        if (s.type != p.type || s.piece >= 0 || completed[s.board]) continue;
        // 직각삼각형 두 칸은 중심이 같다. 마우스를 놓은 삼각형 내부로 구분한다.
        if (p.type == RIGHT_TRIANGLE) {
            Piece target = {s.type, s.x, s.y, s.angle, 0, 0, 0, -1};
            if (Contains(target, mouseX, mouseY)) { nearest = i; break; }
            continue;
        }
        float d = (p.x-s.x)*(p.x-s.x)+(p.y-s.y)*(p.y-s.y);
        if (d < distance) { nearest = i; distance = d; }
    }
    if (nearest >= 0) {
        Slot& s = slots[nearest];
        p.x = s.x; p.y = s.y; p.angle = s.angle;
        p.slot = nearest; s.piece = index;
        CheckCompleted();
    }
}

Point MousePosition(GLFWwindow* window)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    // 마우스는 왼쪽 위가 원점이므로 y만 뒤집는다.
    return {float(x), HEIGHT-float(y)};
}

void CursorPosCallback(GLFWwindow* window, double, double)
{
    if (selected < 0) return;
    Point mouse = MousePosition(window);
    Piece& p = pieces[selected];
    p.x = mouse.x-dragX; p.y = mouse.y-dragY;

}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS) {
        Point mouse = MousePosition(window);
        for (int i = pieceCount-1; i >= 0; --i) {
            if (Locked(i) || !Contains(pieces[i],mouse.x,mouse.y)) continue;
            selected=i; dragX=mouse.x-pieces[i].x; dragY=mouse.y-pieces[i].y;
            if (pieces[i].slot >= 0) {
                slots[pieces[i].slot].piece=-1; pieces[i].slot=-1;
            }
            break;
        }
    } else if (action == GLFW_RELEASE && selected >= 0) {
        CursorPosCallback(window,0,0);
        Point mouse = MousePosition(window);
        DropPiece(selected,mouse.x,mouse.y); selected=-1;
    }
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_R) ResetScene();
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window,true);
}

void InitBuffer()
{
    glGenVertexArrays(1,&vao); glBindVertexArray(vao); glGenBuffers(2,vbo);
    for (int i=0;i<2;++i) {
        glBindBuffer(GL_ARRAY_BUFFER,vbo[i]);
        glBufferData(GL_ARRAY_BUFFER,12*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
        glVertexAttribPointer(i,3,GL_FLOAT,GL_FALSE,0,nullptr); glEnableVertexAttribArray(i);
    }
}

void DrawVertices(const Point* points, int n, GLenum mode, float r, float g, float b)
{
    float positions[12]={}, colors[12]={};
    for (int i=0;i<n;++i) {
        positions[i*3]=points[i].x/WIDTH*2-1; positions[i*3+1]=points[i].y/HEIGHT*2-1;
        colors[i*3]=r; colors[i*3+1]=g; colors[i*3+2]=b;
    }
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]); glBufferSubData(GL_ARRAY_BUFFER,0,n*3*sizeof(float),positions);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,n*3*sizeof(float),colors);
    glDrawArrays(mode,0,n);
}

void DrawPiece(const Piece& p)
{
    Point points[4]; int n=Vertices(p.type,p.x,p.y,p.angle,points);
    DrawVertices(points,n,GL_TRIANGLE_FAN,p.r,p.g,p.b);
    DrawVertices(points,n,GL_LINE_LOOP,0.15f,0.15f,0.15f);
}

void DrawScene()
{
    glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgramID); glBindVertexArray(vao);
    Point line[2]={{850,30},{850,870}};
    DrawVertices(line,2,GL_LINES,0.45f,0.55f,0.7f);
    for (int i=0;i<slotCount;++i) {
        Slot& s=slots[i]; Point points[4]; int n=Vertices(s.type,s.x,s.y,s.angle,points);
        DrawVertices(points,n,GL_TRIANGLE_FAN,0.93f,0.95f,0.98f);
        DrawVertices(points,n,GL_LINE_LOOP,0.2f,0.4f,0.65f);
    }
    for (int i=0;i<pieceCount;++i) if (i != selected) DrawPiece(pieces[i]);
    if (selected >= 0) DrawPiece(pieces[selected]);

}

int main()
{
    //--- GLFW와 OpenGL 초기화
    srand(unsigned(time(nullptr)));
    if (!glfwInit()) { cerr << "GLFW initialization failed" << endl; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE); // 창 크기를 고정한다.
    GLFWwindow* window=glfwCreateWindow(int(WIDTH),int(HEIGHT),
        "work_10_1 - Drag pieces | R: Reset | Q: Quit",nullptr,nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glewExperimental=GL_TRUE;
    if (glewInit()!=GLEW_OK) {
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    if (!InitShader()) {
        glDeleteProgram(shaderProgramID);
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    InitBuffer(); ResetScene(); glfwSwapInterval(1);

    //--- 처음에 한 번만 그리기 영역을 설정하고 입력 콜백을 등록한다.
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0,0,width,height);
    glfwSetKeyCallback(window,KeyCallback);
    glfwSetMouseButtonCallback(window,MouseButtonCallback);
    glfwSetCursorPosCallback(window,CursorPosCallback);

    //--- 입력 처리 -> 그리기 -> 화면 교체
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        DrawScene();
        glfwSwapBuffers(window);
    }

    //--- 사용한 OpenGL 객체와 창 정리
    glDeleteBuffers(2,vbo);
    glDeleteVertexArrays(1,&vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
