# honto engine Guide

## 개요

`honto engine`은 Win32 기반 C++ 2D 엔진이지만, 아래처럼 다른 엔진들에서 중요하게 여기는 흐름을 직접 코드로 쓸 수 있게 구성되어 있습니다.

- `honto::hontoGame(...)`로 게임 시작
- `honto::hontoStage`에서 장면 구성
- 중력, 점프, 트윈 애니메이션, 화면 전환, 멀티 윈도우
- BMP 텍스처 로딩과 체커 텍스처 생성
- 카메라 따라가기
- 2D 맵을 이용한 둠 스타일 2.5D 레이캐스트 화면

이번 버전은 Unity, Godot, cocos2d-x, Unreal의 공통 핵심 축인 `장면`, `카메라`, `에셋`, `애니메이션`, `전환`, `윈도우`, `2D/2.5D 확장성`을 우선 구현한 상태입니다.

## 현재 가능한 것

### 1. 코드형 장면 구성

```cpp
return honto::hontoGame("My Game")
    .hontoWindow(1280, 720)
    .hontoRender(320, 180)
    .hontoPlay([](honto::hontoStage& stage)
    {
        stage.hontoBackground(18, 22, 34);
    })
    .hontoRun();
```

### 2. 액터 조작

```cpp
auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(98, 232, 132))
    .hontoAt(24.0f, 40.0f)
    .hontoLayer(3)
    .hontoUseGravity()
    .hontoGroundAt(136.0f)
    .hontoMoveLeftRight(120.0f)
    .hontoJumpWhenPressed(honto::hontoKey::Space, 250.0f);
```

### 3. 트윈 애니메이션

```cpp
crate.hontoAnimate()
    .hontoMoveTo(260.0f, 120.0f)
    .hontoScaleTo(1.2f)
    .hontoPaintTo(honto::hontoRGBA(255, 220, 140))
    .hontoIn(1.2f)
    .hontoPingPong()
    .hontoLoop()
    .hontoPlay();
```

### 4. 텍스처

`BMP` 파일을 직접 읽을 수 있고, 외부 파일 없이 체커 텍스처도 만들 수 있습니다.

```cpp
auto bmp = honto::hontoLoadTexture("assets/wall.bmp");
auto checker = honto::hontoCheckerTexture(
    32, 32,
    honto::hontoRGBA(40, 58, 98),
    honto::hontoRGBA(86, 132, 212),
    4
);

stage.hontoImage("panel", checker, 64.0f, 64.0f).hontoAt(40.0f, 30.0f);
```

### 5. 카메라

카메라는 수동 위치 지정과 액터 따라가기를 지원합니다.

```cpp
stage.hontoCameraAt(100.0f, 0.0f, 1.0f);
stage.hontoCameraFollow(player, 1.0f);
stage.hontoCameraReset();
```

### 6. 화면 전환

```cpp
stage.hontoGoWithFade(BuildNextScene, 0.7f);
```

### 7. 여러 창

```cpp
return honto::hontoGame("Main")
    .hontoOpenWindow("Tools", 720, 420, 240, 135, BuildToolsWindow)
    .hontoPlay(BuildMainScene)
    .hontoRun();
```

### 8. 둠 스타일 2.5D

`RaycastView`는 2D 그리드 맵을 기반으로 벽을 세워서 3D처럼 보이게 그립니다.

```cpp
auto raycast = stage.hontoRaycast("dungeon", 320.0f, 180.0f);
raycast.hontoMap({
        "##########",
        "#...A....#",
        "#.#.#.##A#",
        "#........#",
        "##########"
    })
    .hontoPlayer(1.5f, 1.5f, 0.0f)
    .hontoViewDegrees(68.0f)
    .hontoWall('#', honto::hontoRGBA(174, 112, 92))
    .hontoWallTexture('#', honto::hontoCheckerTexture(
        32, 32,
        honto::hontoRGBA(152, 92, 76),
        honto::hontoRGBA(118, 60, 46),
        8
    ))
    .hontoDoomControls();
```

조작은 기본적으로 아래 방식입니다.

- `W/S` 또는 `Up/Down`: 전진 / 후진
- `A/D`: 좌우 스트레이프
- `Left/Right`: 회전

## 주요 파일

- `engine/include/honto/Easy.h`
  - 사용자용 `honto...` DSL
- `engine/include/honto/Application.h`
  - 앱 루프, 전환, 멀티 윈도우
- `engine/include/honto/SceneGraph.h`
  - 노드, 스프라이트, 씬 그래프
- `engine/include/honto/Texture.h`
  - BMP 텍스처와 체커 텍스처
- `engine/include/honto/Raycast.h`
  - 둠 스타일 2.5D 뷰
- `engine/src/Renderer2D.cpp`
  - 소프트웨어 렌더러, 카메라, 알파 블렌딩, 텍스처 샘플링
- `sandbox/src/main.cpp`
  - 실제 예제 게임

## 내부 구조

### Application

- 메인 창과 추가 창을 모두 관리합니다.
- 각 창마다 별도 `Renderer2D`, 백버퍼, 활성 씬을 가집니다.
- 씬 교체 시 페이드 전환도 처리합니다.

### Scene Graph

- `Node` 기반 트리 구조입니다.
- `LayerColor`, `Sprite`, `RaycastView`가 모두 같은 씬 그래프에 올라갑니다.
- `z-order`를 기준으로 정렬됩니다.

### Easy API

- `Stage`가 장면 단위 편의 기능을 제공합니다.
- `Actor`는 노드 핸들 역할을 하며, 중력/점프/트윈/충돌 판정을 쉽게 쓸 수 있습니다.
- `RaycastActor`는 `RaycastView`를 쉽게 설정하기 위한 전용 핸들입니다.

## 현재 제한 사항

- 텍스처 로딩은 현재 `BMP` 중심입니다.
- 오디오, 타일맵 에디터, 정식 충돌 계층, 스프라이트 시트는 아직 최소 수준입니다.
- 2.5D는 둠식 레이캐스트이며, 진짜 폴리곤 3D 엔진은 아닙니다.

## 다음에 붙이면 좋은 것

1. PNG/WAV 로더
2. 스프라이트 시트 애니메이션
3. 타일맵 + 충돌 레이어
4. 오디오 믹서
5. 레벨 저장 포맷
6. 에디터 UI

## 주석 정책

코드 안에는 설명용 주석을 남기지 않고, 이 문서에서 구조와 사용법을 설명하는 방식으로 정리했습니다.
