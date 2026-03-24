# HonTo Engine

`HonTo Engine` is a small C++ 2D game-engine starter designed to grow toward a Unity, Godot, or Unreal-style workflow for 2D projects.

Detailed Korean guide: `docs/HONTO_ENGINE_GUIDE_KO.md`

This first version focuses on the foundation:

- a native Windows window built with Win32
- a software 2D renderer with a pixel backbuffer
- scenes, entities, transforms, sprites, and simple rigid bodies
- a cocos2d-x style code-first scene graph with nodes, layers, and sprites
- a lambda-style "easy" API for gravity, tilemaps, collision, sprite-sheet animation, text/UI/buttons, mouse input, audio buses, level files, scene transitions, and multiple windows
- keyboard and mouse input
- level loading from `.honto`, HonTo JSON, and Tiled-style JSON
- a sandbox game that shows 2D platforming, tile collisions, PNG loading, HUD text/UI, button clicks, JSON/Tiled loading, audio mixer controls, and a DOOM-style 2.5D scene switch

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

- `honto::hontoGame(...)`, `game.hontoWindow(...)`, `game.hontoOpenWindow(...)`
- `stage.hontoBox(...)`, `stage.hontoFill(...)`, `stage.hontoOutline(...)`, `stage.hontoTileMap(...)`, `stage.hontoText(...)`, `stage.hontoBar(...)`, `stage.hontoButton(...)`
- `actor.hontoAt(...)`, `actor.hontoMove(...)`, `actor.hontoPaint(...)`, `actor.hontoLayer(...)`
- `actor.hontoUseGravity()`, `actor.hontoCollideWithMap(...)`, `actor.hontoJumpWhenPressed(...)`
- `actor.hontoContainsPoint(...)`, `stage.hontoMousePosition()`, `stage.hontoWhenClicked(...)`
- `actor.hontoAnimate().hontoMoveTo(...).hontoScaleTo(...).hontoPaintTo(...).hontoIn(...).hontoPingPong().hontoLoop().hontoPlay()`
- `actor.hontoAnimateFrames().hontoTexture(...).hontoFrameSize(...).hontoFrames(...).hontoFPS(...).hontoLoop().hontoPlay()`
- `stage.hontoEveryFrame(...)`, `stage.hontoWhenPressed(...)`, `stage.hontoFind("name")`
- `stage.hontoPlayTone(...)`, `stage.hontoPlaySound(...)`, `stage.hontoPlayMusic(...)`, `stage.hontoPlayOnBus(...)`, `stage.hontoSetMasterVolume(...)`
- `stage.hontoGoWithFade(...)`, `honto::hontoFade(...)`
- `honto::hontoLoadLevel(...)`, `honto::hontoSaveLevel(...)`, `honto::hontoFindLevelEntity(...)`

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

1. richer widgets such as panels, sliders, and draggable windows
2. sprite/font/audio asset import polish beyond BMP/PNG/WAV basics
3. particle and camera effect helpers
4. editor tooling and scene serialization workflow
5. editor and project format
6. scripting layer, such as Lua or C# embedding
