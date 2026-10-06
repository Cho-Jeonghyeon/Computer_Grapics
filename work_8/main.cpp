/*
    실습 8 전체 흐름부터 보기

    1. main(): GLFW 창과 OpenGL 작업 환경을 만든다.
    2. InitShader(): vertex.glsl + fragment.glsl을 읽고 GPU용 프로그램으로 만든다.
    3. InitBuffer(): 정점 위치/색상을 넣을 VBO와 읽는 방법을 기억할 VAO를 만든다.
    4. ResetScene(): CPU의 triangles 배열에 삼각형 4개의 정보를 저장한다.
    5. 반복문: 입력 처리 -> DrawScene() -> 완성된 화면 보여주기.

    왼쪽 클릭 -> MouseButtonCallback -> CreateTriangle -> 배열 값 교체
    오른쪽 클릭 -> MouseButtonCallback -> ResizeTriangle -> size 값 변경
    다음 DrawScene -> 바뀐 배열로 정점 계산 -> VBO에 전달 -> GPU가 그리기

    즉, 마우스 함수는 삼각형 정보를 바꾸고, 실제 그리기는 DrawScene이 한다.
    triangles는 CPU 쪽 데이터, VBO는 OpenGL이 관리하는 정점 데이터 저장소다.
    배열 값을 바꿨다고 VBO가 저절로 바뀌지는 않는다. UploadAndDraw가 전달한다.
*/
#define NOMINMAX // Windows.h가 min/max 매크로를 정의하지 않도록 한다. 헤더보다 먼저 적는다.
//--- 실행 파일의 경로를 알아내는 GetModuleFileNameW를 쓰기 위한 헤더.
#include <Windows.h>
//--- GLEW는 OpenGL 함수 로딩, GLFW는 창/마우스/키보드 담당. GLEW를 먼저 포함한다.
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>   // cout: 안내 출력, cerr: 오류 출력
#include <fstream>    // ifstream: GLSL 파일을 읽는다.
#include <sstream>    // ostringstream: 파일 내용을 하나의 문자열로 모은다.
#include <filesystem> // 경로에서 폴더를 구하고 파일 이름을 붙인다.
#include <cstdlib>    // rand, srand, RAND_MAX: 난수 생성
#include <ctime>      // time: 실행할 때마다 난수 시작값을 바꾸는 데 사용

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

//--- 배열 0, 1, 2, 3번은 각각 1, 2, 3, 4사분면이다.
// struct는 삼각형 하나에 필요한 값들을 한 묶음으로 관리하기 위한 자료형이다.
// 여기에는 정점 3개 자체가 아니라, 정점을 계산할 재료인 위치/크기/색상을 저장한다.
struct Triangle {
    float x, y;           // 삼각형을 감싸는 사각형의 중심. 삼각형의 무게중심과는 다르다.
    float size;           // 반높이. 전체 높이 = size * 2, 반너비 = size * 0.6.
    float r, g, b;        // 빨강/초록/파랑의 세기. 이 코드에서는 0~1 실수를 쓴다.
    bool growing;         // true: 다음 클릭 때 키워라. false: 다음 클릭 때 줄여라.
};
Triangle triangles[4];   // 사분면마다 한 칸씩. 같은 분면에서 다시 클릭하면 그 칸을 덮어쓴다.
const float MIN_SIZE = 0.05f; // 오른쪽 클릭으로 줄일 수 있는 size의 하한.
const float MAX_SIZE = 0.30f; // 오른쪽 클릭으로 키울 수 있는 size의 상한. const라 변경 불가.
bool filled = true;      // 모든 삼각형에 공통 적용. true: 면, false: 테두리.
GLuint shaderProgramID = 0; // GLuint는 OpenGL의 부호 없는 정수형. 프로그램 객체의 ID를 저장.
GLuint vao = 0, vbo[2] = {}; // vao: 읽기 설정 ID, vbo[0]: 위치 버퍼 ID, vbo[1]: 색상 버퍼 ID.
// 위 변수들에 정점 데이터 전체가 들어가는 것이 아니다. 객체를 가리키는 번호를 저장한다.
// = {}는 배열의 ID를 모두 0으로 초기화한다. 실제 객체는 InitBuffer에서 만든다.

