# honto Visual Studio Guide

## 1. 목적

이 문서는 `cocos2d-x`처럼 `라이브러리만 설치한 뒤 Visual Studio 프로젝트에서 게임 코드만 작성하는 방식`으로 `honto engine`을 사용하는 순서를 설명합니다.

## 2. 어떤 방식인가

이 방식에서는 엔진 소스를 프로젝트 안에 다시 넣지 않습니다.

대신:

1. `HonTo SDK` 설치
2. `honto_engine.lib` 링크
3. `honto/HonTo.h` 포함
4. 게임 코드는 프로젝트의 `src/main.cpp`에만 작성

이 흐름으로 사용합니다.

## 3. 먼저 SDK 설치

설치파일 방식이면:

1. `HonToEngine-SDK-Setup.exe` 실행
2. 설치 완료
3. Visual Studio 재시작

portable zip 방식이면:

1. zip 압축 해제
2. 예를 들어 `C:\HonToSDK`에 둠
3. 사용자 환경 변수 `HONTO_SDK_ROOT`를 `C:\HonToSDK`로 설정
4. Visual Studio 재시작

## 4. Visual Studio 프로젝트 생성

설치가 끝나면 PowerShell에서:

```powershell
& "$env:HONTO_SDK_ROOT\bin\New-HonToVsProject.ps1" -ProjectName MyVsGame -DestinationPath C:\Games
```

그러면 아래가 만들어집니다.

```text
C:\Games\MyVsGame
```

안에는:

- `MyVsGame.sln`
- `MyVsGame.vcxproj`
- `src\main.cpp`

이 들어 있습니다.

## 5. Visual Studio에서 여는 방법

1. `C:\Games\MyVsGame\MyVsGame.sln` 열기
2. `Release | x64` 선택
3. 빌드
4. 실행

## 6. 어떻게 라이브러리를 찾나

프로젝트는 SDK 안의 아래 props 파일을 불러옵니다.

```text
%HONTO_SDK_ROOT%\share\HonToEngine\vs\HonToEngine.props
```

이 props가 자동으로:

- include 경로
- lib 경로
- `honto_engine.lib`
- Windows 시스템 라이브러리

를 설정합니다.

즉 사용자는 `#include "honto/HonTo.h"`만 쓰고 게임 코드를 작성하면 됩니다.

## 7. 가장 작은 예제

```cpp
#include "honto/HonTo.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return honto::hontoGame("MyVsGame")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
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

## 8. portable zip과 setup zip의 차이

`setup zip`

- zip 안에 `setup.exe`가 있음
- 사용자는 압축 해제 후 exe 실행
- 환경 변수까지 자동 설정됨

`portable zip`

- 압축만 풀어서 바로 SDK 폴더를 얻음
- 환경 변수는 직접 설정해야 함
- 수동으로 원하는 위치에 둘 수 있음

## 9. 추천 방식

일반 사용자에게 배포할 때는 `setup zip`이 편합니다.

개발자나 팀 내부 공유용으로는 `portable zip`도 좋습니다.
