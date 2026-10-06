//--- fragment shader: fragment.glsl 파일에 저장
#version 330 core // 버텍스 셰이더와 같은 GLSL 3.30 Core 문법을 사용한다.

//--- out_Color: 버텍스 세이더에서 전달받는 색상 값
//--- FragColor: 출력할 색상의 값으로 프레임 버퍼로 전달 됨.
in vec3 out_Color; //--- 버텍스 세이더에게서 전달 받음
out vec4 FragColor; //--- 색상 출력

/*
    vertex.glsl의 out vec3 out_Color
                       ↓ 정점 사이의 값을 보간
    fragment.glsl의 in vec3 out_Color

    한쪽에서는 out, 다음 쪽에서는 in이다. 이 코드에서는 이름과 타입으로 연결된다.
    out_Color라는 이름에 out이 들어 있어도 여기서는 in으로 선언했으므로 입력이다.
    FragColor는 우리가 붙인 출력 변수 이름이다. gl_Position처럼 고정된 내장 이름은 아니다.

    버텍스 셰이더는 정점을 처리한다.
    그 정점들로 삼각형/선을 만들고, 화면에서 덮는 부분을 프래그먼트로 만드는 단계를 거친다.
    프래그먼트 셰이더는 그 조각에 사용할 색상을 계산한다.
    입문할 때는 "그려질 픽셀의 색을 정하는 곳"으로 생각하면 된다.
    정확히는 프래그먼트와 최종 픽셀이 항상 일대일은 아니며,
    뒤의 테스트나 블렌딩 등에 따라 최종 화면에 저장될 색상이 결정된다.

    A: GL_TRIANGLES -> 내부를 채우는 프래그먼트들이 만들어진다.
    B: GL_LINE_LOOP -> 테두리 선을 따라 프래그먼트들이 만들어진다.
    이 셰이더는 두 경우 모두 동일하다. 그리기 방식은 C++의 glDrawArrays에서 결정한다.
*/

void main()
{
    FragColor = vec4(out_Color, 1.0);
    // out_Color가 (r,g,b)이므로 vec4로 만들면 (r,g,b,1.0)이 된다.
    // 여기서 마지막 1.0은 색상의 알파 값이다. vertex의 위치 좌표 w와 역할이 다르다.
    // 알파 1은 완전 불투명 값을 뜻한다. 알파를 줄여도 블렌딩 설정 없이는 자동으로 반투명해지지 않는다.
    // work_8에서는 블렌딩을 켜지 않고, 입력받은 RGB를 그대로 출력한다.
}
