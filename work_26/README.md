# 실습 26

근거: docs/실습/지엘4 조명 (실습 2627).pdf의 2쪽과 docs/지엘4 조명.pdf.

computer_grapics.slnx에서 work_26을 시작 프로젝트로 선택하고 **x64 Debug 또는 Release**로 실행합니다.

## 조작

| 키 | 동작 |
| --- | --- |
| n | 육면체 / 사각뿔 전환 |
| m | 조명 적용 / 원래 재질색 표시 |
| y | 물체의 y축 자전 시작 / 정지 |
| r / Shift+R | 조명의 y축 양 / 음 방향 공전 (같은 방향을 다시 누르면 정지) |
| z / Shift+Z | 누르는 동안 조명을 가깝게 / 멀게 이동 (반지름 1.1~2.8) |
| q / Esc | 종료 |

## work_14에서 이어지는 구조

- filetobuf → MakeShader → InitShader: 기존 파일 읽기, 컴파일, 링크 코드를 유지.
- InitBuffer: 위치 VBO(0), 색상 VBO(1)를 유지하고 법선 VBO(2) 추가.
- KeyCallback / InputProcess → UpdateAnimation → DrawScene: 상태 변경, 시간 갱신, 그리기 분리 유지.
- DrawScene: model/view/projection uniform 전달과 glDrawArrays 사용.
- vertex.glsl: 기존 위치 변환에 월드 위치와 법선 변환 추가.
- fragment.glsl: 주변광, 산란반사, 거울반사의 Phong 조명 계산 추가.
- 축/궤도/조명 표시 도형은 조명 계산을 생략하고 자체 색으로 표시.
- 프로그램 시작 시 조명은 +Z 방향에 있고 공전은 정지 상태.

공전 궤도와 조명 위치는 같은 lightRadius를 사용하므로 z/Z로 함께 변합니다. m으로 조명을 끄면 고정 파이프라인의 조명 비활성화처럼 물체의 원래 색을 표시하고 광원 표시는 회색으로 바뀝니다.

## 빌드 의존성

기존 work_14와 같은 GLEW/GLFW/GLM 환경을 사용합니다. 로컬 external/vcpkg_installed/x64-windows 설치가 있으면 새 프로젝트에서 자동으로 연결하고, 없으면 기존 SDK include/lib 설정을 사용합니다.
출력은 프로젝트별 x64/Debug 또는 x64/Release에 분리하고 GLSL 및 조명 예제에 필요한 DLL을 함께 복사합니다.
