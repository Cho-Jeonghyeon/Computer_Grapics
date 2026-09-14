# OpenGL 강의 함수와 교수님 코드 스타일

작성일: 2026-09-14

## 사용 기준과 읽은 자료

- 사용자 요청: 앞으로 이 저장소의 OpenGL 실습은 강의자료에 나온 함수와 교수님 예제 스타일을 기반으로 작성한다.
- [지엘1 오픈지엘 기초.pdf](<지엘1 오픈지엘 기초.pdf>): 전체 60쪽.
- [지엘1 오픈지엘 기초 (실습 1 2 3).pdf](<지엘1 오픈지엘 기초 (실습 1 2 3).pdf>): 전체 7쪽.
- 두 자료의 전체 텍스트와 코드 예시를 추출해 읽었다. 설치 화면 이미지의 세부 UI는 시각 검증하지 않았다. 이하 페이지는 PDF 페이지 기준이다.
- **자료의 명시적 내용**, **예제에서 관찰한 스타일**, **구현을 위한 보완 판단**을 구분한다. 새 강의자료나 사용자 지시가 나오면 이를 우선하여 갱신한다.

## 수업의 기술 방향

| 구성 | 역할 | 적용 기준 |
| --- | --- | --- |
| C++ / Visual Studio | 응용 프로그램 작성 | 현재 C++ 실습 방식 유지 |
| OpenGL | GPU에 그리기 명령 전달 | 상태 설정 후 렌더링 |
| GLFW | 창, 컨텍스트, 입력, 이벤트, 시간 | 이번 실습의 창과 입력 처리 |
| GLEW | OpenGL 확장 함수 로딩 | 컨텍스트 활성화 후 초기화 |
| GLM | 벡터·행렬·변환 수학 | 뒤의 좌표계 변환 단계에서 적용 |
| GLSL | GPU에서 실행할 셰이더 언어 | 다음 GLSL 강의부터 도형 처리에 적용 |

기초 p.2-16: OpenGL 자체는 창이나 키보드 입력 기능을 제공하지 않는다. 컨텍스트는 현재 셰이더, VAO/VBO 바인딩, 텍스처, 깊이 검사, 뷰포트 등 상태와 리소스를 관리하는 실행 환경이다. 설정한 상태는 변경하기 전까지 이후 명령에도 영향을 준다.

파이프라인의 설명 순서는 정점 데이터 → 정점 처리 → 기본 도형 구성 → 래스터화 → 프래그먼트 처리 → 프레임버퍼이다. 버텍스 셰이더는 정점 위치 등을 처리하고 프래그먼트 셰이더는 픽셀 후보의 색상을 계산한다. VAO/VBO와 셰이더는 이번 자료에서 개념적으로 소개되며, 완성된 도형 구현 예제는 아직 없다.

### 이번 실습과 다음 단계의 프로파일 구분

- 실습 1의 배경색 변경은 자료의 OpenGL 3.3 Core 예제로 가능하다.
- **실습 2·3의 `glColor3f`, `glRectf`는 `GLFW_OPENGL_COMPAT_PROFILE`에서 사용한다.** 기초 p.52, 실습 p.6의 명시적 지시이다.
- 다음 GLSL 기초 이후에는 `GLFW_OPENGL_CORE_PROFILE`로 전환하고 셰이더로 정점 속성을 전달한다.
- 지금 사각형 실습에서는 위 두 함수를 우선한다. 임의로 셰이더/VAO/VBO 구현, GLUT·GLAD 기반 코드로 바꾸지 않는다.
- `glBegin`/`glEnd`는 고정 파이프라인 설명에 등장하지만 이번 실습에서 제시한 사각형 함수는 `glRectf`이다.

```cpp
//--- 이번 사각형 실습용 컨텍스트 설정
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
```

## 교수님 예제에서 관찰한 스타일

근거: 기초 p.17, 22-23, 29, 32, 36-42, 47, 53 / 실습 p.3-4.

