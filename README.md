# HonTo Engine

`HonTo Engine` is a C++ 2D game-engine starter designed to grow toward a Unity, Godot, cocos2d-x, or Unreal-style workflow for code-first 2D projects.

Detailed Korean guide: `docs/HONTO_ENGINE_GUIDE_KO.md`
Source structure guide: `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`

This version now covers the core runtime features most people expect when prototyping a 2D engine:

- a native Windows window built with Win32
- a software 2D renderer with a pixel backbuffer
- scenes, entities, transforms, sprites, and simple rigid bodies
- a cocos2d-x style code-first scene graph with nodes, layers, and sprites
- a lambda-style "easy" API for gravity, tilemaps, collision, sprite-sheet animation, text/UI/buttons, mouse input, audio buses, level files, scene transitions, multiverse-style window travel, and runtime window styling
- keyboard and mouse input
- level loading from `.honto`, HonTo JSON, and Tiled-style JSON
- a sandbox game that shows 2D platforming, tile collisions, PNG loading, HUD text/UI, button clicks, JSON/Tiled loading, audio mixer controls, multiverse window travel, and a DOOM-style 2.5D scene with doors, fog, sprites, a weapon overlay, and a minimap
- installable CMake package support so other developers can use the engine on a different machine with `find_package(HonToEngine CONFIG REQUIRED)`
- a reusable starter template and standalone quickstart example for external game projects

## Why this structure

If the goal is "other people can make games with it", the engine needs a stable runtime API first, then editor tooling, richer asset import, richer animation, physics, polish tools, and scripting.

This repository starts with the runtime layer so we can iterate safely.

## Build

From PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Or use a preset:

```powershell
cmake --preset vs2026-x64
cmake --build --preset build-vs2026-release
```

Run:

```powershell
.\build\Release\honto_sandbox.exe
```

On single-config generators like Ninja:

```powershell
.\build\honto_sandbox.exe
```

## Install As An SDK

You can install the engine as a reusable SDK for another machine or another game project:

```powershell
.\scripts\Build-HonToSdk.ps1 -InstallRoot C:\HonToSDK
```

That installs headers, the engine library, CMake package files, docs, templates, and example projects.

After that, another project can use:

```cmake
find_package(HonToEngine CONFIG REQUIRED)
target_link_libraries(MyGame PRIVATE HonTo::Engine)
```

When configuring that game project, point CMake at the installed SDK:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
```

If the target machine enforces Smart App Control, you can sign generated game executables during the build:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK" -DHONTO_SIGN_CERT_SHA1="YOUR_CERT_SHA1"
cmake --build build --config Release
```

## Create A New Game Project

You can generate a fresh external project from the starter template:

```powershell
.\scripts\New-HonToProject.ps1 -ProjectName MyGame -DestinationPath C:\Games
```

This creates a new buildable game project under `C:\Games\MyGame`.

## Visual Studio

If you generate the project with a recent Visual Studio generator, CMake may create a `.slnx` file instead of the older `.sln` format.

Open the generated solution file in `build`, then run `honto_sandbox`.

This repository is configured so Visual Studio shows:

- engine headers in Solution Explorer
- engine source files in Solution Explorer
- `honto_sandbox` as the startup project
- the workspace root as the debugger working directory

## Smart App Control

If Windows blocks the executable with Smart App Control or Code Integrity, changing `Debug` to `Release` usually is not enough.

The practical fix is to sign the built `.exe` with a trusted code-signing certificate.

For the Visual Studio project in `HonTo_Engine/HonTo_Engine`, optional post-build signing is already wired in:

- create `LocalSigning.props` next to `HonTo_Engine.vcxproj`
- base it on `LocalSigning.props.example`
- set either a `.pfx` certificate path or a certificate thumbprint from the Windows certificate store
- rebuild the project

The signing script is `scripts/Sign-Binary.ps1`.

For local development on a machine that keeps Smart App Control enabled, you can also create a current-user development signing certificate:

```powershell
.\scripts\Setup-LocalDevSigning.ps1
msbuild .\HonTo_Engine\HonTo_Engine.slnx /p:Configuration=Debug /p:Platform=x64
```

This creates a current-user code-signing certificate, trusts it for the current user, and writes `HonTo_Engine/HonTo_Engine/LocalSigning.props` so Visual Studio signs the built executable automatically.

