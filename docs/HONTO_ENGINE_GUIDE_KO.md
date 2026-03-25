# honto engine Guide

## 개요

`honto engine`은 Win32 기반 C++ 2D 엔진이지만, 아래처럼 다른 엔진들에서 중요하게 여기는 흐름을 직접 코드로 쓸 수 있게 구성되어 있습니다.

- `honto::hontoGame(...)`로 게임 시작
- `honto::hontoStage`에서 장면 구성
- 중력, 점프, 트윈 애니메이션, 스프라이트시트 애니메이션, 화면 전환, 멀티 윈도우
- BMP/PNG 텍스처 로딩, 체커 텍스처 생성, 프레임 시트 생성
- 타일맵 렌더링과 맵 충돌
- 텍스트 라벨, 진행 바, 버튼 UI
- 마우스 좌표와 클릭 입력
- 레벨 파일 저장/불러오기, JSON 저장/불러오기, Tiled 스타일 JSON 로딩
- WAV 재생, 시스템 사운드 alias 재생, 간단 톤 재생, 오디오 버스 믹서
- 카메라 따라가기, 부드러운 카메라 추적, 카메라 흔들림
- 창 간 장면 이동, 창 투명도, 창 테두리 숨기기, 창 고정, 창 크기/위치 제어
- 파티클 이펙트, 트리거 존, 간단 순찰/추적 AI
- 클릭으로 타일을 칠하고 저장하는 레벨 에디터 창
- 2D 맵을 이용한 둠 스타일 2.5D 레이캐스트 화면, 문, 안개, 스프라이트, 무기 오버레이, 미니맵

이번 버전은 Unity, Godot, cocos2d-x, Unreal의 공통 핵심 축인 `장면`, `카메라`, `에셋`, `애니메이션`, `전환`, `윈도우`, `2D/2.5D 확장성`을 우선 구현한 상태입니다.

소스 구조를 코드 안 주석 대신 따로 읽고 싶다면 `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`를 보면 됩니다.

현재 샘플게임은 `honto Engine Academy`로 구성되어 있고, 허브에서 레벨을 하나씩 열어가며 각 엔진 기능을 실제 플레이로 익히게 되어 있습니다. 레벨별 상세 설명은 `docs/HONTO_ACADEMY_GUIDE_KO.md`에 정리되어 있습니다.

## 샘플 Academy 레벨

1. `LEVEL 1 STAGE AND ACTOR`
액터를 만들고, 이동시키고, 목표와 충돌하는 가장 기본적인 장면 구성을 배웁니다.

2. `LEVEL 2 PHYSICS AND TILEMAP`
중력, 점프, 타일맵, 맵 충돌을 이용해 플랫폼 동작을 만듭니다.

3. `LEVEL 3 ANIMATION AND PARTICLES`
프레임 애니메이션과 파티클 이펙트를 이용해 캐릭터와 오브젝트를 더 살아 있게 만듭니다.

4. `LEVEL 4 CAMERA, TRIGGER, AI`
카메라 따라가기, 트리거 존, 간단한 순찰/추적 AI를 레벨 목표와 함께 배웁니다.

5. `LEVEL 5 UI, BUTTON, AUDIO`
버튼, 진행 바, 마우스 입력, 오디오 버스를 이용한 상호작용 UI를 배웁니다.

6. `LEVEL 6 LEVEL, SAVE, LOAD`
런타임에서 타일을 바꾸고, 레벨 파일을 저장하고, 다시 불러오는 흐름을 익힙니다.

7. `LEVEL 7 2.5D RAYCAST`
2D 맵 데이터를 이용해 둠 스타일 2.5D 화면을 만드는 흐름을 체험합니다.

## 현재 가능한 것

### 0. 다른 컴퓨터 사용자가 엔진을 가져가서 자기 게임 만들기

이제 엔진은 설치형 SDK처럼 쓸 수 있습니다.

```powershell
.\scripts\Build-HonToSdk.ps1 -InstallRoot C:\HonToSDK
```

그 다음 새 게임 프로젝트를 바로 만들 수 있습니다.

```powershell
.\scripts\New-HonToProject.ps1 -ProjectName MyGame -DestinationPath C:\Games
```

생성된 프로젝트는 아래처럼 빌드합니다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
cmake --build build --config Release
```

Windows에서 Smart App Control이 켜져 있으면 아래처럼 인증서 지문을 같이 넘겨서 실행 파일 서명도 붙일 수 있습니다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK" -DHONTO_SIGN_CERT_SHA1="YOUR_CERT_SHA1"
cmake --build build --config Release
```