//--- 함수 선언: 이 이름과 인자를 가진 함수가 뒤에서 정의된다고 컴파일러에게 알려준다.
void InputProcess(GLFWwindow* window);
void DrawScene();

//--- 실행 파일 옆의 셰이더를 읽는다. VS 작업 폴더에서도 읽을 수 있다.
// 이름은 filetobuf지만 반환값은 C++ string이다. 아직 컴파일하거나 GPU에 보내지 않는다.
string filetobuf(const char* name)
{
    wchar_t exePath[32768]; // 한글 경로도 담을 수 있는 넓은 문자 배열.
    DWORD length = GetModuleFileNameW(nullptr, exePath, 32768); // 현재 실행 중인 exe의 전체 경로.
    // 첫 인자 nullptr는 현재 프로그램을 뜻한다. length는 얻은 경로의 문자 수다.
    ifstream file; // 파일을 읽기 위한 객체. 여기서는 아직 파일을 열지 않았다.
    if (length > 0 && length < 32768)
        // parent_path(): exe 이름을 빼고 폴더만 구한다. / name: 그 폴더에 파일 이름을 붙인다.
        file.open(filesystem::path(exePath).parent_path() / name);
    if (!file.is_open()) {
        file.clear(); // 앞에서 파일 열기에 실패했다면 스트림의 오류 상태를 지운다.
        file.open(name); // 실행 파일 옆에서 못 찾았으면 현재 작업 폴더에서도 찾아본다.
    }
    if (!file) {
        cerr << "Cannot open shader: " << name << endl;
        return ""; // 빈 문자열로 실패를 알린다. MakeShader에서 empty()로 검사한다.
    }
    ostringstream buffer; // 문자열을 차곡차곡 모으는 출력 스트림.
    buffer << file.rdbuf(); // 파일의 남은 내용을 통째로 문자열 스트림에 옮긴다.
    return buffer.str(); // 모은 내용을 string으로 돌려준다. 지역 파일 객체는 함수 종료 시 닫힌다.
}

GLuint MakeShader(const char* name, GLenum type)    //vertex, fragment.glsl 파일을 컴파일해서 shader 객체를 만들어주는 함수

{
    string sourceText = filetobuf(name);    // 쉐이더 파일 읽기
    if (sourceText.empty()) return 0; // 파일이 없거나 비어 있으면 유효한 셰이더를 만들 수 없다.
    const char* source = sourceText.c_str();    //glShaderSource 이함수가 c++의 string 을 모르기 때문에 .c_str() 를 사용해서 const char*로 바꿔준다.
    GLuint shader = glCreateShader(type);       //OpenGL아 Vertex Shader 하나 만들어줘, 또는 Fragment Shader 하나 만들어줘. //반환값은 ID다. ex) Shader ID = 7
    //아직 이 시점에는 코드는 안 들어있다. 빈 쉐이더 객체만 만든 상태다.

    glShaderSource(shader, 1, &source, nullptr);    //방금 만든 shader 객체에 이 GLSL 코드를 넣어라.
    // 1: 소스 문자열 1개. &source: 문자열 포인터의 주소를 전달한다.
    // 마지막 nullptr: 별도의 길이 배열 없이 문자열 끝의 '\0'까지 읽으라는 뜻.
    // glShaderSource가 내용을 복사하므로 이 함수가 끝나 sourceText가 사라져도 괜찮다.
    glCompileShader(shader);    //GPU용 프로그램을 컴파일한다. 문법 오류가 있으면 여기서 실패한다.

    GLint result;   //컴파일 부분 검사
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);  //GL_COMPILE_STATUS == 이 쉐이더 컴파일 성공했냐?
    if (!result) {
        char log[2048]; // 드라이버가 알려 주는 컴파일 오류 메시지를 받을 공간.
        // sizeof(log): 최대 저장 공간. nullptr: 실제 메시지 길이는 따로 받지 않겠다.
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        cerr << name << ": " << log << endl;
        glDeleteShader(shader); // 실패한 객체는 정리한다.
        return 0; // 호출한 쪽에서 ID가 0인지 보고 실패 여부를 판단한다.
    }
    return shader; // GLSL 글자 자체가 아니라 컴파일에 성공한 셰이더 객체 ID를 반환한다.
}

