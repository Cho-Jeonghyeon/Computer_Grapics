#version 330 core

//--- 정점에서 넘어온 색상에 알파 1(불투명)을 붙여 출력한다.
in vec3 out_Color;
out vec4 FragColor;

void main()
{
    FragColor = vec4(out_Color, 1.0);
}