To remove the local development certificate again:

```powershell
.\scripts\Remove-LocalDevSigning.ps1
```

## Project layout

- `engine/include/honto`: public engine API headers
- `engine/src`: engine implementation
- `sandbox/src/main.cpp`: example game built on the engine
- `examples/quickstart`: standalone example project for installed SDK usage
- `templates/HonToStarter`: starter project for external users
- `docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md`: separate source explanation file

## Easy code-first API

You can still build a game by subclassing `HonTo::Scene`, but the easier path now is a lambda-style stage builder.

If you want every user-facing name to start with `honto`, the recommended style is now the prefixed API in the `honto` namespace.

That means less boilerplate and more "declare what should happen" code:

```cpp
int main()
{
    return honto::hontoGame("My Game")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoClear(honto::hontoRGBA(12, 16, 28))
        .hontoOpenWindow(
            "honto Tools",
            720, 420,
            240, 135,
            [](honto::hontoStage& tools)
            {
                auto pulse = tools.hontoBox("pulse", 20.0f, 20.0f, honto::hontoRGBA(114, 236, 148))
                    .hontoAt(104.0f, 82.0f)
                    .hontoLayer(2);

                pulse.hontoAnimate()
                    .hontoScaleTo(1.9f)
                    .hontoPaintTo(honto::hontoRGBA(190, 255, 212))
                    .hontoIn(0.75f)
                    .hontoPingPong()
                    .hontoLoop()
                    .hontoPlay();
            }
        )
        .hontoPlay(
            [](honto::hontoStage& stage)
            {
                stage.hontoBackground(24, 28, 40);
                stage.hontoGravity(0.0f, 520.0f);

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

                auto sheet = honto::hontoFrameSheetTexture(
                    16,
                    16,
                    {
                        honto::hontoRGBA(92, 220, 128),
                        honto::hontoRGBA(112, 240, 148),
                        honto::hontoRGBA(84, 204, 122),
                        honto::hontoRGBA(126, 255, 164)
                    },
                    4
                );

                auto player = stage.hontoImage("player", sheet, 16.0f, 16.0f)
                    .hontoAt(24.0f, 40.0f)
                    .hontoLayer(3)
                    .hontoUseGravity()
                    .hontoCollideWithMap(world)
                    .hontoMoveLeftRight(110.0f)
                    .hontoJumpWhenPressed(honto::hontoKey::Space, 230.0f);

                player.hontoAnimateFrames()
                    .hontoTexture(sheet)
                    .hontoFrameSize(16, 16)
                    .hontoFrames({ 0, 1, 2, 3, 2, 1 })
                    .hontoFPS(10.0f)
                    .hontoLoop()
                    .hontoPlay();

                auto goal = stage.hontoBox("goal", 20.0f, 32.0f, honto::hontoRGBA(240, 196, 64))
                    .hontoAt(278.0f, 104.0f)
                    .hontoLayer(3);

                goal.hontoAnimate()
                    .hontoScaleTo(1.15f)
                    .hontoPaintTo(honto::hontoRGBA(255, 226, 120))
                    .hontoIn(0.85f)
                    .hontoPingPong()
                    .hontoLoop()
                    .hontoPlay();

                stage.hontoEveryFrame([player, goal, stage](float)
                {
                    if (player.hontoTouching(goal))
                    {
                        stage.hontoPlayTone(880, 90);
                        stage.hontoGoWithFade([](honto::hontoStage& clear)
                        {
                            clear.hontoBackground(18, 22, 34);
                        }, 0.65f);
                    }
                });
            }
        )
        .hontoRun();
}
```

Useful helpers in this style:

- `honto::hontoGame(...)`, `game.hontoWindow(...)`, `game.hontoWindowId(...)`, `game.hontoBorderless(...)`, `game.hontoResizable(...)`, `game.hontoOpacity(...)`, `game.hontoTopMost(...)`, `game.hontoOpenWindow(...)`
- `stage.hontoBox(...)`, `stage.hontoFill(...)`, `stage.hontoOutline(...)`, `stage.hontoTileMap(...)`, `stage.hontoText(...)`, `stage.hontoBar(...)`, `stage.hontoButton(...)`
- `actor.hontoAt(...)`, `actor.hontoMove(...)`, `actor.hontoPaint(...)`, `actor.hontoLayer(...)`
- `actor.hontoUseGravity()`, `actor.hontoCollideWithMap(...)`, `actor.hontoJumpWhenPressed(...)`
- `actor.hontoContainsPoint(...)`, `stage.hontoMousePosition()`, `stage.hontoWhenClicked(...)`
- `actor.hontoAnimate().hontoMoveTo(...).hontoScaleTo(...).hontoPaintTo(...).hontoIn(...).hontoPingPong().hontoLoop().hontoPlay()`
- `actor.hontoAnimateFrames().hontoTexture(...).hontoFrameSize(...).hontoFrames(...).hontoFPS(...).hontoLoop().hontoPlay()`
- `stage.hontoEveryFrame(...)`, `stage.hontoWhenPressed(...)`, `stage.hontoFind("name")`
- `stage.hontoPlayTone(...)`, `stage.hontoPlaySound(...)`, `stage.hontoPlayMusic(...)`, `stage.hontoPlayOnBus(...)`, `stage.hontoSetMasterVolume(...)`
- `stage.hontoGoWithFade(...)`, `stage.hontoGoWindowWithFade(...)`, `stage.hontoFocusWindow(...)`, `honto::hontoFade(...)`
- `stage.hontoWindowOpacity(...)`, `stage.hontoWindowBorderless(...)`, `stage.hontoWindowResizable(...)`, `stage.hontoWindowTopMost(...)`, `stage.hontoWindowSize(...)`, `stage.hontoWindowPosition(...)`, `stage.hontoWindowCenter()`
- `raycast.hontoDoor(...)`, `raycast.hontoDoorTexture(...)`, `raycast.hontoThingTexture(...)`, `raycast.hontoWeapon(...)`, `raycast.hontoWeaponBob(...)`, `raycast.hontoFog(...)`
- `honto::hontoLoadLevel(...)`, `honto::hontoSaveLevel(...)`, `honto::hontoFindLevelEntity(...)`

## Multiverse windows and DOOM helpers

The sandbox now demonstrates a simple "multiverse" workflow: one window can replace the scene inside another window and optionally focus it.

```cpp
stage.hontoGoWindowWithFade(
    "honto Multiverse Window",
    BuildMultiverseScene,
    0.7f,
    honto::hontoRGBA(22, 32, 58),
    true
);
```

Inside that other window, you can change the host window itself at runtime:

```cpp
stage.hontoWindowBorderless(true)
    .hontoWindowOpacity(0.82f)
    .hontoWindowResizable(false)
    .hontoWindowTopMost(true)
    .hontoWindowSize(1120, 640)
    .hontoWindowCenter();
```

The DOOM-style `RaycastView` also grew beyond plain walls:

- openable doors with `E` or `Enter`
- sprite billboards for enemies and props
- fog blending for depth
- shift-to-run movement
- a weapon overlay with bobbing and muzzle flash
- a toggleable minimap with `Tab` or `M`

If you prefer the more traditional engine style, the older scene-subclass API still works:

```cpp
class MyScene final : public HonTo::Scene
{
public:
    bool Setup() override
    {
        Add(HonTo::Fill(HonTo::VisibleSize(), HonTo::RGBA(24, 28, 40)));

        m_Player = Add(
            HonTo::Box(16.0f, 16.0f, HonTo::RGBA(92, 220, 128))
                .At(24.0f, 24.0f)
                .Z(3)
        );

        EveryFrame();
        return true;
    }

    void Update(float deltaTime) override
    {
        if (HonTo::Pressing(HonTo::Key::Right))
        {
            m_Player->SetPosition(m_Player->GetPosition() + HonTo::Vec2(90.0f * deltaTime, 0.0f));
        }
    }

private:
    std::shared_ptr<honto::Sprite> m_Player;
};
```

## Next milestones

1. richer widgets such as sliders, checkboxes, text input, and draggable panels
2. particle emitters, camera shake, and post-effect style helpers
3. enemy AI, triggers, and pathfinding helpers for real gameplay loops
4. editor tooling, scene inspectors, and stronger project serialization
5. content pipeline polish for sprite sheets, fonts, and packaged assets
6. scripting layer, such as Lua or C# embedding