bool InitShader()
{
    // vertex는 정점의 위치/속성을 처리하고, fragment는 그려질 조각의 색상을 출력한다.
    GLuint vertexShader = MakeShader("vertex.glsl", GL_VERTEX_SHADER);       //vertex, fragment.glsl 파일을 컴파일해서 shader 객체를 만들어주는 함수
    GLuint fragmentShader = MakeShader("fragment.glsl", GL_FRAGMENT_SHADER);
    if (!vertexShader || !fragmentShader) {
        // 둘 중 하나만 성공했어도 만들어진 객체는 지우고 끝낸다.
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }
    shaderProgramID = glCreateProgram();        //둘을 하나의 그래픽 파이프라인으로 묶어야 하기 때문이다. //Shader들을 묶을 프로그램 객체 하나 만들어라.
    glAttachShader(shaderProgramID, vertexShader);
    glAttachShader(shaderProgramID, fragmentShader);    //이제 프로그램 안에 두 쉐이더를 붙인다.

    glLinkProgram(shaderProgramID);     //붙인 쉐이더들을 실제로 하나의 실행 가능한 프로그램으로 연결해라.
                                        /*
                                            Vertex Shader ─┐
                                                            ├─ Link → Shader Program
                                             Fragment Shader┘
                                        */
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);     // 린크가 끝나면 완성된 프로그램 안에서 사용할 수 있게 됐기 때문에 개별 Shader 객체는 더 이상 필요 없다.
    // 정확히는 삭제 예약이다. 붙어 있는 셰이더는 연결이 해제될 때 실제로 삭제될 수 있다.
    // 링크된 프로그램은 이 호출 이후에도 사용할 수 있고, 마지막에 glDeleteProgram으로 정리한다.
    GLint result;                       //링크 검사
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
    // 개별 컴파일 성공과 프로그램 링크 성공은 별개다.
    // 예: vertex의 출력과 fragment의 입력 타입이 맞지 않으면 링크가 실패할 수 있다.
    if (!result) {
        char log[2048];
        glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log);
        cerr << "Shader link failed: " << log << endl;
    }
    return result == GL_TRUE; // 성공이면 true. main에서 실패 시 창을 정리하고 종료한다.
}

//--- 위치와 색상을 각각 VBO에 저장한다. 축은 4개, 삼각형은 3개 정점.
void InitBuffer()
{
    glGenVertexArrays(1, &vao); //VAO는 쉽게 말하면: 정점 데이터를 어떻게 읽어야 하는지 설정을 기억하는 객체 //VAO = VBO 사용 설명서
    glBindVertexArray(vao); // 이 VAO에 정점 속성 설정을 기록하겠다.
    glGenBuffers(2, vbo);   //VBO용 Buffer 객체 두 개 만들어라.
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);//“지금부터 이 객체를 작업 대상으로 삼겠다.”
        // 이건 GPU 메모리에 공간을 만든다. 4정점 × 정점당 float 3개 × float 크기.
        // work_7은 사각형을 삼각형 2개로 그려서 6정점이 필요했고, 여기서는 축의 4정점이면 충분하다.
        // nullptr: 아직 내용은 넣지 않고 공간만 확보. GL_DYNAMIC_DRAW: "이 데이터 자주 바뀔 거야."
        // 이 값은 사용 패턴을 알려 주는 힌트다. 데이터가 자동으로 갱신된다는 뜻은 아니다.
        glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, nullptr);    //현재 VBO의 데이터를 Vertex Shader 입력 i번에서 이런 형식으로 읽어라.
        // i: GLSL의 layout(location = i)와 연결. 0은 위치, 1은 색상.
        // 3: 한 정점당 성분 3개. GL_FLOAT: 각 성분은 float.
        // GL_FALSE: 정수 속성을 0~1 등으로 정규화하지 않음. 지금 float 데이터에는 영향이 없다.
        // stride 0: 다음 정점은 바로 뒤의 성분 3개. nullptr: 버퍼의 시작 위치(offset 0).
        // 이 호출이 현재 VBO와 읽는 형식의 연결을 VAO에 기록한다.
        glEnableVertexAttribArray(i);   //attribute를 실제로 활성화해야 한다.
    }
    /*
    VBO 0
    [x y z]
    [x y z]
    [x y z]
    ...

    VBO 1
    [r g b]
    [r g b]
    [r g b]
    ...

    VAO는 위 버퍼의 내용을 복사해서 가지는 것이 아니라, 연결과 읽기 설정을 기억한다.
    여기서는 공간/설정을 한 번 만들고, 그릴 때마다 UploadAndDraw에서 내용을 바꾼다.
    삼각형 4개를 그려도 VBO가 4세트 필요한 것은 아니다. 같은 버퍼를 차례대로 사용한다.
    */
}

