# honto Installer Guide

## 1. 목적

이 문서는 `honto engine`을 `설치 프로그램` 또는 `릴리즈 zip`으로 배포하고, 설치한 뒤 `Visual Studio`에서 바로 새 게임 프로젝트를 여는 흐름을 설명합니다.

## 2. 설치파일 만드는 사람 기준

저장소 루트에서 아래 명령을 실행하면 `SDK 설치파일`이 만들어집니다.

```powershell
.\scripts\Build-HonToInstaller.ps1
```

기본 출력 파일은 아래입니다.

```text
dist\HonToEngine-SDK-Setup.exe
```

설치형 zip과 portable zip까지 같이 만들고 싶다면 아래 명령을 사용합니다.

```powershell
.\scripts\Build-HonToReleaseAssets.ps1
```

그러면 아래 파일들이 생깁니다.

```text
dist\HonToEngine-SDK-Portable-0.1.0-win64.zip
dist\HonToEngine-SDK-Setup-0.1.0-win64.zip
dist\HonToEngine-SDK-Setup.exe
```

이 설치파일은 내부적으로:

1. 엔진을 `Release`로 빌드
2. SDK zip 패키지 생성
3. 설치용 실행 파일 생성

순서로 동작합니다.

## 3. 설치파일을 받는 사용자 기준

사용자는 아래처럼 진행하면 됩니다.

1. `HonToEngine-SDK-Setup.exe` 실행
2. 설치 완료 대기
3. Visual Studio 완전히 종료 후 다시 실행

설치가 끝나면 기본 설치 위치는 아래입니다.

```text
%LOCALAPPDATA%\HonToSDK
```

동시에 사용자 환경 변수 `HONTO_SDK_ROOT`도 설정됩니다.

`HonToEngine-SDK-Setup-0.1.0-win64.zip`로 배포하면 사용자는:

1. zip 다운로드
2. 압축 해제
3. 안의 `HonToEngine-SDK-Setup.exe` 실행

순서로 설치하면 됩니다.

## 4. 설치 후 Visual Studio에서 어떻게 연결되나

새 HonTo 프로젝트의 `CMakeLists.txt`는 `HONTO_SDK_ROOT` 환경 변수를 먼저 확인합니다.

즉 설치가 끝난 뒤에는 프로젝트 안에서 별도로 `CMAKE_PREFIX_PATH`를 수동 입력하지 않아도, 보통은 아래 줄이 자동으로 SDK를 찾게 만듭니다.

```cmake
find_package(HonToEngine CONFIG REQUIRED)
```

중요한 점은 `Visual Studio를 한 번 껐다가 다시 켜야` 새 환경 변수를 읽는다는 점입니다.

## 5. 새 게임 만들기

설치 후 PowerShell에서:

```powershell
& "$env:HONTO_SDK_ROOT\bin\New-HonToProject.ps1" -ProjectName MyGame -DestinationPath C:\Games
```

그러면 아래 폴더가 만들어집니다.

```text
C:\Games\MyGame
```

Visual Studio 라이브러리 프로젝트 형식으로 만들고 싶다면:

```powershell
& "$env:HONTO_SDK_ROOT\bin\New-HonToVsProject.ps1" -ProjectName MyVsGame -DestinationPath C:\Games
```

## 6. Visual Studio에서 열기

1. `Visual Studio` 실행
2. `Open a local folder`
3. `C:\Games\MyGame` 선택
4. CMake configure 완료 대기
5. 실행 대상 `MyGame` 선택
6. `Release | x64` 또는 `Debug | x64` 빌드 후 실행

## 7. 직접 CMake로 빌드하는 방법

환경 변수가 이미 잡혀 있으면 아래처럼 간단히 빌드할 수 있습니다.

```powershell
cd C:\Games\MyGame
cmake -S . -B build
cmake --build build --config Release
```

## 8. 설치 후 파일 위치

기본적으로 아래가 들어갑니다.

- `include`
- `lib`
- `bin`
- `share\HonToEngine`

특히 자주 쓰는 것은 아래입니다.

- `%LOCALAPPDATA%\HonToSDK\bin\New-HonToProject.ps1`
- `%LOCALAPPDATA%\HonToSDK\share\HonToEngine\templates\HonToStarter`

## 9. 게임 코드 작성 위치

새 프로젝트에서 가장 자주 수정하는 파일은 아래입니다.

```text
C:\Games\MyGame\src\main.cpp
```

## 10. 가장 짧은 예제

```cpp
#include "honto/HonTo.h"

int main()
{
    return honto::hontoGame("My Game")
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