이 흐름으로 다른 컴퓨터의 다른 사용자도 SDK를 설치한 뒤 자기 게임을 만들 수 있습니다.

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
stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);
stage.hontoCameraShake(3.0f, 0.25f, 18.0f);
stage.hontoCameraReset();
```

### 6-1. 파티클

```cpp
auto fx = stage.hontoParticles("goal_fx", 24.0f, 32.0f);
fx.hontoAt(270.0f, 34.0f).hontoLayer(2);
fx.hontoEmissionRate(16.0f);
fx.hontoSpawnArea(24.0f, 32.0f);
fx.hontoVelocityRange({ -10.0f, -26.0f }, { 10.0f, -64.0f });
fx.hontoLifetimeRange(0.35f, 0.9f);
fx.hontoSizeRange(2.0f, 4.0f);
fx.hontoColorRange(honto::hontoRGBA(255, 236, 162, 220), honto::hontoRGBA(255, 132, 92, 0));
fx.hontoBurst(20);
```

### 6-2. 트리거와 간단 AI

```cpp
auto trigger = stage.hontoTrigger("goal_trigger", 26.0f, 34.0f).hontoAt(270.0f, 34.0f);

auto enemy = stage.hontoBox("enemy", 16.0f, 16.0f, honto::hontoRGBA(214, 86, 102))
    .hontoAt(220.0f, 80.0f)
    .hontoPatrolX(220.0f, 280.0f, 44.0f);

stage.hontoWhenTouching(player, trigger, []()
{
    honto::hontoPrint("entered");
});
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

버튼은 이렇게 바로 만들 수 있습니다.

```cpp
auto saveButton = stage.hontoButton("save", "SAVE JSON", 92.0f, 16.0f)
    .hontoAt(12.0f, 92.0f);

stage.hontoWhenClicked(saveButton, []()
{
    honto::hontoPrint("clicked");
});
```

마우스 좌표도 바로 읽을 수 있습니다.

```cpp
auto mouse = stage.hontoMousePosition();
if (stage.hontoHasMouse())
{
    honto::hontoPrint(std::to_string(static_cast<int>(mouse.x)));
}
```

### 8. 레벨 파일과 JSON

```cpp
auto level = honto::hontoLoadLevel("sandbox/levels/platform.honto");
auto spawn = honto::hontoFindLevelEntity(level, "player_spawn");

auto world = stage.hontoTileMap("world", level);
if (spawn != nullptr)
{
    player.hontoAt(spawn->position);
}

honto::hontoSaveLevel("sandbox/levels/platform_exported.honto", level);
honto::hontoSaveLevel("sandbox/levels/platform_exported.json", level);
```

레벨 로더는 아래 형식을 자동으로 인식합니다.

- `sandbox/levels/platform.honto` 같은 HonTo 텍스트 포맷
- `sandbox/levels/platform.json` 같은 HonTo JSON 포맷
- `sandbox/levels/platform_tiled.json` 같은 Tiled 스타일 JSON 포맷

Tiled 스타일 JSON은 현재 `tilelayer`와 `objectgroup` 중심의 실전형 일부 지원입니다.

### 9. 오디오와 믹서

```cpp
stage.hontoPlayTone(740, 70);
stage.hontoPlaySound("assets/jump.wav");
honto::hontoPlayAlias("SystemAsterisk");
stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
stage.hontoSetMasterVolume(0.85f);
stage.hontoSetBusVolume("music", 0.72f);
```

버스별 현재 볼륨도 읽을 수 있습니다.

```cpp
float master = stage.hontoMasterVolume();
float music = stage.hontoBusVolume("music");
float effect = stage.hontoBusVolume("effect");
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

### 11-1. 레벨 에디터 창

샌드박스에는 별도 `honto Level Editor` 창이 같이 뜨고, 여기서 마우스로 타일을 칠한 뒤 `.json`과 `.honto` 파일로 저장할 수 있습니다.

핵심은 아래 흐름입니다.

- `stage.hontoTileMap(...)`으로 편집 대상 맵 표시
- `stage.hontoMousePosition()`과 `world.hontoWorldToCell(...)`로 현재 셀 계산
- `world.hontoCell(column, row, tile)`로 실제 타일 변경
- `honto::hontoSaveLevel(...)`로 저장

### 12. 멀티버스 창 이동과 창 스타일 제어

다른 창 안의 장면을 바로 바꾸고, 그 창을 앞으로 가져오게 할 수 있습니다.

```cpp
stage.hontoGoWindowWithFade(
    "honto Multiverse Window",
    BuildMultiverseScene,
    0.7f,
    honto::hontoRGBA(22, 32, 58),
    true
);
```

현재 장면이 떠 있는 창 자체도 바꿀 수 있습니다.

```cpp
stage.hontoWindowBorderless(true)
    .hontoWindowOpacity(0.82f)
    .hontoWindowResizable(false)
    .hontoWindowTopMost(true)
    .hontoWindowSize(1120, 640)
    .hontoWindowCenter();
```

여기서 쓸 수 있는 대표 기능은 아래입니다.

- `stage.hontoWindowOpacity(...)`
- `stage.hontoWindowBorderless(...)`
- `stage.hontoWindowResizable(...)`
- `stage.hontoWindowTopMost(...)`
- `stage.hontoWindowSize(...)`
- `stage.hontoWindowPosition(...)`
- `stage.hontoWindowCenter()`
- `stage.hontoGoWindowWithFade(...)`
- `stage.hontoFocusWindow(...)`

### 13. 둠 스타일 2.5D

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
    .hontoDoor('D', honto::hontoRGBA(206, 168, 102), 0.65f, 2.0f)
    .hontoFog(honto::hontoRGBA(14, 18, 28), 0.42f)
    .hontoWeapon(honto::hontoCheckerTexture(
        96, 64,
        honto::hontoRGBA(64, 64, 72),
        honto::hontoRGBA(132, 132, 148),
        6
    ), 140.0f, 84.0f, honto::hontoRGBA(255, 255, 255))
    .hontoMiniMap(true, 7.0f)
    .hontoDoomControls();
```

