#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 out_Color;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    //--- work_14의 모델링 -> 뷰잉 -> 투영 변환 연결을 유지한다.
    gl_Position = projection * view * model * vec4(vPosition, 1.0);
    out_Color = vColor;
    FragPos = vec3(model * vec4(vPosition, 1.0));
    //--- 회전/크기 변환 후에도 표면에 수직이 되도록 법선도 월드 공간으로 변환한다.
    Normal = mat3(transpose(inverse(model))) * vNormal;
}
