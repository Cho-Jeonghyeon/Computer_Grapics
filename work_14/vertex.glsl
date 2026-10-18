#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 out_Color;

void main()
{
    //--- 모델링 → 뷰잉 → 투영 순서로 적용한다.
    gl_Position = projection * view * model * vec4(vPosition, 1.0);
    out_Color = vColor;
}