float RandomFloat(float low, float high)
{
    // rand(): 0~RAND_MAX 정수 -> float로 바꾸고 나누면 0~1 실수.
    // 실수 변환 없이 정수끼리 나누면 대부분 0이 되므로 float(rand())가 필요하다.
    // 예: low=0.1, high=0.2, 비율=0.5라면 0.1 + 0.1*0.5 = 0.15.
    // 결과는 low~high 범위다. 무작위이므로 이전 값과 절대로 안 겹친다는 보장은 없다.
    return low + (high - low) * (float(rand()) / RAND_MAX);
}

int GetQuadrant(float x, float y)
{
    /*
           2사분면(index 1) | 1사분면(index 0)
           x<0, y>=0       | x>=0, y>=0
        -------------------+-------------------
           3사분면(index 2) | 4사분면(index 3)
           x<0, y<0        | x>=0, y<0

        반환값은 수학의 사분면 번호가 아니라 배열 인덱스(0~3)다.
        조건 ? 참일 때 값 : 거짓일 때 값 -> 삼항 연산자.
        경계 규칙: x=0은 오른쪽, y=0은 위쪽. 원점은 index 0에 들어간다.
    */
    if (y >= 0) return x >= 0 ? 0 : 1;
    return x < 0 ? 2 : 3;
}

void CreateTriangle(int index, float x, float y)
{
    Triangle& t = triangles[index]; // &는 참조. 배열 원본에 t라는 별명을 붙인다.
    // Triangle t처럼 복사하면 t만 바뀐다. 참조를 쓰므로 t.size를 바꾸면 배열 원본이 바뀐다.
    t.x = x;
    t.y = y;
    t.size = RandomFloat(0.10f, 0.22f);
    // 새로 생성할 때의 범위는 0.10~0.22. 오른쪽 클릭의 전체 허용 범위 0.05~0.30과 구분.
    t.r = RandomFloat(0.1f, 0.85f);
    t.g = RandomFloat(0.1f, 0.85f);
    t.b = RandomFloat(0.1f, 0.85f);
    t.growing = true;
    // 생성할 때마다 다음 크기 변경은 확대로 시작한다.
    // new나 배열 추가 없이 같은 칸을 덮어쓰므로 해당 분면에는 계속 삼각형 하나만 남는다.
    // 클릭 위치를 중심으로 쓰며 가장자리로 위치를 보정하지 않는다.
    // 따라서 축을 걸쳐 그려질 수 있고, 화면 밖으로 나간 부분은 OpenGL이 잘라낸다.
}

//--- 처음 실행하거나 C를 누르면 각 사분면 중앙에 새 삼각형을 만든다.
void ResetScene()
{
    // 이전 삼각형의 값을 모두 새 값으로 교체한다. VBO/VAO를 삭제하고 다시 만드는 것이 아니다.
    // 중심만 각 분면의 중앙으로 고정하고 크기/색상은 다시 랜덤으로 정한다.
    // filled는 바꾸지 않으므로 현재 면/테두리 모드는 유지된다.
    CreateTriangle(0,  0.5f,  0.5f);
    CreateTriangle(1, -0.5f,  0.5f);
    CreateTriangle(2, -0.5f, -0.5f);
    CreateTriangle(3,  0.5f, -0.5f);
}