- `main()`에 초기화 → 창 생성 → 컨텍스트 활성화 → GLEW 초기화 → 뷰포트/콜백 설정 → 메인 루프 → 종료 흐름이 드러난다.
- 입력과 그리기를 일반 함수로 나눈다. 실습의 대표 이름은 `InputProcess(...)`, `DrawScene(...)`이다. 다른 예제에는 `processInput`, `inputProcess`, `Render`도 있으므로 대소문자는 절대 규칙이 아니다.
- 콜백은 `KeyCallback`, `MouseButtonCallback`, `CursorPosCallback`, `FramebufferSizeCallback`처럼 역할이 보이는 이름을 사용한다.
- `//--- GLFW 초기화`, `//--- 메인 루프`, `//--- 키보드 입력 처리 함수`처럼 목적과 단계를 설명하는 한글 주석을 사용한다.
- `if`, `switch`, `while`과 단순 변수로 동작을 직접 보여준다. 예제에 별도 엔진이나 클래스 계층은 없다.
- 색상은 `rColor`, `gColor`, `bColor`처럼 직접 관리하며 실수 리터럴은 `0.0f`, `1.0f` 형태이다.
- 드래그 예제는 `static` 변수 `oldX`, `oldY`, `dragging`으로 이전 위치와 드래그 여부를 유지한다.
- 초기화 실패는 `std::cerr`로 표시하고 종료한다. 일부 축약 예제에는 오류 처리나 함수 선언이 생략되어 있다.
- 중괄호 배치는 혼재한다. 기존 파일과 일관되게 작성하되 함수 구조와 실행 흐름을 우선한다.

**구현 판단:** 작은 실습은 단일 `main.cpp`, 단순 구조체·고정 배열·반복문을 기본으로 한다. 이는 기존 사용자 선호와 예제의 단순성을 결합한 판단이며 교수님이 특정 자료구조나 전역 변수를 의무화한 것은 아니다. 입력 함수는 상태를 바꾸고 `DrawScene`은 저장된 상태를 매 프레임 그리도록 나누면 선택·드래그·리셋을 설명하기 쉽다.

## 기본 실행 순서

1. `glfwInit()` 성공 여부 확인.
2. `glfwWindowHint()`로 버전과 과제에 맞는 프로파일 지정.
3. `glfwCreateWindow(800, 600, ..., nullptr, nullptr)` 후 실패 검사.
4. `glfwMakeContextCurrent(window)`로 현재 컨텍스트 활성화.
5. `glewExperimental = GL_TRUE;` 설정 후 `glewInit() != GLEW_OK` 검사.
6. 프레임버퍼 크기에 맞게 `glViewport()` 설정. 필요한 콜백 등록.
7. 메인 루프에서 입력·시간 갱신·그리기·버퍼 교환·이벤트 처리 반복.
8. `glfwDestroyWindow(window)`, `glfwTerminate()`로 종료.

아래는 구조를 기억하기 위한 예시이며 독립 실행용 전체 코드가 아니다.

