//--- vertex shader: vertex.glsl 파일에 저장
#version 330 core		//GLSL 버전 3.30 문법으로 컴파일해라.
//--- Position: attribute index 0
//--- Color: attribute index 1
layout (location = 0) in vec3 vPosition; //--- 위치 변수: attribute position 0 //CPU에서 attribute 0번으로 들어오는 데이터를 vPosition으로 받아라.
layout (location = 1) in vec3 vColor; //--- 컬러 변수: attribute position 1	//attribute 1번으로 들어오는 데이터를 vColor로 받아라.
out vec3 out_Color; //--- 프래그먼트 세이더에게 전달

void main(void)
{
	gl_Position = vec4 (vPosition, 1.0);
	out_Color = vColor;
}
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


*/