//--- 클릭할 때마다 조금씩 커지다가 상한에서 축소로, 하한에서 확대로 바뀐다.
void ResizeTriangle(int index)
{
    Triangle& t = triangles[index];
    // size += (growing ? +0.025 : -0.025)와 같은 의미다.
    // 마우스 클릭 한 번에 한 단계 바뀐다. 자동으로 계속 움직이는 애니메이션은 아니다.
    t.size += t.growing ? 0.025f : -0.025f;
    if (t.size >= MAX_SIZE) {
        t.size = MAX_SIZE; // 예: 0.29 + 0.025 = 0.315가 되면 0.30으로 제한한다.
        t.growing = false; // 다음 클릭부터는 축소한다.
    }
    if (t.size <= MIN_SIZE) {
        t.size = MIN_SIZE; // 하한보다 작아지면 0.05로 제한한다.
        t.growing = true; // 다음 클릭부터는 확대한다.
    }
    // x/y와 r/g/b는 안 바꾸므로 중심과 색상은 그대로, 위아래/좌우 크기만 바뀐다.
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    // GLFW가 마우스 버튼 이벤트를 처리할 때 호출하는 함수. 등록은 main에서 한다.
    // button: 어느 버튼인지, action: 눌렀는지/놓았는지.
    // 마지막 이름 없는 int는 Shift/Ctrl 등의 조합 정보(mods). 여기서는 사용하지 않는다.
    if (action != GLFW_PRESS) return; // 놓는 이벤트는 무시해서 한 번 클릭에 두 번 바뀌지 않게 한다.
    if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT) return;
    int width, height;
    double mx, my;
    // 마우스 좌표와 단위를 맞추기 위해 창의 콘텐츠 영역 크기를 사용한다.
    // 그림을 그릴 실제 픽셀 크기(glfwGetFramebufferSize)와는 고해상도 배율에서 다를 수 있다.
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return; // 아래 좌표 계산에서 0으로 나누는 것을 막는다.
    glfwGetCursorPos(window, &mx, &my); // &를 통해 함수가 mx/my 변수에 좌표를 써 준다.
    if (mx < 0 || mx >= width || my < 0 || my >= height) return;
    //--- 마우스 좌표를 -1 ~ 1 좌표로 바꾸고 Y 방향을 뒤집는다.
    /*
        마우스: 왼쪽 위 (0,0), 오른쪽으로 x 증가, 아래쪽으로 y 증가.
        이 프로그램의 그리기 좌표: 중앙 (0,0), 오른쪽 +x, 위쪽 +y.

        x = 2 * mx / width - 1
            왼쪽 mx=0 -> -1, 중앙 mx=width/2 -> 0, 오른쪽 끝 -> 1.
        y = 1 - 2 * my / height
            위쪽 my=0 -> 1, 중앙 my=height/2 -> 0, 아래쪽 끝 -> -1.

        예: 1920x1080 창에서 (480,270)을 클릭하면 (-0.5,+0.5), 2사분면이다.
        이 식은 화면 전체를 viewport로 쓰고 별도 좌표 변환을 하지 않는 현재 코드 기준이다.
    */
    float x = float(2.0 * mx / width - 1.0);
    float y = float(1.0 - 2.0 * my / height);
    int index = GetQuadrant(x, y);
    // 삼각형 자체를 정확히 클릭할 필요는 없다. 클릭한 사분면의 삼각형을 대상으로 삼는다.
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        CreateTriangle(index, x, y); // 해당 사분면의 기존 삼각형을 교체한다.
    else
        ResizeTriangle(index);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    // 이름 없는 두 int는 scancode와 mods. 콜백 형식은 맞추되 안 쓰는 인자는 이름을 생략했다.
    // GLFW_REPEAT(길게 누를 때의 반복)도 무시한다. C를 길게 눌러 계속 랜덤 변경되는 것을 막는다.
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_A) filled = true;
    if (key == GLFW_KEY_B) filled = false;
    if (key == GLFW_KEY_C) ResetScene();
    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
}

