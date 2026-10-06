//--- vertex shader: vertex.glsl 파일에 저장
#version 330 core        //GLSL 버전 3.30 문법으로 컴파일해라.

//--- Position: attribute index 0
//--- Color: attribute index 1
layout (location = 0) in vec3 vPosition; //--- 위치 변수: attribute position 0 //CPU에서 attribute 0번으로 들어오는 데이터를 vPosition으로 받아라.
layout (location = 1) in vec3 vColor; //--- 컬러 변수: attribute position 1 //attribute 1번으로 들어오는 데이터를 vColor로 받아라.
out vec3 out_Color; //--- 프래그먼트 세이더에게 전달

/*
    CPU

    Attribute 0 ─────────────→ vPosition
    Attribute 1 ─────────────→ vColor

                              Vertex Shader

    C++의 main()은 프로그램 실행 시 한 번 실행되지만,
    Vertex Shader의 main()은:
    정점마다 한 번씩 실행된다.

    Vertex Shader는 최종적으로:
    “이 정점이 화면 어디에 위치해야 하는지”
    gl_Position에 넣어줘야 한다.

    work_8 코드에 연결해서 보면:
    positions 배열 -> vbo[0] -> location 0 -> vPosition
    colors 배열    -> vbo[1] -> location 1 -> vColor

    location 숫자는 C++의 glVertexAttribPointer(i, ...)에 넣은 i와 맞춰야 한다.
    C++ 변수 이름 positions와 GLSL 변수 이름 vPosition은 같을 필요가 없다.
    두 코드는 서로 다른 언어로 실행되며, 여기서는 location 번호로 데이터를 연결한다.

    in: 이 셰이더가 받는 입력. out: 다음 단계로 전달할 출력.
    vec3: float 성분 3개를 묶는 GLSL 자료형.
    위치라면 (x,y,z), 색상이라면 (r,g,b)라는 의미로 사용한다.
    vPosition에는 배열 전체가 아니라 지금 처리하는 정점 하나의 위치가 들어온다.
*/

void main()
{
    gl_Position = vec4(vPosition, 1.0);
    // vec4(vPosition, 1.0)은 (x, y, z, 1.0) 네 성분을 만든다.
    // gl_Position은 OpenGL이 정점 위치 결과를 읽는 내장 출력 변수다.
    // 정확히는 클립 좌표이며 마지막 값 w는 동차 좌표 성분이다. 색상의 알파가 아니다.
    // 이후 좌표를 w로 나누는데 여기서는 w=1이라 x/y/z 값이 그대로 유지된다.
    // C++에서 이미 -1~1 기준으로 위치를 계산했으므로 여기서는 별도 행렬 변환을 하지 않는다.

    out_Color = vColor; // CPU에서 받은 이 정점의 색상을 그대로 다음 단계에 전달한다.
    // 삼각형 내부를 채울 때는 정점들의 출력 색상을 이용해 각 프래그먼트의 색상이 보간된다.
    // 세 정점에 같은 색을 넣었으므로 work_8의 삼각형은 전체가 같은 색으로 보인다.
    // 세 정점 색을 다르게 보내면 그 사이에 색상 변화가 생길 수 있다.
}
