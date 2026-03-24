# honto engine Source Guide

## 이 문서의 목적

이 문서는 코드 안 주석 대신, 엔진 소스 구조와 사용 방법을 한 곳에서 설명하기 위한 파일입니다.

목표는 두 가지입니다.

- 엔진을 수정하려는 개발자가 내부 구조를 빠르게 이해하는 것
- 다른 컴퓨터의 다른 사용자가 엔진을 설치하고 자기 게임을 바로 시작하는 것

## 전체 구조

### 1. 런타임 코어

- `engine/include/honto/Application.h`
- `engine/src/Application.cpp`
- `engine/include/honto/Window.h`
- `engine/src/WindowWin32.cpp`

이 계층은 창 생성, 메인 루프, 장면 교체, 멀티 윈도우, 페이드 전환, 창 포커스 이동, 창 스타일 제어를 담당합니다.

### 2. 씬 그래프와 렌더링

- `engine/include/honto/SceneGraph.h`
- `engine/src/SceneGraph.cpp`
- `engine/include/honto/Renderer2D.h`
- `engine/src/Renderer2D.cpp`

이 계층은 노드 트리, 색 사각형, 스프라이트, 텍스트, 진행 바, 버튼, 카메라, 알파 블렌딩을 처리합니다.

### 3. 편의 DSL

- `engine/include/honto/Easy.h`

사용자는 보통 여기의 `honto::hontoGame(...)`, `honto::hontoStage`, `actor.honto...` 문법으로 게임을 만듭니다.

### 4. 월드와 에셋

- `engine/include/honto/TileMap.h`
- `engine/src/TileMap.cpp`
- `engine/include/honto/Texture.h`
- `engine/src/Texture.cpp`
- `engine/include/honto/Level.h`
- `engine/src/Level.cpp`
- `engine/include/honto/Audio.h`
- `engine/src/Audio.cpp`

이 계층은 타일맵, 충돌, BMP/PNG 텍스처, 레벨 포맷, JSON/Tiled 로딩, WAV와 오디오 버스를 담당합니다.

### 5. 2.5D 확장

- `engine/include/honto/Raycast.h`
- `engine/src/Raycast.cpp`

이 계층은 둠 스타일 레이캐스트 뷰, 문, 안개, 빌보드 스프라이트, 무기 오버레이, 미니맵을 담당합니다.

## 외부 사용자가 쓰는 방식

### 방법 1. SDK를 설치해서 사용

엔진 저장소에서 아래처럼 SDK를 설치할 수 있습니다.

```powershell
.\scripts\Build-HonToSdk.ps1 -InstallRoot C:\HonToSDK
```

이렇게 설치한 뒤에는 다른 컴퓨터로 `C:\HonToSDK` 폴더를 복사하거나, 같은 과정을 그 컴퓨터에서 다시 실행하면 됩니다.

### 방법 2. 새 게임 프로젝트 생성

설치된 SDK나 저장소 루트에서 아래 스크립트를 실행합니다.

```powershell
.\scripts\New-HonToProject.ps1 -ProjectName MyGame -DestinationPath C:\Games
```

그러면 `C:\Games\MyGame` 안에 새 프로젝트가 생성됩니다.

### 방법 3. 생성된 프로젝트 빌드

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
cmake --build build --config Release
```

여기서 `CMAKE_PREFIX_PATH`는 설치된 `HonToEngineConfig.cmake`를 찾기 위한 경로입니다.

Smart App Control이 켜진 PC라면 아래처럼 인증서 지문을 함께 넘겨서 빌드 후 서명을 자동으로 붙일 수 있습니다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK" -DHONTO_SIGN_CERT_SHA1="YOUR_CERT_SHA1"
cmake --build build --config Release
```

## 코드 흐름 설명

### 시작점

사용자 게임은 보통 아래 순서로 시작합니다.

1. `honto::hontoGame("Game Name")`
2. `hontoWindow(...)`와 `hontoRender(...)`로 창과 내부 렌더 해상도 지정
3. `hontoPlay(...)`에 장면 빌더 람다 전달
4. `hontoRun()`으로 실행

### 장면 구성

`honto::hontoStage`는 장면 안에서 액터를 만들고 입력과 전환을 연결하는 중심 객체입니다.

대표 메서드는 아래와 같습니다.

- `stage.hontoBox(...)`
- `stage.hontoImage(...)`
- `stage.hontoTileMap(...)`
- `stage.hontoText(...)`
- `stage.hontoButton(...)`
- `stage.hontoEveryFrame(...)`
- `stage.hontoWhenPressed(...)`
- `stage.hontoGoWithFade(...)`
- `stage.hontoGoWindowWithFade(...)`

### 액터 조작

액터는 주로 `Actor` 핸들로 다룹니다.

- `hontoAt(...)`
- `hontoMoveLeftRight(...)`
- `hontoUseGravity()`
- `hontoCollideWithMap(...)`
- `hontoJumpWhenPressed(...)`
- `hontoAnimate()`
- `hontoAnimateFrames()`

### 둠 스타일 2.5D

레이캐스트 뷰는 `stage.hontoRaycast(...)`로 만들고, 아래 기능을 조합합니다.

- `hontoMap(...)`
- `hontoWall(...)`
- `hontoWallTexture(...)`
- `hontoDoor(...)`
- `hontoThingTexture(...)`
- `hontoWeapon(...)`
- `hontoWeaponBob(...)`
- `hontoFog(...)`
- `hontoMiniMap(...)`
- `hontoDoomControls()`

## 예시 1. 가장 작은 게임

```cpp
#include "honto/HonTo.h"

int main()
{
    return honto::hontoGame("Tiny Game")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoPlay([](honto::hontoStage& stage)
        {
            stage.hontoBackground(18, 22, 34);
            stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(98, 232, 132))
                .hontoAt(24.0f, 24.0f)
                .hontoMoveWithArrows(120.0f);
        })
        .hontoRun();
}
```

## 예시 2. 설치형 외부 프로젝트 템플릿

- `templates/HonToStarter/CMakeLists.txt`
- `templates/HonToStarter/src/main.cpp`

이 템플릿은 다른 사용자가 자기 게임을 만드는 출발점입니다.

## 예시 3. 분리된 예제 프로젝트

- `examples/quickstart/CMakeLists.txt`
- `examples/quickstart/src/main.cpp`

이 예제는 엔진 저장소 밖에서도 `find_package(HonToEngine CONFIG REQUIRED)`로 사용하는 방식을 보여줍니다.

## 지금 상태에서 가능한 것

- 여러 창을 띄우고 각 창마다 다른 씬 실행
- 창 간 장면 이동
- 창 투명도, 테두리 숨김, 고정, 크기 조절, 이동
- 2D 플랫폼 게임
- 타일맵 충돌
- 프레임 애니메이션
- 버튼과 HUD UI
- JSON/Tiled 스타일 레벨 로딩
- 오디오 버스 믹서
- 둠 스타일 2.5D

## 아직 남아 있는 큰 축

- 파티클과 화면 효과
- 더 풍부한 UI 위젯
- 에디터 인스펙터
- 적 AI와 경로 탐색
- 스크립팅 계층

## 추천 시작 순서

1. SDK 설치
2. 새 프로젝트 생성
3. `templates/HonToStarter/src/main.cpp`를 자기 게임 시작점으로 수정
4. 필요하면 `examples/quickstart/src/main.cpp`를 참고해서 구조 확장
5. 더 고급 기능이 필요하면 `engine/include/honto/Easy.h`에서 DSL 추가
