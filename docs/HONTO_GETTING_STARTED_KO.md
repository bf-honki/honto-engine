# honto Getting Started Guide

## 1. 목적

이 문서는 `cocos2d-x`처럼 `honto engine`을 내려받아 설치하고, 새 게임 프로젝트를 만들고, 직접 코드를 써서 빌드하고 실행하는 순서를 처음부터 끝까지 설명합니다.

이 문서 기준 운영체제는 `Windows`, 개발 도구는 `Visual Studio + CMake`입니다.

## 2. 준비물

필요한 것은 아래 3가지입니다.

1. `Git`
저장소를 내려받을 때 사용합니다.

2. `Visual Studio`
`Desktop development with C++` 워크로드가 포함되어 있어야 합니다.

3. `CMake`
보통 Visual Studio 설치 시 함께 들어오거나, 별도로 설치할 수 있습니다.

## 3. 저장소 받기

PowerShell에서 원하는 위치로 이동한 뒤 아래처럼 저장소를 내려받습니다.

```powershell
git clone https://github.com/bf-honki/honto-engine.git
cd honto-engine
```

압축 파일로 받았다면 적당한 폴더에 압축을 푼 뒤, 그 폴더를 작업 폴더로 사용하면 됩니다.

## 4. 샘플 먼저 실행해보기

엔진이 정상적으로 돌아가는지 가장 먼저 샘플을 돌려보는 것이 좋습니다.

```powershell
cmake -S . -B build
cmake --build build --config Release
```

실행:

```powershell
.\build\Release\honto_sandbox.exe
```

또는 Visual Studio Generator를 썼다면:

```powershell
.\build\vs2026-x64\Release\honto_sandbox.exe
```

실행되면 왼쪽 `honto Playground`와 오른쪽 `honto Code Lab`가 뜹니다.

## 5. SDK로 설치하기

다른 게임 프로젝트에서 엔진을 재사용하려면 SDK처럼 설치하는 편이 가장 편합니다.

저장소 루트에서:

```powershell
.\scripts\Build-HonToSdk.ps1 -InstallRoot C:\HonToSDK
```

설치가 끝나면 `C:\HonToSDK` 아래에 다음이 들어갑니다.

- 헤더 파일
- 엔진 라이브러리
- CMake package 파일
- 템플릿 프로젝트
- 문서
- 예제

## 6. 새 게임 프로젝트 만들기

이제 스타터 템플릿으로 새 게임을 생성합니다.

```powershell
.\scripts\New-HonToProject.ps1 -ProjectName MyGame -DestinationPath C:\Games
```

그러면 아래 폴더가 생깁니다.

```text
C:\Games\MyGame
```

## 7. 새 프로젝트 빌드하기

새 프로젝트 폴더로 이동합니다.

```powershell
cd C:\Games\MyGame
```

이제 `HonTo SDK`를 찾을 수 있게 `CMAKE_PREFIX_PATH`를 넘겨서 빌드합니다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
cmake --build build --config Release
```

실행 파일은 보통 아래 위치에 생깁니다.

```text
C:\Games\MyGame\build\Release\MyGame.exe
```

Visual Studio Generator일 때는:

```text
C:\Games\MyGame\build\Release\MyGame.exe
```

또는 Generator에 따라 `build\vs...` 아래에 생길 수 있습니다.

## 8. Visual Studio로 열기

PowerShell 대신 Visual Studio에서 작업하고 싶다면:

1. `C:\Games\MyGame` 또는 그 안의 `build`를 엽니다.
2. CMake 구성이 완료될 때까지 기다립니다.
3. 실행 대상을 `MyGame`으로 선택합니다.
4. `Release | x64`로 빌드하고 실행합니다.

## 9. 게임 코드는 어디에 쓰나

가장 중요한 파일은 보통 아래입니다.

```text
C:\Games\MyGame\src\main.cpp
```

이 파일 안에서 `honto::hontoGame(...)`으로 게임을 만들게 됩니다.

## 10. 가장 작은 예제

아래처럼 쓰면 창 하나가 열리고 배경색이 칠해집니다.

```cpp
#include "honto/HonTo.h"