현재 `RaycastView`에는 아래 기능도 들어 있습니다.

- 문 열기
- 적/오브젝트용 빌보드 스프라이트
- 거리 안개
- 달리기
- 무기 오버레이와 흔들림
- 미니맵

조작은 기본적으로 아래 방식입니다.

- `W/S` 또는 `Up/Down`: 전진 / 후진
- `A/D`: 좌우 스트레이프
- `Left/Right`: 회전
- `Shift`: 달리기
- `E` 또는 `Enter`: 문 사용
- `Tab` 또는 `M`: 미니맵 토글

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
  - WAV, alias, tone 재생과 오디오 버스 믹서
- `engine/include/honto/Level.h`
  - `.honto`, JSON, Tiled 스타일 JSON 레벨 로더
- `engine/include/honto/Raycast.h`
  - 둠 스타일 2.5D 뷰
- `engine/src/Renderer2D.cpp`
  - 소프트웨어 렌더러, 카메라, 알파 블렌딩, 텍스처 샘플링
- `sandbox/src/main.cpp`
  - 실제 예제 게임
- `examples/quickstart/src/main.cpp`
  - 설치형 SDK 기준의 외부 프로젝트 예제
- `templates/HonToStarter/src/main.cpp`
  - 다른 사용자가 자기 게임을 시작할 기본 템플릿
- `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`
  - 코드 안 주석 대신 따로 읽는 소스 구조 설명서

## 내부 구조

### Application

- 메인 창과 추가 창을 모두 관리합니다.
- 각 창마다 별도 `Renderer2D`, 백버퍼, 활성 씬을 가집니다.
- 씬 교체 시 페이드 전환도 처리합니다.
- 창 ID를 기준으로 다른 창의 장면 교체와 포커스 이동도 처리합니다.

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
- `Button`은 호버, 눌림, 가운데 정렬 텍스트를 가진 클릭 가능한 UI 노드입니다.
- 셋 다 카메라 영향을 받을지 여부를 선택할 수 있습니다.

### Input

- 키보드는 기존처럼 `hontoPressed`, `hontoPressing`으로 읽을 수 있습니다.
- 마우스는 현재 활성 윈도우의 렌더 좌표 기준으로 변환되어 들어옵니다.
- 멀티 윈도우 상황에서도 각 창이 자기 렌더 해상도 기준으로 마우스를 받습니다.

### Level

- `LevelDocument`는 타일 크기, 맵, 엔티티 목록을 담습니다.
- `LevelFile::Load/Save`로 텍스트 파일로 읽고 저장할 수 있습니다.
- 샌드박스는 실제로 `sandbox/levels/platform.honto`를 읽어서 장면을 구성합니다.

### Easy API

- `Stage`가 장면 단위 편의 기능을 제공합니다.
- `Actor`는 노드 핸들 역할을 하며, 중력/점프/트윈/타일 충돌/프레임 애니메이션/UI 갱신을 쉽게 쓸 수 있습니다.
- `TileMapActor`는 타일 정의와 충돌 맵 구성을 담당합니다.
- `RaycastActor`는 `RaycastView`를 쉽게 설정하기 위한 전용 핸들입니다.
- `Stage`는 창 투명도, 테두리, 리사이즈, 최상단, 포커스 이동 같은 런타임 창 제어도 제공합니다.

## 현재 제한 사항

- 텍스처 로딩은 현재 `BMP`와 `PNG` 중심입니다.
- 오디오는 현재 Windows 기본 재생 경로 중심입니다.
- 타일 충돌은 축 정렬 박스 기준의 가벼운 플랫폼형 해결 방식입니다.
- 샌드박스에는 기본 레벨 에디터 창이 있지만, 정식 인스펙터형 에디터는 아직 없습니다.
- 2.5D는 둠식 레이캐스트이며, 진짜 폴리곤 3D 엔진은 아닙니다.
- 하드웨어 가속 3D 렌더러나 셰이더 파이프라인은 아직 없습니다.

## 다음에 붙이면 좋은 것

1. 슬라이더, 체크박스, 텍스트 입력 같은 고급 UI 위젯
2. 포스트 효과와 더 풍부한 카메라 연출
3. 더 고급 적 AI와 경로 탐색
4. 에디터용 씬 인스펙터와 프로젝트 포맷
5. 에셋 파이프라인 강화와 패키징
6. Lua 또는 C# 스크립팅 계층

## 주석 정책

코드 안에는 설명용 주석을 남기지 않고, 이 문서에서 구조와 사용법을 설명하는 방식으로 정리했습니다.
