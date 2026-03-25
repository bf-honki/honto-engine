# honto Engine Academy Guide

## 개요

`honto Engine Academy`는 샘플게임 자체를 튜토리얼처럼 구성한 예제입니다.

허브에서 레벨 게이트를 왼쪽에서 오른쪽 순서로 열어가며, 다른 게임 엔진들에서도 거의 항상 다루는 핵심 코드 흐름을 직접 플레이하면서 배우게 됩니다.

메인 샘플 구현은 `sandbox/src/main.cpp`에 있고, 코드 내부 주석 대신 이 문서와 `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`에서 구조를 설명합니다.

## 진행 방식

- 허브에서 `A/D` 또는 방향키로 이동
- `Space`로 점프
- 게이트에 닿은 상태에서 `Enter`로 레벨 진입
- 각 레벨에서 목표를 달성하면 허브로 돌아오며 다음 레벨이 열림
- 대부분의 레벨은 `Esc`로 허브 복귀 가능

## 레벨 1

### STAGE AND ACTOR

배우는 것:

- `stage.hontoBox(...)`
- `actor.hontoAt(...)`
- `actor.hontoMoveWithArrows(...)`
- `actor.hontoTouching(...)`

대표 코드:

```cpp
auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(114, 236, 148))
    .hontoAt(16.0f, 118.0f)
    .hontoMoveWithArrows(124.0f)
    .hontoKeepInside(10.0f, 74.0f, 294.0f, 132.0f);
```

플레이 목표:

- 세 개의 목표 오브젝트를 주워서 출구 포털 열기

## 레벨 2

### PHYSICS AND TILEMAP

배우는 것:

- `stage.hontoGravity(...)`
- `stage.hontoTileMap(...)`
- `player.hontoUseGravity()`
- `player.hontoCollideWithMap(world)`
- `player.hontoJumpWhenPressed(...)`

대표 코드:

```cpp
stage.hontoGravity(0.0f, 760.0f);

auto world = stage.hontoTileMap("world", map, 16.0f, 16.0f);
world.hontoTile('#', honto::hontoRGBA(142, 110, 82), true, true);

player.hontoUseGravity()
    .hontoCollideWithMap(world)
    .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);
```

플레이 목표:

- 발판을 점프로 올라가서 출구 비콘에 닿기

## 레벨 3

### ANIMATION AND PARTICLES

배우는 것:

- `honto::hontoFrameSheetTexture(...)`
- `actor.hontoAnimateFrames()`
- `stage.hontoParticles(...)`
- `fx.hontoBurst(...)`

대표 코드:

```cpp
player.hontoAnimateFrames()
    .hontoTexture(sheet)
    .hontoFrameSize(16, 16)
    .hontoFrames({ 0, 1, 2, 3, 2, 1 })
    .hontoFPS(10.0f)
    .hontoLoop()
    .hontoPlay();

burst.hontoBurst(18);
```

플레이 목표:

- 애니메이션이 적용된 캐릭터로 세 개의 크리스털을 활성화하고 FX 포털 열기

## 레벨 4

### CAMERA, TRIGGER, AI

배우는 것:

- `stage.hontoCameraFollowSmooth(...)`
- `stage.hontoTrigger(...)`
- `enemy.hontoPatrolX(...)`
- `enemy.hontoChaseX(...)`
- `stage.hontoCameraShake(...)`

대표 코드:

```cpp
stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);

auto hunter = stage.hontoBox("hunter", 16.0f, 16.0f, honto::hontoRGBA(214, 86, 102))
    .hontoUseGravity()
    .hontoCollideWithMap(world)
    .hontoChaseX(player, 56.0f, 18.0f);
```

플레이 목표:

- 카메라가 따라오는 긴 맵에서 적을 피하고 스위치 트리거에 닿기

## 레벨 5

### UI, BUTTON, AUDIO

배우는 것:

- `stage.hontoButton(...)`
- `stage.hontoWhenClicked(...)`
- `stage.hontoBar(...)`
- `stage.hontoPlayMusic(...)`
- `stage.hontoPlayOnBus(...)`
- `stage.hontoMousePosition()`

대표 코드:

```cpp
auto musicButton = stage.hontoButton("music", "PLAY MUSIC", 84.0f, 16.0f);

stage.hontoWhenClicked(musicButton, [stage]()
{
    stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
});
```

플레이 목표:

- 세 개의 콘솔 시스템을 클릭으로 켜고, 모든 게이지를 채운 뒤 허브 버튼 해금하기

## 레벨 6

### LEVEL, SAVE, LOAD

배우는 것:

- `honto::hontoLevel`
- `world.hontoCell(...)`
- `honto::hontoSaveLevel(...)`
- `honto::hontoLoadLevel(...)`

대표 코드:

```cpp
editor.hontoCell(column, 0, '#');
world.hontoCell(8 + column, 4, '#');
level.map[4][8 + column] = '#';

honto::hontoSaveLevel("sandbox/levels/academy_bridge.json", level);
```

플레이 목표:

- 에디터 셀 세 칸을 눌러 다리를 만들고, 필요하면 저장/불러오기를 써서 출구까지 건너가기

## 레벨 7

### 2.5D RAYCAST

배우는 것:

- `stage.hontoRaycast(...)`
- `doom.hontoMap(...)`
- `doom.hontoDoor(...)`
- `doom.hontoDoomControls()`
- `doom.hontoPlayerPosition()`

대표 코드:

```cpp
auto doom = stage.hontoRaycast("lesson7_doom", 320.0f, 112.0f);

doom.hontoMap(map)
    .hontoPlayer(1.5f, 1.5f, 0.0f)
    .hontoDoor('D', honto::hontoRGBA(206, 168, 102), 0.65f, 2.0f)
    .hontoDoomControls();
```

플레이 목표:

- `E`로 문을 열고, 2.5D 미로 끝의 출구 지점까지 이동하기

## 어떤 순서로 읽으면 좋은가

1. `LEVEL 1`과 `LEVEL 2`로 액터와 물리 기본기 익히기
2. `LEVEL 3`과 `LEVEL 4`로 게임다운 표현과 레벨 로직 익히기
3. `LEVEL 5`와 `LEVEL 6`으로 툴, UI, 데이터 흐름 익히기
4. `LEVEL 7`로 2D 기반 2.5D 확장 보기

## 관련 문서

- `docs/HONTO_ENGINE_GUIDE_KO.md`
- `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`
