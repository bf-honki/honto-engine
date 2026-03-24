# honto engine Guide

## 개요

`honto engine`은 Win32 기반 C++ 2D 엔진이지만, 아래처럼 다른 엔진들에서 중요하게 여기는 흐름을 직접 코드로 쓸 수 있게 구성되어 있습니다.

- `honto::hontoGame(...)`로 게임 시작
- `honto::hontoStage`에서 장면 구성
- 중력, 점프, 트윈 애니메이션, 스프라이트시트 애니메이션, 화면 전환, 멀티 윈도우
- BMP/PNG 텍스처 로딩, 체커 텍스처 생성, 프레임 시트 생성
- 타일맵 렌더링과 맵 충돌
- 텍스트 라벨과 진행 바 UI
- 레벨 파일 저장/불러오기
- WAV 재생, 시스템 사운드 alias 재생, 간단 톤 재생
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

### 2. 액터 조작과 타일 충돌

```cpp
auto world = stage.hontoTileMap(
    "world",
    {
        "....................",
        "......###...........",
        "..............####..",
        "####################"
    },
    16.0f,
    16.0f
);
world.hontoTile('#', honto::hontoRGBA(82, 126, 86), true, true);

auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(98, 232, 132))
    .hontoAt(24.0f, 40.0f)
    .hontoLayer(3)
    .hontoUseGravity()
    .hontoCollideWithMap(world)
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

### 4. 텍스처와 프레임 시트

`BMP` 파일을 직접 읽을 수 있고, 외부 파일 없이 체커 텍스처와 프레임 시트도 만들 수 있습니다.

```cpp
auto bmp = honto::hontoLoadTexture("assets/wall.bmp");
auto checker = honto::hontoCheckerTexture(
    32, 32,
    honto::hontoRGBA(40, 58, 98),
    honto::hontoRGBA(86, 132, 212),
    4
);
auto sheet = honto::hontoFrameSheetTexture(
    16, 16,
    {
        honto::hontoRGBA(92, 220, 128),
        honto::hontoRGBA(112, 240, 148),
        honto::hontoRGBA(84, 204, 122),
        honto::hontoRGBA(126, 255, 164)
    },
    4
);

stage.hontoImage("panel", checker, 64.0f, 64.0f).hontoAt(40.0f, 30.0f);
```

### 5. 스프라이트시트 애니메이션

```cpp
auto hero = stage.hontoImage("hero", sheet, 16.0f, 16.0f).hontoAt(24.0f, 40.0f);

hero.hontoAnimateFrames()
    .hontoTexture(sheet)
    .hontoFrameSize(16, 16)
    .hontoFrames({ 0, 1, 2, 3, 2, 1 })
    .hontoFPS(10.0f)
    .hontoLoop()
    .hontoPlay();
```

### 6. 카메라

카메라는 수동 위치 지정과 액터 따라가기를 지원합니다.

```cpp
stage.hontoCameraAt(100.0f, 0.0f, 1.0f);
stage.hontoCameraFollow(player, 1.0f);
stage.hontoCameraReset();
```

### 7. 텍스트와 UI

```cpp
auto title = stage.hontoText("title", "PORTAL PLAINS", honto::hontoRGBA(238, 245, 255), 2)
    .hontoAt(8.0f, 8.0f)
    .hontoLayer(10);

auto hp = stage.hontoBar(
    "hp",
    120.0f,
    10.0f,
    0.75f,
    honto::hontoRGBA(104, 228, 136),
    honto::hontoRGBA(12, 18, 30, 220),
    honto::hontoRGBA(236, 245, 255)
).hontoAt(8.0f, 32.0f);
```

### 8. 레벨 파일

```cpp
auto level = honto::hontoLoadLevel("sandbox/levels/platform.honto");
auto spawn = honto::hontoFindLevelEntity(level, "player_spawn");

auto world = stage.hontoTileMap("world", level);
if (spawn != nullptr)
{
    player.hontoAt(spawn->position);
}

honto::hontoSaveLevel("sandbox/levels/platform_exported.honto", level);
```

레벨 파일은 `sandbox/levels/platform.honto`처럼 텍스트 기반으로 저장됩니다.

### 9. 오디오

```cpp
stage.hontoPlayTone(740, 70);
stage.hontoPlaySound("assets/jump.wav");
honto::hontoPlayAlias("SystemAsterisk");
```

### 10. 화면 전환

```cpp
stage.hontoGoWithFade(BuildNextScene, 0.7f);
```

### 11. 여러 창

```cpp
return honto::hontoGame("Main")
    .hontoOpenWindow("Tools", 720, 420, 240, 135, BuildToolsWindow)
    .hontoPlay(BuildMainScene)
    .hontoRun();
```

### 12. 둠 스타일 2.5D

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
- `engine/include/honto/TileMap.h`
  - 타일맵 렌더링과 월드 충돌
- `engine/include/honto/Texture.h`
  - BMP/PNG 텍스처, 체커 텍스처, 프레임 시트
- `engine/include/honto/Audio.h`
  - WAV, alias, tone 재생
- `engine/include/honto/Level.h`
  - 레벨 저장/불러오기 포맷
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
- `Sprite`는 텍스처 전체뿐 아니라 특정 프레임 영역만 그릴 수 있습니다.

### TileMap

- `TileMap`은 문자 그리드 기반으로 맵을 정의합니다.
- 각 문자마다 색, 텍스처, 텍스처 영역, 고체 여부를 지정할 수 있습니다.
- 액터는 `hontoCollideWithMap(...)`으로 맵 충돌을 바로 붙일 수 있습니다.

### UI

- `Label`은 5x7 비트맵 글꼴 기반 텍스트를 그립니다.
- `ProgressBar`는 HUD용 진행 바를 그립니다.
- 둘 다 카메라 영향을 받을지 여부를 선택할 수 있습니다.

### Level

- `LevelDocument`는 타일 크기, 맵, 엔티티 목록을 담습니다.
- `LevelFile::Load/Save`로 텍스트 파일로 읽고 저장할 수 있습니다.
- 샌드박스는 실제로 `sandbox/levels/platform.honto`를 읽어서 장면을 구성합니다.

### Easy API

- `Stage`가 장면 단위 편의 기능을 제공합니다.
- `Actor`는 노드 핸들 역할을 하며, 중력/점프/트윈/타일 충돌/프레임 애니메이션/UI 갱신을 쉽게 쓸 수 있습니다.
- `TileMapActor`는 타일 정의와 충돌 맵 구성을 담당합니다.
- `RaycastActor`는 `RaycastView`를 쉽게 설정하기 위한 전용 핸들입니다.

## 현재 제한 사항

- 텍스처 로딩은 현재 `BMP`와 `PNG` 중심입니다.
- 오디오는 현재 Windows 기본 재생 경로 중심입니다.
- 타일 충돌은 축 정렬 박스 기준의 가벼운 플랫폼형 해결 방식입니다.
- 타일맵 에디터와 고급 충돌 계층은 아직 없습니다.
- 2.5D는 둠식 레이캐스트이며, 진짜 폴리곤 3D 엔진은 아닙니다.

## 다음에 붙이면 좋은 것

1. 마우스 입력과 버튼 위젯
2. JSON 또는 Tiled 맵 포맷 지원
3. 오디오 믹서
4. 파티클과 카메라 흔들림
5. 에디터용 프로젝트 포맷
6. 에디터 UI

## 주석 정책

코드 안에는 설명용 주석을 남기지 않고, 이 문서에서 구조와 사용법을 설명하는 방식으로 정리했습니다.