int main()
{
    return honto::hontoGame("My Game")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoClear(honto::hontoRGBA(14, 18, 30))
        .hontoPlay([](honto::hontoStage& stage)
        {
            stage.hontoBackground(18, 22, 34);
            stage.hontoText("title", "HELLO HONTO", honto::hontoRGBA(240, 245, 255), 2)
                .hontoAt(10.0f, 10.0f)
                .hontoLayer(2);
        })
        .hontoRun();
}
```

## 11. 캐릭터를 움직이는 첫 게임 예제

아래 예제는 `타일맵`, `중력`, `좌우 이동`, `점프`를 같이 씁니다.

```cpp
#include "honto/HonTo.h"

int main()
{
    return honto::hontoGame("My Platformer")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoPlay([](honto::hontoStage& stage)
        {
            stage.hontoBackground(18, 22, 34);
            stage.hontoGravity(0.0f, 760.0f);

            auto world = stage.hontoTileMap(
                "world",
                {
                    "....................",
                    "..........##........",
                    "......##............",
                    "....................",
                    "####################"
                },
                16.0f,
                16.0f
            );
            world.hontoTile('#', honto::hontoRGBA(84, 128, 94), true, true);

            stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(96, 228, 136))
                .hontoAt(24.0f, 24.0f)
                .hontoLayer(3)
                .hontoUseGravity()
                .hontoCollideWithMap(world)
                .hontoMoveLeftRight(124.0f)
                .hontoJumpWhenPressed(honto::hontoKey::Space, 260.0f);
        })
        .hontoRun();
}
```

## 12. 자주 쓰는 흐름

새 게임을 만들 때 가장 자주 쓰는 순서는 보통 이렇습니다.

1. `honto::hontoGame("게임 이름")`
앱 시작

2. `.hontoWindow(...)`
실제 창 크기 설정

3. `.hontoRender(...)`
게임 내부 렌더 해상도 설정

4. `.hontoPlay(...)`
현재 씬 작성

5. `stage.hontoBackground(...)`
배경색 설정

6. `stage.hontoTileMap(...)`
맵 만들기

7. `stage.hontoBox(...)` 또는 `stage.hontoImage(...)`
플레이어/적/오브젝트 만들기

8. `.hontoUseGravity()`, `.hontoCollideWithMap(...)`, `.hontoMoveLeftRight(...)`
기본 플랫폼 액션 붙이기

9. `stage.hontoText(...)`, `stage.hontoButton(...)`
UI 붙이기

10. `stage.hontoPlayMusic(...)`, `stage.hontoPlayOnBus(...)`
사운드 붙이기

## 13. 에셋은 어디에 두나

보통 새 프로젝트 안에 이런 식으로 폴더를 두면 편합니다.

```text
MyGame
 ├─ assets
 │   ├─ audio
 │   ├─ images
 │   └─ levels
 └─ src
     └─ main.cpp
```

그 다음 코드에서 상대 경로로 불러옵니다.

```cpp
stage.hontoImage("hero", "assets/images/hero.png", 32.0f, 32.0f);
stage.hontoPlayMusic("assets/audio/theme.wav");
```

## 14. 다른 컴퓨터에서 쓰려면

방법은 두 가지입니다.

1. 저장소 자체를 내려받아 같은 과정을 다시 실행
2. 이미 설치된 `HonToSDK` 폴더를 옮기고 새 프로젝트에서 `CMAKE_PREFIX_PATH`로 연결

가장 안전한 방법은 각 컴퓨터에서 아래를 다시 한 번 실행하는 것입니다.

```powershell
.\scripts\Build-HonToSdk.ps1 -InstallRoot C:\HonToSDK
```

## 15. Smart App Control이 켜져 있을 때

대상 PC에서 실행 파일이 막히면, 공개 신뢰 코드서명 인증서나 개발용 인증서가 필요할 수 있습니다.

설치형 프로젝트에서는 아래처럼 인증서 지문을 넘겨 빌드할 수 있습니다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK" -DHONTO_SIGN_CERT_SHA1="YOUR_CERT_SHA1"
cmake --build build --config Release
```

## 16. 추천 시작 순서

처음 시작할 때는 아래 순서가 가장 편합니다.

1. 저장소 clone
2. 샘플 실행
3. SDK 설치
4. 새 프로젝트 생성
5. `main.cpp`에서 배경 + 텍스트부터 확인
6. 타일맵과 플레이어 추가
7. 애니메이션, 사운드, UI 추가

## 17. 함께 보면 좋은 문서

- `README.md`
- `docs/HONTO_ENGINE_GUIDE_KO.md`
- `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`
- `docs/HONTO_ACADEMY_GUIDE_KO.md`