void UploadAndDraw(const float* positions, int count, GLenum mode, float r, float g, float b)
{
    // positions: CPU에서 계산한 위치 배열의 시작 주소. const라 여기서 위치 배열을 수정하지 않는다.
    // count: 정점 개수(삼각형 3, 축 4), mode: 어떻게 이어 그릴지, r/g/b: 이번 도형의 색.
    float colors[4][3];
    // 최대 4정점까지 담는다. 실제로 쓸 count개만 채워서 그만큼만 GPU에 보낸다.
    for (int i = 0; i < count; ++i) {
        colors[i][0] = r;
        colors[i][1] = g;
        colors[i][2] = b;
    }
    // 정점마다 같은 RGB를 넣기 때문에 보간되어도 삼각형 전체가 한 색으로 보인다.
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // 실제 위치 데이터를 넣는다.
    // glBufferData는 공간 확보, glBufferSubData는 확보해 둔 공간의 일부 내용을 덮어쓰기.
    // offset 0부터 count * 3 * sizeof(float) 바이트를 positions에서 복사한다.
    // 삼각형이면 3정점 * 3성분 * 4바이트 = 36바이트(이 환경에서 float는 4바이트).
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), positions);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), colors);
    glDrawArrays(mode, 0, count); // 0번 정점부터 count개를 mode 방식으로 그려라.
    // 여기까지 해야 실제 그리기 명령이 나간다. 버퍼에 값을 넣는 것만으로는 그림이 나오지 않는다.
    // 현재 바인딩된 VAO와 현재 사용 중인 shaderProgramID를 이용한다.
}

void DrawScene()
{
    glClearColor(1, 1, 1, 1); // 지울 때 사용할 RGBA: 흰색, 알파 1. 이 줄만으로 지워지지는 않는다.
    glClear(GL_COLOR_BUFFER_BIT); // 이전 프레임의 색상 내용을 지운다. 매번 새 화면을 그리기 위함.
    glUseProgram(shaderProgramID);  //지금부터 그림 그릴 때 이 Shader Program을 사용해라.
    glBindVertexArray(vao); // InitBuffer에서 정해 둔 위치/색상 읽기 설정을 사용한다.
    for (int i = 0; i < 4; ++i) {
        const Triangle& t = triangles[i]; // 복사 없이 읽되, 그리는 동안 t를 통해 수정하지 못하게 const.
        float w = t.size * 0.6f;
        float h = t.size;
        /*
                       (x, y+h)
                          /\
                         /  \
                        /    \
               (x-w,y-h)------(x+w,y-h)

            왼쪽/오른쪽 꼭짓점이 중심 x에서 같은 거리 w만큼 떨어져 있어 이등변삼각형이다.
            높이는 2h, 밑변 길이는 2w. 모든 z는 0이므로 같은 평면 위에 그린다.
            size가 커지면 w와 h가 같은 비율로 커져 삼각형 모양은 유지된다.
        */
        float positions[] = {
            t.x - w, t.y - h, 0,   // 왼쪽 아래
            t.x + w, t.y - h, 0,   // 오른쪽 아래
            t.x,     t.y + h, 0    // 위쪽 꼭짓점: 이등변삼각형
        };
        // GL_TRIANGLES: 3정점으로 내부를 채운 삼각형. GL_LINE_LOOP: 0->1->2->0으로 테두리 연결.
        UploadAndDraw(positions, 3, filled ? GL_TRIANGLES : GL_LINE_LOOP, t.r, t.g, t.b); //UploadAndDraw()가 진짜 데이터를 넣는다
    }
    //--- x축과 y축을 마지막에 그려 사분면 경계가 항상 보이게 한다.
    float axes[] = { -1,0,0, 1,0,0, 0,-1,0, 0,1,0 };
    // GL_LINES는 정점을 두 개씩 묶는다. 0~1번은 가로축, 2~3번은 세로축.
    // 깊이 검사를 켜지 않은 현재 2D 코드에서는 나중에 그린 축이 겹친 도형 위에 보인다.
    UploadAndDraw(axes, 4, GL_LINES, 0.55f, 0.65f, 0.8f);
}

