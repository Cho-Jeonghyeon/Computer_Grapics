# 실습 27

근거: docs/실습/지엘4 조명 (실습 2627).pdf의 3쪽과 docs/지엘4 조명.pdf.

computer_grapics.slnx에서 work_27을 시작 프로젝트로 선택하고 **x64 Debug 또는 Release**로 실행합니다.

## 조작

| 키 | 동작 |
| --- | --- |
| c | 백색 → 붉은색 → 푸른색 조명 순환 |
| r / Shift+R | 중앙 구의 y축 기준 양 / 음 방향 공전 (같은 방향을 다시 누르면 정지) |
| q / Esc | 종료 |

## work_14에서 이어지는 구조

- filetobuf → MakeShader → InitShader: 기존 파일 읽기, 컴파일, 링크 코드를 유지.
- InitBuffer: 위치 VBO(0), 색상 VBO(1)를 유지하고 법선 VBO(2) 추가.
- KeyCallback / InputProcess → UpdateAnimation → DrawScene: 상태 변경, 시간 갱신, 그리기 분리 유지.
- DrawScene: model/view/projection uniform 전달과 glDrawArrays 사용.
- vertex.glsl: 기존 위치 변환에 월드 위치와 법선 변환 추가.
- fragment.glsl: 주변광, 산란반사, 거울반사의 Phong 조명 계산 추가.
- 조명 표시 도형은 조명 계산을 생략하고 자체 색으로 표시.
- 프로그램 시작 시 조명은 +Z 방향에 있고 공전은 정지 상태.

중앙의 초록 구, 왼쪽의 파랑 구, 오른쪽의 빨강 구를 같은 구 정점으로 그립니다. 구 생성은 work_18의 위도/경도 삼각형 방식을 사용하고, 매 정점의 법선은 단위 구의 위치 벡터입니다.

화면에서 두 번째인 초록 구의 중심이 조명 공전의 중심입니다. 세 번째 빨강 구는 x=2.2에 배치하고 조명 공전 반지름은 1.0으로 설정해, 조명이 두 구 사이의 빈 공간을 지나갑니다.

## 빌드 의존성

기존 work_14와 같은 GLEW/GLFW/GLM 환경을 사용합니다. 로컬 external/vcpkg_installed/x64-windows 설치가 있으면 새 프로젝트에서 자동으로 연결하고, 없으면 기존 SDK include/lib 설정을 사용합니다.
출력은 프로젝트별 x64/Debug 또는 x64/Release에 분리하고 GLSL 및 조명 예제에 필요한 DLL을 함께 복사합니다.