```cpp
//--- 메인 루프
while (!glfwWindowShouldClose(window))
{
    //--- 키보드 입력 처리
    InputProcess(window);

    //--- 타이머가 있으면 경과 시간에 따라 상태 갱신

    //--- 현재 상태를 화면에 그리기
    DrawScene();

    //--- 버퍼 교체와 이벤트 처리
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

대표 예제는 `glfwPollEvents()`를 루프 끝에 두지만 기초 p.22는 앞에 둔다. 하나를 절대 규칙으로 단정하지 않는다. 폴링 입력은 가장 최근 이벤트 처리에서 갱신된 상태를 읽는다.

## 강의 함수 목록

### 초기화·창·이벤트·시간 (기초 p.15-47)

| 함수 | 역할 |
| --- | --- |
| `glfwInit`, `glfwTerminate` | GLFW 시작·종료 |
| `glfwWindowHint` | 창 생성 전에 버전·프로파일·속성 지정 |
| `glfwCreateWindow`, `glfwDestroyWindow` | 창과 연결된 컨텍스트 생성·삭제 |
| `glfwMakeContextCurrent` | 현재 스레드의 컨텍스트 지정 |
| `glewInit` | 활성화된 컨텍스트에서 OpenGL 함수 로딩 |
| `glfwGetPrimaryMonitor` | 전체 화면 생성에 사용할 주 모니터 조회 |
| `glfwWindowShouldClose` | 종료 플래그 조회 |
| `glfwSetWindowShouldClose` | ESC/q 등에서 종료 플래그 설정 |
| `glfwPollEvents` | 이벤트 처리, 입력 상태와 콜백 갱신 |
| `glfwWaitEvents` | 이벤트가 올 때까지 대기 |
| `glfwSwapBuffers` | 그리기 완료 후 앞/뒤 버퍼 교환 |
| `glfwGetError` | 오류 코드 조회, 설명 문자열은 포인터 인자로 받음 |
| `glfwSetWindowTitle` | 창 제목 변경 |
| `glfwGetTime`, `glfwSetTime` | 시간 조회·설정, 타이머·애니메이션에 활용 |
| `glfwSwapInterval` | 현재 컨텍스트 설정 후 VSync 간격 지정. 예제는 `1` |

### 입력과 크기 조회 (기초 p.26-44)

| 조회 방식 | 콜백 등록 | 사용 목적 |
| --- | --- | --- |
| `glfwGetKey` | `glfwSetKeyCallback` | 키 상태 / 누름·놓음 이벤트 |
| 별도 문자 상태 조회 없음 | `glfwSetCharCallback` | 유니코드 문자 입력 |
| `glfwGetMouseButton` | `glfwSetMouseButtonCallback` | 버튼 상태 / 클릭·놓음 |
| `glfwGetCursorPos` | `glfwSetCursorPosCallback` | 커서 위치 / 마우스 이동 |
| 별도 휠 상태 조회 없음 | `glfwSetScrollCallback` | 수평·수직 스크롤 |
| `glfwGetWindowSize` | `glfwSetWindowSizeCallback` | 논리적인 창 크기 |
| `glfwGetFramebufferSize` | `glfwSetFramebufferSizeCallback` | 실제 렌더링 픽셀 크기 |
| `glfwGetWindowPos` | `glfwSetWindowPosCallback` | 화면에서 창의 위치 |
| `glfwWindowShouldClose` | `glfwSetWindowCloseCallback` | 종료 플래그 / 닫기 요청 |

```cpp
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void CharCallback(GLFWwindow* window, unsigned int codepoint);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
```

콜백은 선언·정의뿐 아니라 `glfwSetKeyCallback(window, KeyCallback)` 같은 등록도 필요하다.

### 그리기와 데이터 (기초 p.48-52, 실습 p.6)

| 함수 | 역할 |
| --- | --- |
| `glClearColor(r, g, b, a)` | 지울 때 쓸 배경색 설정. 실습 RGB는 0~1, alpha는 1 |
| `glClear(GL_COLOR_BUFFER_BIT)` | 컬러 버퍼 초기화 |
| `glClear`의 다른 마스크 | `GL_DEPTH_BUFFER_BIT`, `GL_STENCIL_BUFFER_BIT`도 소개됨 |
| `glViewport(x, y, width, height)` | 프레임버퍼의 렌더링 영역 설정. 좌측 아래 기준 |
| `glColor3f(r, g, b)` | 이후 레거시 도형에 사용할 현재 색상 설정 |
| `glRectf(x1, y1, x2, y2)` | 대각선 두 꼭짓점으로 사각형. 강의는 좌하단·우상단으로 설명 |

```cpp
//--- 사각형 색상 지정 후 그리기
glColor3f(1.0f, 0.0f, 0.0f);
glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
```

`glClearColor`만으로는 화면을 지우지 않는다. `glColor3f`는 배경색 설정과 다르다. 여러 사각형은 각각 색을 설정한 직후 그린다. `glUniform` 계열은 p.48의 함수 명명법 소개이며 현재 실습의 필수 구현이 아니다. `GLfloat`는 실수, `GLint`는 정수, `GLuint`는 부호 없는 정수, `GLenum`은 열거값, `GLbitfield`는 비트 조합 용도로 이해한다.

## 입력·시간·좌표에 적용할 보완 판단

- 연속 이동은 폴링으로 처리하기 쉽다. 한 번 생성·랜덤 변경·리셋·분할하는 명령은 콜백의 `action == GLFW_PRESS` 또는 폴링의 이전 키 상태 비교로 한 번만 실행한다.
- 무조건 매 프레임 생성하면 한 번 누르는 동안 여러 개가 생긴다. `GLFW_REPEAT`는 키 콜백의 반복 이벤트이며 `glfwGetKey()` 반환값과 혼동하지 않는다.
- 드래그는 클릭 때 대상을 기억하고, 이전·현재 커서 차이만큼 이동하고, 놓으면 해제한다. 기초 p.32의 `oldX/oldY/dragging` 흐름을 활용한다.
- 타이머는 `glfwGetTime()`과 마지막 갱신 시각을 비교한다. 루프를 긴 대기로 막지 않는다. 실습 1의 정확한 변경 주기는 자료에 없다.
- p.47의 `deltaTime = currentTime - lastTime`, `position += speed * deltaTime` 관계로 프레임 속도와 이동 속도를 분리할 수 있다.
- 좌표·색상·선택·개수·타이머 같은 지속 상태를 루프마다 초기화하지 않는다.

기초 p.49의 초기 화면은 중앙 `(0, 0)`, x는 오른쪽, y는 위쪽이 양의 방향이다. `[-1, 1]`은 NDC 화면 범위의 설명이다. 이후 변환을 배워도 모든 원본 정점이 무조건 이 범위여야 한다는 뜻으로 확대하지 않는다.

전체 창을 뷰포트로 사용하고 별도 변환이 없는 2D 실습의 커서 → NDC 변환은 다음처럼 보완할 수 있다. 이 식은 강의 원문 코드가 아니다.

```cpp
//--- width, height는 glfwGetWindowSize로 얻고 0보다 클 때 계산
float x = 2.0f * static_cast<float>(mouseX) / width - 1.0f;
float y = 1.0f - 2.0f * static_cast<float>(mouseY) / height;
```

커서는 창 콘텐츠 영역의 왼쪽 위를 기준으로 하므로 y를 뒤집는다. 커서와 같은 논리 단위의 창 크기를 사용하고, `glViewport`에는 프레임버퍼 크기를 사용한다. HiDPI에서는 두 크기가 다를 수 있다. 부분 뷰포트나 투영을 도입하면 변환도 조정한다. 근거: [GLFW 입력 안내](https://www.glfw.org/docs/latest/input_guide.html), [창·프레임버퍼 참조](https://www.glfw.org/docs/latest/group__window.html).

## 실습별 요구사항

### 실습 1: 배경색과 시간 (실습 p.2-4)

- GLFW, GLEW, GLM 설치. **800×600 창, 초기 배경색 흰색**.
- `c`: 청록 `(0, 1, 1)`, `m`: 자홍 `(1, 0, 1)`, `y`: 노랑 `(1, 1, 0)`.
- `a`: 랜덤색, `g`: 회색, `k`: 검정.
- `t`: 일정 시간마다 랜덤색으로 변경 시작, `s`: 타이머 종료, `Esc`: 프로그램 종료.
- 회색 밝기와 타이머 주기는 원문에 고정값이 없다.
- 예제의 파란 배경은 골격 코드이다. 실제 초기값은 문제 본문의 흰색을 따른다.

### 실습 2: 영역별 사각형과 선택 (실습 p.5-6)

- 배경색은 자유. 가로·세로를 각각 2등분하여 네 영역을 만들고 영역마다 랜덤한 서로 다른 색을 설정한다.
- `1/2/3/4`: 각 사분면 중앙에 사각형. 색상·크기는 자유. 다시 누르면 다른 크기와 색으로 그린다.
- **각 영역 최대 5개**.
- 왼쪽 클릭: 사각형 선택, 선택 표시 방법은 자유.
- `+/-`: 선택 사각형 확대/축소. `c`: 선택 사각형 색 랜덤 변경.
- `r`: 사각형 모두 삭제와 영역 색 변경. `q`: 종료.
- 원문에 “다시 그려진다”와 “영역당 최대 5개”가 함께 있다. 기존 도형 교체인지 누적 추가인지 구현 시 해석을 명시한다. 한도 초과 동작도 명시되어 있지 않다.
- 숫자와 화면 영역의 구체적인 좌표표가 없으므로 사용하는 사분면 번호 배치를 설명한다.

### 실습 3: 이동·합치기·분리 (실습 p.7)

- `a`: 랜덤 위치에 다른 색 사각형 생성. 이 명령으로 만드는 개수는 최대 10개라는 조건이 있다.
- 서로 겹칠 수 있고 **나중에 만든 사각형이 위에** 올라온다.
- 왼쪽 버튼을 사각형 위에서 누른 채 드래그하면 이동, 놓으면 이동 종료.
- 선택·드래그로 두 사각형이 겹치면 x/y 최솟값과 최댓값을 감싸는 큰 사각형으로 합치고 랜덤색 부여.
- 오른쪽 클릭: 해당 사각형을 2개로 분리. 크기와 색은 랜덤.
- **전체 개수 최대 20개**.
- 생성 명령 최대 10개와 분할 포함 전체 최대 20개를 별도로 기억한다. 10개가 누적 생성 횟수인지 현재 개수 기준인지는 원문만으로 확정하지 않는다.
- 합치기 시점(드래그 도중/놓을 때), 여러 도형 연쇄 병합, 분할 위치·범위, 한도 도달 처리는 미지정이다. 구현 시 선택한 해석을 기록한다.

**구현 판단:** 깊이 검사를 쓰지 않는 단순 2D에서는 생성 순서대로 그리고 클릭 판정은 역순으로 검사하면 맨 위 도형을 선택하기 쉽다. 병합 경계는 `min(x1)`, `min(y1)`, `max(x2)`, `max(y2)`로 구한다. 고정 배열을 쓴다면 용량 20개와 활성 개수를 분리한다. 원문이 이런 자료구조나 구현을 지정한 것은 아니다.

## 설치 방식 기억 (기초 p.54-60)

- 자료는 다운로드한 GLFW·GLEW·GLM을 Windows SDK의 include/lib 경로 등에 배치하는 방식을 설명한다.
- `<GL/glew.h>`, `<GL/glfw3.h>`는 GLFW 헤더까지 SDK의 `gl` 폴더에 넣은 배치를 전제로 한다. 배포본 폴더 구조를 유지한다면 실제 include 경로에 맞춘다.
- 예제처럼 GLEW 헤더를 GLFW 헤더보다 먼저 포함한다.
- 자료의 링크 입력은 `opengl32.lib glew32.lib glfw3.lib`이다.
- 이후 변환 단계의 GLM 헤더는 `<gl/glm/glm.hpp>`, `<gl/glm/ext.hpp>`, `<gl/glm/gtc/matrix_transform.hpp>`로 소개된다.
- SDK 버전 숫자와 GLEW 버전이 슬라이드 사이에서 다르다. 실제 설치 경로·x64 구성·선택한 라이브러리를 확인한다.
- 이번 기록 작업에서는 라이브러리 설치나 프로젝트 설정 변경을 하지 않았다. 설치 완료로 간주하지 않는다.

## 그대로 복제하지 않을 오타와 축약

| 위치 | 자료 표기 또는 상황 | 작성 시 처리 |
| --- | --- | --- |
| 기초 p.15 | `glfwDestroyWindAow` | `glfwDestroyWindow` |
| 기초 p.18 | `void glfwInit()` | 실제 반환형 `int`, 성공 여부 검사 |
| 기초 p.21 | `glfwSetWindowShouldClose`에 반환값 설명 | 반환형 `void`, 조회 함수와 구분 |
| 기초 p.25 | `const char* glfwGetError(...)` | 반환형 `int`, 설명 문자열은 인자로 받음 |
| 기초 p.26 | `glfwPollEvent()` | `glfwPollEvents()` |
| 기초 p.29, 37 | 본문 없는 `if (!glfwInit())` | 실패 처리 블록 완성 |
| 기초 p.41 | 문자 콜백 괄호·쉼표 오류 | `void CharCallback(GLFWwindow*, unsigned int)` |
| 기초 p.42 | 프레임버퍼 콜백 인자 `GLFWscrollfun` | `GLFWframebuffersizefun` |
| 기초 p.43 | `f (height != 0)` | `if (height != 0)` |
| 기초 p.48 | `glUniform2(...)` 등 축약 | 실제 사용 단계에서 타입 접미어와 원형 확인 |
| 실습 p.4 | `GLFW_KEY_ESC` | `GLFW_KEY_ESCAPE` |
| 여러 예제 | 함수 대소문자 혼용, 스마트 따옴표, `…` | 선언·정의·호출 이름 통일, 정상 문법으로 완성 |

GLFW 원형 보정은 [공식 초기화·오류 참조](https://www.glfw.org/docs/latest/group__init.html)와 [공식 창 참조](https://www.glfw.org/docs/latest/group__window.html)를 함께 확인했다. 교수님 학습 흐름을 따르되 명백한 오타는 그대로 보존하지 않는다.

## 다음 코드 작업 전 확인

- 실습 번호와 추가 강의자료.
- 도형 함수에 맞는 프로파일, 초기화 순서, 실패 처리, 콜백 등록.
- `InputProcess`·`DrawScene` 분리와 `//---` 한글 주석.
- 한 번 실행할 명령과 연속 동작, 좌표 변환과 창 크기 변경.
- 실습별 초기값·명령 키·개수 제한 및 원문에서 정하지 않은 동작.
- 실제 구현 요청 단계에서 빌드·실행 검증. 이 문서의 흐름 예시는 빌드 검증된 전체 프로그램이 아니다.