void InputProcess(GLFWwindow* window)
{
    // 콜백과 달리 매 프레임 직접 키 상태를 조회하는 방식이다(폴링).
    // Esc가 눌린 상태면 종료 표시를 켠다. 여기서 창을 즉시 파괴하는 것은 아니다.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    srand(unsigned(time(nullptr))); // 현재 시각으로 난수 시작값 설정. 매 프레임이 아니라 처음 한 번.
    //--- 창과 입력 기능을 쓰기 전에 GLFW부터 초기화한다. 실패하면 -1로 종료.
    if (!glfwInit()) { cerr << "Failed to initialize GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // 다음에 만들 창에 OpenGL 3.3 Core 환경을 요청한다. 정점 데이터 + 셰이더로 그리는 방식.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "work_8 - Quadrant Triangles", nullptr, nullptr);
    // 반환된 포인터가 이 창을 가리킨다. 뒤의 nullptr 두 개: 전체화면 모니터 없음, 공유 컨텍스트 없음.
    if (!window) {
        cerr << "Failed to create window\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window); // 이 스레드에서 OpenGL 명령을 보낼 작업 환경을 이 창으로 정한다.
    glewExperimental = GL_TRUE; // GLEW가 지원 가능한 확장 함수 진입점을 적극적으로 로딩하도록 설정.
    // GLEW 초기화는 컨텍스트를 현재 것으로 만든 다음에 한다. 순서가 바뀌면 함수 로딩에 실패할 수 있다.
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!InitShader()) { // 파일 읽기/컴파일/링크 중 실패하면 그리기를 진행하지 않는다.
        if (shaderProgramID) glDeleteProgram(shaderProgramID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    InitBuffer(); // GPU 쪽 데이터 공간과 읽기 설정 준비.
    ResetScene(); // CPU 쪽 삼각형 4개의 초기 상태 준비.
    glfwSwapInterval(1); // 화면 교체를 모니터 갱신에 맞추도록 요청(VSync).
    // 콜백 등록은 지금 실행하라는 것이 아니라, 해당 이벤트가 오면 이 함수를 호출하라는 예약이다.
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        // []로 시작하는 함수는 이름 없는 함수(람다). 바깥 변수를 캡처하지 않으므로 []가 비어 있다.
        glViewport(0, 0, width, height); // 실제 그리기 영역을 새 프레임버퍼 전체 크기로 바꾼다.
    });
    int width, height;
    // 창 크기 변경 이벤트를 기다리지 않고 처음부터 올바른 viewport를 설정한다.
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    cout << "Left click: replace triangle in quadrant | Right click: resize\n"
        << "A: filled | B: outline | C: reset all | Q / Esc: exit\n";
    while (!glfwWindowShouldClose(window)) {
        //--- 아래 네 단계를 종료 표시가 켜질 때까지 반복한다. 한 바퀴가 한 프레임이다.
        glfwPollEvents(); // 쌓인 입력/창 이벤트를 처리한다. 등록한 콜백도 이 과정에서 호출된다.
        InputProcess(window); // Esc 키의 현재 상태 확인.
        DrawScene(); // 현재 triangles 배열의 내용으로 다음 화면을 그린다.
        glfwSwapBuffers(window); // 뒤쪽 버퍼에 그린 화면을 표시한다(기본 더블 버퍼링).
    }
    // Q/Esc/창 닫기로 종료 표시가 켜지면 while을 빠져나와 여기에 도착한다.
    // OpenGL 자원은 컨텍스트가 살아 있을 때 정리해야 하므로 창 파괴보다 먼저 삭제한다.
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgramID);
    glfwDestroyWindow(window); // 창과 그 창의 OpenGL 컨텍스트를 정리한다.
    glfwTerminate(); // GLFW가 사용한 나머지 자원을 정리한다.
    return 0; // 운영체제에 정상 종료를 알린다.
}
