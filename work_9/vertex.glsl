#version 330 core

//--- location 0은 위치 VBO, 1은 색상 VBO와 연결된다.
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;
out vec3 out_Color;

void main()
{
    // 이동과 회전은 C++에서 계산했다. w=1이므로 위치를 그대로 전달한다.
    gl_Position = vec4(vPosition, 1.0);
    out_Color = vColor;
}
