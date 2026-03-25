# honto Code Lab Guide

## 개요

현재 샘플게임은 두 개의 창으로 이루어집니다.

- `honto Playground`
왼쪽 실행 창입니다. 기본 목표 크기는 `1620x1080`입니다.

- `honto Code Lab`
오른쪽 코드 선택 창입니다. 기본 목표 크기는 `1040x820`입니다.

`Code Lab`에서 코드를 마우스로 누르면 `Playground`가 그 코드에 맞는 데모로 즉시 바뀝니다. `stage.hontoOpenWindow(...)` 항목은 실제로 빈 `honto Runtime Window`를 띄웁니다.

## 공통 조작

- 마우스 클릭: `Code Lab` 코드 선택 또는 데모 버튼 실행
- `Esc`: 설정창 열기
- 설정창 `English`: 영어 UI
- 설정창 `한국어`: 한국어 UI
- 설정창 `게임 나가기`: 샘플 종료

## Code Lab 목록

### 1. `honto::hontoGame(...)`

기능:

- 게임 시작
- 메인 창 생성
- 코드 창 생성

예시:

```cpp
return honto::hontoGame("honto Playground")
    .hontoWindow(1320, 1080)
    .hontoPlay(BuildPlaygroundScene)
    .hontoOpenWindow(...)
    .hontoRun();
```

### 2. `stage.hontoGoWithFade(...)`

기능:

- 씬 교체
- 페이드 전환

예시:

```cpp
gSceneFlip = 1 - gSceneFlip;
stage.hontoGoWithFade(BuildPlaygroundScene, 0.25f);
```

### 3. `stage.hontoText(...)`

기능:

- 라벨
- 제목
- 상태 문구

예시:

```cpp
stage.hontoText("title", "HONTO LABEL", honto::hontoRGBA(238, 245, 255), 1)
    .hontoAt(24, 74)
    .hontoLayer(4);
```

### 4. `stage.hontoImage(...)`

기능:

- PNG 스프라이트
- 생성 텍스처
- 이미지 렌더링

예시:

```cpp
stage.hontoImage("badge", "sandbox/assets/honto_badge.png", 48, 48)
    .hontoAt(42, 82)
    .hontoLayer(4);
```

### 5. `stage.hontoBox(...)`

기능:

- 액터 생성
- 위치 지정
- 이동 입력

예시:

```cpp
auto player = stage.hontoBox("player", 16, 16, honto::hontoRGBA(114, 236, 148))
    .hontoAt(18, 110)
    .hontoMoveWithArrows(124);
```

### 6. `stage.hontoGravity(...)`

기능:

- 중력
- 점프
- 타일 충돌

예시:

```cpp
stage.hontoGravity(0, 760);
auto world = stage.hontoTileMap("world", map, 16, 16);
player.hontoUseGravity()
    .hontoCollideWithMap(world);
```

### 7. `actor.hontoAnimateFrames()`

기능:

- 프레임 애니메이션
- 스프라이트시트 재생

예시:

```cpp
player.hontoAnimateFrames()
    .hontoFrameSize(16, 16)
    .hontoFrames({0, 1, 2, 3})
    .hontoLoop()
    .hontoPlay();
```

### 8. `stage.hontoParticles(...)`

기능:

- 파티클 이미터
- 버스트 발생

예시:

```cpp
auto burst = stage.hontoParticles("burst", 18, 18);
burst.hontoVelocityRange({-26, -24}, {26, -86});
burst.hontoBurst(18);
```

### 9. `stage.hontoCamera(...)`

기능:

- 카메라 따라가기
- 트리거
- 순찰/추적 AI

예시:

```cpp
stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);
auto enemy = stage.hontoBox("enemy", 16, 16, color)
    .hontoPatrolX(172, 244, 44);
```

### 10. `stage.hontoButton(...)`

기능:

- 버튼 UI
- 진행 바
- 마우스 클릭 반응

예시:

```cpp
auto play = stage.hontoButton("music", "PLAY", 84, 16);
auto bar = stage.hontoBar("level", 96, 8, 0.0f, color, bg, border);
```

### 11. `stage.hontoPlayMusic(...)`

기능:

- 배경음
- 효과음
- 톤 재생
- 오디오 버스

예시:

```cpp
stage.hontoSetBusVolume("music", 0.72f);
stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
```

### 12. `honto::hontoSaveLevel(...)`

기능:

- 런타임 타일 수정
- 저장
- 불러오기

예시:

```cpp
editor.hontoCell(1, 0, '#');
world.hontoCell(9, 4, '#');
honto::hontoSaveLevel("sandbox/levels/bridge_demo.json", level);
```

### 13. `stage.hontoRaycast(...)`

기능:

- 둠식 2.5D
- 문
- 무기 오버레이

예시:

```cpp
auto doom = stage.hontoRaycast("view", 320, 112);
doom.hontoMap(kRaycastMap)
    .hontoPlayer(1.5f, 1.5f, 0.0f)
    .hontoDoor('D', color, 0.65f, 2.0f)
    .hontoDoomControls();
```

### 14. `stage.hontoOpenWindow(...)`

기능:

- 런타임 창 생성
- 코드 클릭으로 새 창 띄우기

예시:

```cpp
stage.hontoOpenWindow(
    "honto Runtime Window",
    720, 420, 320, 180,
    BuildRuntimeWindowScene,
    honto::hontoRGBA(12, 16, 24),
    false, true);
```

## 확인 포인트

- 코드 목록이 `Game`, `Scene`, `Label`, `Sprite`까지 포함해서 넓어졌는지
- `Code Lab`이 오른쪽, `Playground`가 왼쪽에 배치되는지
- 클릭한 코드가 즉시 데모로 실행되는지
- `stage.hontoOpenWindow(...)` 클릭 시 빈 창이 뜨는지
- `Esc` 설정창에서 영어/한국어 전환과 종료가 되는지
