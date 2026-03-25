#include "honto/HonTo.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr const char* kPlaygroundWindow = "honto Playground";
    constexpr const char* kCodeLabWindow = "honto Code Lab";
    constexpr const char* kRuntimeWindow = "honto Runtime Window";
    constexpr int kWindowOuterMargin = 12;
    constexpr int kWindowGap = 10;
    constexpr int kPlaygroundWindowWidth = 1620;
    constexpr int kPlaygroundWindowHeight = 1080;
    constexpr int kCodeLabWindowWidth = 1040;
    constexpr int kCodeLabWindowHeight = 820;
    constexpr int kPlaygroundRenderWidth = 320;
    constexpr int kPlaygroundRenderHeight = 180;
    constexpr int kCodeLabRenderWidth = 320;
    constexpr int kCodeLabRenderHeight = 240;

    enum class LanguageMode
    {
        English,
        Korean
    };

    enum class DemoKind
    {
        Game,
        Scene,
        Label,
        Sprite,
        Actor,
        Physics,
        Animation,
        Particle,
        Camera,
        Ui,
        Audio,
        SaveLoad,
        Raycast,
        Window
    };

    struct DemoEntry
    {
        DemoKind kind;
        const char* buttonCode;
        const char* snippet;
        const char* titleEnglish;
        const char* titleKorean;
        const char* descriptionEnglish;
        const char* descriptionKorean;
    };

    struct WindowPlacement
    {
        int width = 0;
        int height = 0;
        int x = 0;
        int y = 0;
    };

    struct SampleWindowLayout
    {
        WindowPlacement playground;
        WindowPlacement codeLab;
        WindowPlacement runtime;
    };

    LanguageMode gLanguage = LanguageMode::English;
    DemoKind gCurrentDemo = DemoKind::Game;
    DemoKind gSelectedDemo = DemoKind::Game;
    std::string gSettingsWindowTitle;
    int gSceneFlip = 0;

    RECT GetDesktopWorkArea()
    {
        RECT workArea {};
        if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0) == FALSE)
        {
            workArea.left = 0;
            workArea.top = 0;
            workArea.right = GetSystemMetrics(SM_CXSCREEN);
            workArea.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        if (workArea.right <= workArea.left)
        {
            workArea.right = workArea.left + 1920;
        }

        if (workArea.bottom <= workArea.top)
        {
            workArea.bottom = workArea.top + 1080;
        }

        return workArea;
    }

    SIZE GetStandardWindowFrameSize()
    {
        RECT rect { 0, 0, 0, 0 };
        AdjustWindowRectEx(
            &rect,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            FALSE,
            0
        );

        SIZE size {};
        size.cx = rect.right - rect.left;
        size.cy = rect.bottom - rect.top;
        return size;
    }

    WindowPlacement MakePlacement(int requestedWidth, int requestedHeight, float scale)
    {
        WindowPlacement placement {};
        placement.width = std::max(1, static_cast<int>(std::lround(static_cast<float>(requestedWidth) * scale)));
        placement.height = std::max(1, static_cast<int>(std::lround(static_cast<float>(requestedHeight) * scale)));
        return placement;
    }

    SampleWindowLayout GetSampleWindowLayout()
    {
        const RECT workArea = GetDesktopWorkArea();
        const SIZE frameSize = GetStandardWindowFrameSize();
        const int frameWidth = static_cast<int>(frameSize.cx);
        const int frameHeight = static_cast<int>(frameSize.cy);
        const int workWidth = std::max(320, static_cast<int>(workArea.right - workArea.left));
        const int workHeight = std::max(180, static_cast<int>(workArea.bottom - workArea.top));
        const int availableWidth = std::max(320, workWidth - (kWindowOuterMargin * 2));
        const int availableHeight = std::max(180, workHeight - (kWindowOuterMargin * 2));
        const int outerWidthBudget = std::max(1, availableWidth - (frameWidth * 2) - kWindowGap);
        const int outerHeightBudget = std::max(1, availableHeight - frameHeight);
        const float widthScale = static_cast<float>(outerWidthBudget) / static_cast<float>(kPlaygroundWindowWidth + kCodeLabWindowWidth);
        const float heightScale = static_cast<float>(outerHeightBudget) / static_cast<float>(std::max(kPlaygroundWindowHeight, kCodeLabWindowHeight));
        const float sharedScale = std::max(0.25f, std::min(1.0f, std::min(widthScale, heightScale)));
        const int scaledGap = std::max(6, static_cast<int>(std::lround(static_cast<float>(kWindowGap) * sharedScale)));

        SampleWindowLayout layout {};
        layout.playground = MakePlacement(kPlaygroundWindowWidth, kPlaygroundWindowHeight, sharedScale);
        layout.codeLab = MakePlacement(kCodeLabWindowWidth, kCodeLabWindowHeight, sharedScale);

        const int playgroundOuterWidth = layout.playground.width + frameWidth;
        const int playgroundOuterHeight = layout.playground.height + frameHeight;
        const int codeLabOuterWidth = layout.codeLab.width + frameWidth;
        const int codeLabOuterHeight = layout.codeLab.height + frameHeight;
        const int combinedWidth = playgroundOuterWidth + scaledGap + codeLabOuterWidth;
        const int combinedHeight = std::max(playgroundOuterHeight, codeLabOuterHeight);
        const int originX = workArea.left + kWindowOuterMargin + std::max(0, (availableWidth - combinedWidth) / 2);
        const int originY = workArea.top + kWindowOuterMargin + std::max(0, (availableHeight - combinedHeight) / 2);

        layout.playground.x = originX;
        layout.playground.y = originY;
        layout.codeLab.x = originX + playgroundOuterWidth + scaledGap;
        layout.codeLab.y = originY;

        const float runtimeScale = std::min(
            1.0f,
            std::min(
                static_cast<float>(std::max(1, availableWidth - frameWidth)) / 720.0f,
                static_cast<float>(std::max(1, availableHeight - frameHeight)) / 420.0f
            )
        );
        layout.runtime = MakePlacement(720, 420, runtimeScale);
        layout.runtime.x = workArea.left + kWindowOuterMargin + std::max(0, (availableWidth - (layout.runtime.width + frameWidth)) / 2);
        layout.runtime.y = workArea.top + kWindowOuterMargin + std::max(0, (availableHeight - (layout.runtime.height + frameHeight)) / 2);

        return layout;
    }

    bool IsSettingsVisibleFor(const std::string& windowTitle)
    {
        return gSettingsWindowTitle == windowTitle;
    }

    void SetSettingsVisibleFor(const std::string& windowTitle, bool visible)
    {
        if (visible)
        {
            gSettingsWindowTitle = windowTitle;
        }
        else if (gSettingsWindowTitle == windowTitle)
        {
            gSettingsWindowTitle.clear();
        }
    }

    const std::array<DemoEntry, 14> kDemoEntries {
        DemoEntry {
            DemoKind::Game,
            "game.hontoCode(\"Window(...)\")",
            "return honto::hontoGame(\"honto Playground\")\n"
            "    .hontoCode(\"Window(1620, 1080)\")\n"
            "    .hontoCode(\"Render(320, 180)\")\n"
            "    .hontoCode(\"Clear(8, 10, 16)\")\n"
            "    .hontoPlay(BuildPlaygroundScene)\n"
            "    .hontoRun();",
            "Game Bootstrap",
            "게임 시작점",
            "Create the app and open the\nsample windows.",
            "앱을 만들고 샘플 창들을\n엽니다."
        },
        DemoEntry {
            DemoKind::Scene,
            "stage.hontoGoWithFade(...)",
            "if (Pressed(honto::hontoKey::Enter))\n"
            "{\n"
            "    gSceneFlip = 1 - gSceneFlip;\n"
            "    stage.hontoGoWithFade(BuildPlaygroundScene, 0.25f);\n"
            "}",
            "Scene And Transition",
            "씬과 전환",
            "Replace the current scene\nwith a fade transition.",
            "페이드 전환으로 현재 씬을\n교체합니다."
        },
        DemoEntry {
            DemoKind::Label,
            "title.hontoCode(\"Text(...)\")",
            "auto title = stage.hontoText(\"title\", \"HONTO LABEL\")\n"
            "    .hontoCode(\"TextScale(2)\")\n"
            "    .hontoCode(\"At(18, 66)\")\n"
            "    .hontoCode(\"Layer(3)\");",
            "Label And Text",
            "라벨과 텍스트",
            "Draw labels, titles, colors,\nand status messages.",
            "라벨, 제목, 색상, 상태 문구를\n화면에 그립니다."
        },
        DemoEntry {
            DemoKind::Sprite,
            "hero.hontoCode(\"UseTextureFrame(...)\")",
            "auto badge = stage.hontoImage(\"badge\",\n"
            "    \"sandbox/assets/honto_badge.png\", 48, 48)\n"
            "    .hontoAt(42, 82)\n"
            "    .hontoLayer(4);",
            "Sprite And Texture",
            "스프라이트와 텍스처",
            "Load PNG images and generated\ntextures into sprites.",
            "PNG 이미지와 생성 텍스처를\n스프라이트에 넣습니다."
        },
        DemoEntry {
            DemoKind::Actor,
            "player.hontoCode(\"MoveWithArrows(...)\")",
            "auto player = stage.hontoBox(\"player\", 16, 16,\n"
            "    honto::hontoRGBA(114, 236, 148))\n"
            "    .hontoCode(\"At(18, 110)\")\n"
            "    .hontoCode(\"MoveWithArrows(124)\");",
            "Actor And Input",
            "액터와 입력",
            "Create an actor and move it\nwith the keyboard.",
            "액터를 만들고 키보드로\n움직입니다."
        },
        DemoEntry {
            DemoKind::Physics,
            "stage.hontoCode(\"Gravity(...)\")",
            "stage.hontoCode(\"Gravity(0, 760)\");\n"
            "auto world = stage.hontoTileMap(...);\n"
            "auto player = stage.hontoBox(\"player\", 16, 16, color)\n"
            "    .hontoCode(\"UseGravity()\")\n"
            "    .hontoCode(\"CollideWithMap(\\\"world\\\")\");",
            "Physics And TileMap",
            "물리와 타일맵",
            "Apply gravity, jump, and collide\nwith solid tiles.",
            "중력, 점프, 타일 충돌을\n적용합니다."
        },
        DemoEntry {
            DemoKind::Animation,
            "actor.hontoAnimateFrames()",
            "player.hontoAnimateFrames()\n"
            "    .hontoFrameSize(16, 16)\n"
            "    .hontoFrames({0, 1, 2, 3})\n"
            "    .hontoLoop()\n"
            "    .hontoPlay();",
            "Animation",
            "애니메이션",
            "Play sprite-sheet frame\nanimation.",
            "스프라이트시트 프레임\n애니메이션을 재생합니다."
        },
        DemoEntry {
            DemoKind::Particle,
            "burst.hontoCode(\"Burst(...)\")",
            "auto burst = stage.hontoParticles(\"burst\", 18, 18);\n"
            "burst.hontoCode(\"VelocityRange(-26, -24, 26, -86)\");\n"
            "burst.hontoCode(\"ColorRange(255, 132, 214, 255, 255, 255, 255, 0)\");\n"
            "burst.hontoCode(\"Burst(18)\");",
            "Particles",
            "파티클",
            "Emit burst effects from code.",
            "코드에서 파티클 버스트를\n발생시킵니다."
        },
        DemoEntry {
            DemoKind::Camera,
            "stage.hontoCode(\"CameraFollowSmooth(...)\")",
            "stage.hontoCode(\"CameraFollowSmooth(\\\"player\\\", 1.0, 8.0)\");\n"
            "auto enemy = stage.hontoBox(\"enemy\", 16, 16, color)\n"
            "    .hontoCode(\"PatrolX(172, 244, 44)\");\n"
            "auto trigger = stage.hontoTrigger(\"gate\", 20, 32);",
            "Camera, Trigger, AI",
            "카메라, 트리거, AI",
            "Follow the player and react\nto enemies and triggers.",
            "플레이어를 따라가고 적과\n트리거에 반응합니다."
        },
        DemoEntry {
            DemoKind::Ui,
            "bar.hontoCode(\"BarValue(...)\")",
            "auto play = stage.hontoButton(\"music\", \"PLAY\", 84, 16);\n"
            "auto bar = stage.hontoBar(\"level\", 96, 8, 0.0f, color,\n"
            "    honto::hontoRGBA(12, 16, 24), honto::hontoRGBA(236, 245, 255));\n"
            "bar.hontoCode(\"BarValue(1.0)\");",
            "UI And Button",
            "UI와 버튼",
            "Click buttons and update\nprogress bars.",
            "버튼을 누르고 진행 바를\n바꿉니다."
        },
        DemoEntry {
            DemoKind::Audio,
            "stage.hontoCode(\"PlayMusic(...)\")",
            "stage.hontoCode(\"SetBusVolume(\\\"music\\\", 0.72)\");\n"
            "stage.hontoCode(\"PlayMusic(\\\"sandbox/assets/honto_theme.wav\\\")\");\n"
            "stage.hontoCode(\"PlayOnBus(\\\"effect\\\", \\\"sandbox/assets/honto_click.wav\\\")\");",
            "Audio",
            "오디오",
            "Play music, effects, and bus\nmixing code.",
            "배경음, 효과음, 버스 믹싱\n코드를 실행합니다."
        },
        DemoEntry {
            DemoKind::SaveLoad,
            "honto::hontoSaveLevel(...)",
            "editor.hontoCell(1, 0, '#');\n"
            "world.hontoCell(9, 4, '#');\n"
            "level.map[4][9] = '#';\n"
            "honto::hontoSaveLevel(\"sandbox/levels/bridge_demo.json\",\n"
            "    level);",
            "Save And Load",
            "저장과 불러오기",
            "Edit tiles at runtime, then save\nor load the level file.",
            "런타임에서 타일을 고친 뒤\n레벨 파일을 저장하거나 불러옵니다."
        },
        DemoEntry {
            DemoKind::Raycast,
            "doom.hontoCode(\"DoomControls(...)\")",
            "auto doom = stage.hontoRaycast(\"view\", 320, 112);\n"
            "doom.hontoCode(\"Map(\\\"##########|#...D....#|#.###.##.#\\\")\");\n"
            "doom.hontoCode(\"Player(1.5, 1.5, 0.0)\");\n"
            "doom.hontoCode(\"Door('D', 206, 168, 102, 0.65, 2.0)\");\n"
            "doom.hontoCode(\"DoomControls(3.0, 2.0)\");",
            "2.5D Raycast",
            "2.5D 레이캐스트",
            "Build a DOOM-style 3D view\nfrom a 2D map.",
            "2D 맵으로 둠 스타일 3D\n화면을 만듭니다."
        },
        DemoEntry {
            DemoKind::Window,
            "stage.hontoCode(\"OpenWindow(...)\")",
            "stage.hontoCode(\n"
            "    \"OpenWindow(\\\"honto Runtime Window\\\", 720, 420, 320, 180)\");",
            "Runtime Window",
            "런타임 창",
            "Click to create a new blank\nruntime window.",
            "클릭해서 새 빈 런타임 창을\n만듭니다."
        }
    };

    const char* ShortEnglishLabel(DemoKind kind)
    {
        switch (kind)
        {
        case DemoKind::Game: return "Game";
        case DemoKind::Scene: return "Scene";
        case DemoKind::Label: return "Label";
        case DemoKind::Sprite: return "Sprite";
        case DemoKind::Actor: return "Actor";
        case DemoKind::Physics: return "Physics";
        case DemoKind::Animation: return "Anim";
        case DemoKind::Particle: return "Particle";
        case DemoKind::Camera: return "Camera";
        case DemoKind::Ui: return "UI";
        case DemoKind::Audio: return "Audio";
        case DemoKind::SaveLoad: return "Save";
        case DemoKind::Raycast: return "Raycast";
        case DemoKind::Window: return "Window";
        default: return "Demo";
        }
    }

    const char* ShortKoreanLabel(DemoKind kind)
    {
        switch (kind)
        {
        case DemoKind::Game: return "게임";
        case DemoKind::Scene: return "씬";
        case DemoKind::Label: return "라벨";
        case DemoKind::Sprite: return "스프라이트";
        case DemoKind::Actor: return "액터";
        case DemoKind::Physics: return "물리";
        case DemoKind::Animation: return "애니";
        case DemoKind::Particle: return "파티클";
        case DemoKind::Camera: return "카메라";
        case DemoKind::Ui: return "UI";
        case DemoKind::Audio: return "오디오";
        case DemoKind::SaveLoad: return "저장";
        case DemoKind::Raycast: return "레이캐스트";
        case DemoKind::Window: return "창";
        default: return "데모";
        }
    }

    const std::vector<std::string> kRaycastMap {
        "##########",
        "#...D....#",
        "#.###.##.#",
        "#.#.....##",
        "#.#.###..#",
        "#...#....#",
        "###.#.##.#",
        "#.......##",
        "##########"
    };

    std::string L(const char* english, const char* korean)
    {
        return gLanguage == LanguageMode::English ? std::string(english) : std::string(korean);
    }

    const DemoEntry& FindDemo(DemoKind kind)
    {
        const auto found = std::find_if(
            kDemoEntries.begin(),
            kDemoEntries.end(),
            [kind](const DemoEntry& entry)
            {
                return entry.kind == kind;
            }
        );

        return found != kDemoEntries.end() ? *found : kDemoEntries.front();
    }

    honto::hontoColor DemoColor(DemoKind kind)
    {
        switch (kind)
        {
        case DemoKind::Game:
            return honto::hontoRGBA(122, 210, 255);
        case DemoKind::Scene:
            return honto::hontoRGBA(108, 148, 255);
        case DemoKind::Label:
            return honto::hontoRGBA(255, 232, 174);
        case DemoKind::Sprite:
            return honto::hontoRGBA(132, 255, 226);
        case DemoKind::Actor:
            return honto::hontoRGBA(114, 236, 148);
        case DemoKind::Physics:
            return honto::hontoRGBA(255, 196, 92);
        case DemoKind::Animation:
            return honto::hontoRGBA(122, 210, 255);
        case DemoKind::Particle:
            return honto::hontoRGBA(255, 132, 214);
        case DemoKind::Camera:
            return honto::hontoRGBA(255, 142, 96);
        case DemoKind::Ui:
            return honto::hontoRGBA(216, 150, 255);
        case DemoKind::Audio:
            return honto::hontoRGBA(132, 244, 255);
        case DemoKind::SaveLoad:
            return honto::hontoRGBA(132, 255, 226);
        case DemoKind::Raycast:
            return honto::hontoRGBA(255, 112, 112);
        default:
            return honto::hontoRGBA(236, 245, 255);
        }
    }

    std::shared_ptr<honto::Texture> MakeHeroSheet(honto::hontoColor a, honto::hontoColor b, honto::hontoColor c, honto::hontoColor d)
    {
        return honto::hontoFrameSheetTexture(16, 16, { a, b, c, d }, 4);
    }

    void ApplyWalkAnimation(const honto::hontoActor& actor, const std::shared_ptr<honto::Texture>& sheet, float fps = 10.0f)
    {
        actor.hontoAnimateFrames()
            .hontoTexture(sheet)
            .hontoFrameSize(16, 16)
            .hontoFrames({ 0, 1, 2, 3, 2, 1 })
            .hontoFPS(fps)
            .hontoLoop()
            .hontoPlay();
    }

    std::vector<std::string> MakeMap(int rows, int columns, char fill = '.')
    {
        return std::vector<std::string>(static_cast<std::size_t>(rows), std::string(static_cast<std::size_t>(columns), fill));
    }

    void FillSpan(std::vector<std::string>& map, int row, int beginColumn, int endColumn, char tile)
    {
        if (row < 0 || row >= static_cast<int>(map.size()))
        {
            return;
        }

        const int left = std::max(0, beginColumn);
        const int right = std::min(static_cast<int>(map[static_cast<std::size_t>(row)].size()) - 1, endColumn);
        for (int column = left; column <= right; ++column)
        {
            map[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = tile;
        }
    }

    honto::hontoLevel MakeBridgeLevel()
    {
        honto::hontoLevel level;
        level.title = "BRIDGE DEMO";
        level.tileSize = { 16.0f, 16.0f };
        level.map = {
            "....................",
            "....................",
            "....................",
            "....................",
            "########...#########"
        };
        return level;
    }

    std::vector<std::string> MakeBridgeEditorMap(const honto::hontoLevel& level)
    {
        std::string row = "...";
        if (!level.map.empty() && level.map.back().size() >= 11)
        {
            row[0] = level.map.back()[8];
            row[1] = level.map.back()[9];
            row[2] = level.map.back()[10];
        }

        return { row };
    }

    void ApplyBridgeEditorState(
        const honto::hontoLevel& level,
        const honto::hontoTileMapActor& world,
        const honto::hontoTileMapActor& editor)
    {
        world.hontoMap(level.map);
        editor.hontoMap(MakeBridgeEditorMap(level));
    }

    void BuildPlaygroundScene(honto::hontoStage& stage);
    void BuildCodeLabScene(honto::hontoStage& stage);
    void BuildRuntimeWindowScene(honto::hontoStage& stage);

    void RefreshWindows(
        const honto::hontoStage& stage,
        void(*currentBuilder)(honto::hontoStage&),
        const std::string& currentWindowTitle)
    {
        if (currentWindowTitle == kPlaygroundWindow)
        {
            stage.hontoGoWithFade(currentBuilder, 0.15f);
        }
        else
        {
            stage.hontoGoWindowWithFade(kPlaygroundWindow, BuildPlaygroundScene, 0.15f, honto::hontoRGBA(8, 10, 16), false);
        }

        if (currentWindowTitle == kCodeLabWindow)
        {
            stage.hontoGoWithFade(currentBuilder, 0.15f);
        }
        else
        {
            stage.hontoGoWindowWithFade(kCodeLabWindow, BuildCodeLabScene, 0.15f, honto::hontoRGBA(12, 16, 24), false);
        }

        if (currentWindowTitle == kRuntimeWindow)
        {
            stage.hontoGoWithFade(currentBuilder, 0.15f);
        }
        else
        {
            stage.hontoGoWindowWithFade(kRuntimeWindow, BuildRuntimeWindowScene, 0.15f, honto::hontoRGBA(12, 16, 24), false);
        }
    }

    std::shared_ptr<bool> AttachSettingsOverlay(
        honto::hontoStage& stage,
        void(*currentBuilder)(honto::hontoStage&),
        const std::string& currentWindowTitle)
    {
        auto open = std::make_shared<bool>(false);
        const honto::hontoVec2 visible = stage.hontoVisibleSize();
        const float panelWidth = std::min(visible.x - 32.0f, 210.0f);
        const float panelHeight = 132.0f;
        const float panelX = (visible.x - panelWidth) * 0.5f;
        const float panelY = (visible.y - panelHeight) * 0.5f;

        auto dim = stage.hontoFill("settings_dim", visible.x, visible.y, honto::hontoRGBA(0, 0, 0, 178))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(90)
            .hontoShow(false);
        auto panel = stage.hontoFill("settings_panel", panelWidth, panelHeight, honto::hontoRGBA(18, 24, 40, 244))
            .hontoAt(panelX, panelY)
            .hontoLayer(91)
            .hontoShow(false);
        auto border = stage.hontoOutline("settings_border", panelWidth, panelHeight, honto::hontoRGBA(104, 122, 160), 1)
            .hontoAt(panelX, panelY)
            .hontoLayer(92)
            .hontoShow(false);
        auto title = stage.hontoText("settings_title", "", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(panelX + 10.0f, panelY + 8.0f)
            .hontoLayer(93)
            .hontoShow(false);
        auto description = stage.hontoText("settings_desc", "", honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(panelX + 10.0f, panelY + 24.0f)
            .hontoLayer(93)
            .hontoShow(false);

        auto englishButton = stage.hontoButton("settings_en", "English", panelWidth - 20.0f, 16.0f)
            .hontoAt(panelX + 10.0f, panelY + 52.0f)
            .hontoLayer(93)
            .hontoShow(false);
        auto koreanButton = stage.hontoButton("settings_ko", "한국어", panelWidth - 20.0f, 16.0f)
            .hontoAt(panelX + 10.0f, panelY + 72.0f)
            .hontoLayer(93)
            .hontoShow(false);
        auto hideButton = stage.hontoButton("settings_hide", "", panelWidth - 20.0f, 16.0f)
            .hontoAt(panelX + 10.0f, panelY + 92.0f)
            .hontoLayer(93)
            .hontoShow(false);
        auto exitButton = stage.hontoButton("settings_exit", "", panelWidth - 20.0f, 16.0f)
            .hontoAt(panelX + 10.0f, panelY + 112.0f)
            .hontoLayer(93)
            .hontoShow(false);

        const std::vector<honto::hontoActor> overlayActors {
            dim,
            panel,
            border,
            title,
            description,
            englishButton,
            koreanButton,
            hideButton,
            exitButton
        };

        auto showOverlay = [overlayActors](bool visibleNow)
        {
            for (const auto& actor : overlayActors)
            {
                actor.hontoShow(visibleNow);
            }
        };

        stage.hontoWhenPressed(
            honto::hontoKey::Escape,
            [stage, open, showOverlay, currentWindowTitle]()
            {
                *open = !*open;
                SetSettingsVisibleFor(currentWindowTitle, *open);
                showOverlay(*open);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenClicked(
            englishButton,
            [stage, open, showOverlay, currentBuilder, currentWindowTitle]()
            {
                gLanguage = LanguageMode::English;
                *open = false;
                SetSettingsVisibleFor(currentWindowTitle, false);
                showOverlay(false);
                RefreshWindows(stage, currentBuilder, currentWindowTitle);
            }
        );

        stage.hontoWhenClicked(
            koreanButton,
            [stage, open, showOverlay, currentBuilder, currentWindowTitle]()
            {
                gLanguage = LanguageMode::Korean;
                *open = false;
                SetSettingsVisibleFor(currentWindowTitle, false);
                showOverlay(false);
                RefreshWindows(stage, currentBuilder, currentWindowTitle);
            }
        );

        stage.hontoWhenClicked(
            hideButton,
            [stage, open, showOverlay, currentWindowTitle]()
            {
                *open = false;
                SetSettingsVisibleFor(currentWindowTitle, false);
                showOverlay(false);

                if (currentWindowTitle == kPlaygroundWindow)
                {
                    stage.hontoFocusWindow(kCodeLabWindow);
                }
                else
                {
                    stage.hontoFocusWindow(kPlaygroundWindow);
                }

                stage.hontoHideThisWindow();
            }
        );

        stage.hontoWhenClicked(
            exitButton,
            [stage, open, showOverlay, currentWindowTitle]()
            {
                *open = false;
                SetSettingsVisibleFor(currentWindowTitle, false);
                showOverlay(false);
                stage.hontoCloseWindow(kPlaygroundWindow);
            }
        );

        stage.hontoEveryFrame(
            [open, title, description, englishButton, koreanButton, hideButton, exitButton](float)
            {
                if (!*open)
                {
                    return;
                }

                title.hontoTextValue(L("SETTINGS", "설정"));
                description.hontoTextValue(L("Choose language.\nPress ESC to close.", "언어를 선택하세요.\nESC로 닫을 수 있습니다."));
                hideButton.hontoTextValue(L("HIDE WINDOW", "창 숨기기"));
                exitButton.hontoTextValue(L("EXIT GAME", "게임 나가기"));

                const bool englishSelected = gLanguage == LanguageMode::English;
                englishButton.hontoButtonColors(
                    englishSelected ? honto::hontoRGBA(82, 128, 206) : honto::hontoRGBA(44, 62, 96),
                    honto::hontoRGBA(74, 98, 146),
                    honto::hontoRGBA(98, 136, 196),
                    honto::hontoRGBA(236, 245, 255)
                );
                koreanButton.hontoButtonColors(
                    !englishSelected ? honto::hontoRGBA(82, 128, 206) : honto::hontoRGBA(44, 62, 96),
                    honto::hontoRGBA(74, 98, 146),
                    honto::hontoRGBA(98, 136, 196),
                    honto::hontoRGBA(236, 245, 255)
                );
            }
        );

        return open;
    }

    void DrawPlaygroundHeader(honto::hontoStage& stage, DemoKind demo, const std::string& objective)
    {
        const DemoEntry& entry = FindDemo(demo);
        const honto::hontoVec2 visible = stage.hontoVisibleSize();
        stage.hontoFill("header_back", visible.x, 44.0f, honto::hontoRGBA(10, 14, 24, 232))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(20);
        stage.hontoOutline("header_border", visible.x, 44.0f, DemoColor(demo), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(21);
        stage.hontoText("header_title", L(entry.titleEnglish, entry.titleKorean), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(8.0f, 6.0f)
            .hontoLayer(22);
        stage.hontoText("header_code", entry.buttonCode, DemoColor(demo), 1)
            .hontoAt(8.0f, 18.0f)
            .hontoLayer(22);
        stage.hontoText("header_objective", objective, honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(8.0f, 30.0f)
            .hontoLayer(22);
    }

    void ApplyWindowLayout(const honto::hontoStage& stage, const std::string& windowTitle)
    {
        const SampleWindowLayout layout = GetSampleWindowLayout();
        stage.hontoWindowBorderless(false);
        stage.hontoWindowResizable(false);

        if (windowTitle == kPlaygroundWindow)
        {
            stage.hontoWindowSize(layout.playground.width, layout.playground.height);
            stage.hontoWindowPosition(layout.playground.x, layout.playground.y);
            return;
        }

        if (windowTitle == kCodeLabWindow)
        {
            stage.hontoWindowSize(layout.codeLab.width, layout.codeLab.height);
            stage.hontoWindowPosition(layout.codeLab.x, layout.codeLab.y);
            return;
        }

        if (windowTitle == kRuntimeWindow)
        {
            stage.hontoWindowSize(layout.runtime.width, layout.runtime.height);
            stage.hontoWindowPosition(layout.runtime.x, layout.runtime.y);
        }
    }

    void BuildGameDemo(honto::hontoStage& stage)
    {
        stage.hontoBackground(14, 18, 30);
        DrawPlaygroundHeader(stage, DemoKind::Game, L("This sample pair started from honto::hontoGame(...).", "이 샘플 창들은 honto::hontoGame(...)에서 시작됐습니다."));

        stage.hontoFill("card", 292.0f, 88.0f, honto::hontoRGBA(18, 24, 40))
            .hontoAt(14.0f, 60.0f)
            .hontoLayer(1);
        stage.hontoOutline("card_border", 292.0f, 88.0f, DemoColor(DemoKind::Game), 1)
            .hontoAt(14.0f, 60.0f)
            .hontoLayer(2);
        stage.hontoFill("left_window", 106.0f, 56.0f, honto::hontoRGBA(26, 42, 66))
            .hontoAt(30.0f, 78.0f)
            .hontoLayer(3);
        stage.hontoOutline("left_window_border", 106.0f, 56.0f, DemoColor(DemoKind::Game), 1)
            .hontoAt(30.0f, 78.0f)
            .hontoLayer(4);
        stage.hontoFill("right_window", 92.0f, 34.0f, honto::hontoRGBA(34, 52, 82))
            .hontoAt(184.0f, 90.0f)
            .hontoLayer(3);
        stage.hontoOutline("right_window_border", 92.0f, 34.0f, honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(184.0f, 90.0f)
            .hontoLayer(4);
        stage.hontoText("left_name", L("PLAYGROUND", "실행 창"), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(40.0f, 100.0f)
            .hontoLayer(5);
        stage.hontoText("right_name", L("CODE LAB", "코드 창"), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(196.0f, 102.0f)
            .hontoLayer(5);
        stage.hontoText("status", L("Left window runs demos. Right window chooses engine code.", "왼쪽 창은 데모를 실행하고 오른쪽 창은 엔진 코드를 선택합니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);
    }

    void BuildSceneDemo(honto::hontoStage& stage)
    {
        const bool alternate = (gSceneFlip % 2) != 0;
        stage.hontoBackground(alternate ? 26 : 12, alternate ? 32 : 18, alternate ? 54 : 38);
        DrawPlaygroundHeader(stage, DemoKind::Scene, L("Press Enter to fade-reload this scene.", "Enter를 눌러 이 씬을 페이드로 다시 불러오세요."));

        stage.hontoFill("scene_card", 292.0f, 88.0f, alternate ? honto::hontoRGBA(28, 40, 72) : honto::hontoRGBA(20, 28, 46))
            .hontoAt(14.0f, 60.0f)
            .hontoLayer(1);
        stage.hontoOutline("scene_border", 292.0f, 88.0f, DemoColor(DemoKind::Scene), 1)
            .hontoAt(14.0f, 60.0f)
            .hontoLayer(2);
        stage.hontoText("scene_state", alternate ? L("SCENE STATE B", "씬 상태 B") : L("SCENE STATE A", "씬 상태 A"), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(28.0f, 82.0f)
            .hontoLayer(3);
        stage.hontoText("scene_desc", L("Code Lab chooses the demo.\nEnter calls stage.hontoGoWithFade(...).", "Code Lab이 데모를 고릅니다.\nEnter가 stage.hontoGoWithFade(...)를 호출합니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(28.0f, 98.0f)
            .hontoLayer(3);
        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                gSceneFlip = 1 - gSceneFlip;
                stage.hontoGoWithFade(BuildPlaygroundScene, 0.25f);
            }
        );
    }

    void BuildLabelDemo(honto::hontoStage& stage)
    {
        stage.hontoBackground(16, 18, 28);
        DrawPlaygroundHeader(stage, DemoKind::Label, L("Labels draw titles, captions, and status lines.", "라벨은 제목, 설명, 상태 문구를 그립니다."));

        stage.hontoText("headline", L("HONTO LABEL", "HONTO 라벨"), DemoColor(DemoKind::Label), 2)
            .hontoAt(18.0f, 66.0f)
            .hontoLayer(3);
        stage.hontoText("line_a", L("stage.hontoText(...) can draw English and Korean.", "stage.hontoText(...)는 영어와 한국어를 모두 그릴 수 있습니다."), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(18.0f, 94.0f)
            .hontoLayer(3);
        stage.hontoText("line_b", L("Color, scale, and placement are part of the node setup.", "색상, 크기, 위치도 노드 설정에 포함됩니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(18.0f, 110.0f)
            .hontoLayer(3);
        stage.hontoText("line_c", L("This is a status message rendered as a label.", "이 문장도 라벨로 렌더링되는 상태 메시지입니다."), honto::hontoRGBA(132, 255, 226), 1)
            .hontoAt(18.0f, 136.0f)
            .hontoLayer(3);
    }

    void BuildSpriteDemo(honto::hontoStage& stage)
    {
        auto badgeTexture = honto::hontoLoadTexture("sandbox/assets/honto_badge.png");
        auto checkerTexture = honto::hontoCheckerTexture(32, 32, honto::hontoRGBA(132, 255, 226), honto::hontoRGBA(255, 232, 174), 4);
        auto heroSheet = MakeHeroSheet(
            honto::hontoRGBA(114, 236, 148),
            honto::hontoRGBA(146, 248, 182),
            honto::hontoRGBA(88, 198, 124),
            honto::hontoRGBA(202, 255, 214));

        stage.hontoBackground(16, 22, 32);
        DrawPlaygroundHeader(stage, DemoKind::Sprite, L("Sprites can use PNG files or generated textures.", "스프라이트는 PNG 파일이나 생성 텍스처를 사용할 수 있습니다."));

        stage.hontoImage("badge", badgeTexture, 44.0f, 44.0f)
            .hontoAt(26.0f, 78.0f)
            .hontoLayer(3);
        stage.hontoImage("checker", checkerTexture, 44.0f, 44.0f)
            .hontoAt(92.0f, 78.0f)
            .hontoLayer(3);
        auto hero = stage.hontoImage("hero", heroSheet, 20.0f, 20.0f)
            .hontoAt(164.0f, 90.0f)
            .hontoLayer(3);
        ApplyWalkAnimation(hero, heroSheet, 8.0f);
        stage.hontoText("sprite_status", L("PNG, checker texture, and sprite sheet are all visible.", "PNG, 체커 텍스처, 스프라이트시트가 모두 보입니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(18.0f, 160.0f)
            .hontoLayer(10);
    }

    void BuildActorDemo(honto::hontoStage& stage)
    {
        stage.hontoCode("Background(16, 20, 30)");
        DrawPlaygroundHeader(stage, DemoKind::Actor, L("Move and touch the goal.", "움직여서 목표 지점에 닿아보세요."));

        stage.hontoOutline("arena", 304.0f, 118.0f, honto::hontoRGBA(74, 92, 124), 1)
            .hontoAt(8.0f, 54.0f)
            .hontoLayer(1);
        auto player = stage.hontoBox("player", 16.0f, 16.0f, DemoColor(DemoKind::Actor))
            .hontoCode("At(18, 110)")
            .hontoCode("Layer(4)")
            .hontoCode("MoveWithArrows(124)")
            .hontoCode("KeepInside(10, 56, 294, 152)");
        auto goal = stage.hontoChecker("goal", 18.0f, 18.0f, honto::hontoRGBA(255, 236, 162), honto::hontoRGBA(255, 132, 92), 3)
            .hontoAt(272.0f, 70.0f)
            .hontoLayer(4);
        goal.hontoAnimate()
            .hontoScaleTo(1.12f)
            .hontoIn(0.8f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();
        auto status = stage.hontoText("status", L("Use arrow keys or WASD.", "방향키 또는 WASD를 사용하세요."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [player, goal, status](float)
            {
                if (player.hontoTouching(goal))
                {
                    status.hontoTextValue(L("Goal touched. Code works.", "목표 도달. 코드가 정상 동작합니다."));
                }
            }
        );
    }

    void BuildPhysicsDemo(honto::hontoStage& stage)
    {
        auto map = MakeMap(7, 20);
        FillSpan(map, 6, 0, 19, '#');
        FillSpan(map, 4, 3, 5, '#');
        FillSpan(map, 3, 9, 11, '#');
        FillSpan(map, 2, 15, 17, '#');

        stage.hontoCode("Background(18, 20, 32)");
        stage.hontoCode("Gravity(0, 760)");
        DrawPlaygroundHeader(stage, DemoKind::Physics, L("Jump to the beacon.", "비콘까지 점프해보세요."));

        auto world = stage.hontoTileMap("world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, 68.0f);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(142, 110, 82), true, true);

        auto player = stage.hontoBox("player", 16.0f, 16.0f, DemoColor(DemoKind::Physics))
            .hontoAt(16.0f, 132.0f)
            .hontoLayer(4)
            .hontoCode("UseGravity()")
            .hontoCode("CollideWithMap(\"world\")")
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);

        auto beacon = stage.hontoChecker("beacon", 16.0f, 24.0f, honto::hontoRGBA(255, 248, 184), DemoColor(DemoKind::Physics), 2)
            .hontoAt(276.0f, 98.0f)
            .hontoLayer(4);
        auto status = stage.hontoText("status", L("Space jumps.", "Space로 점프합니다."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [player, beacon, status](float)
            {
                if (player.Position().y > 180.0f)
                {
                    player.hontoAt(16.0f, 132.0f).hontoVelocity(0.0f, 0.0f);
                }

                if (player.hontoTouching(beacon))
                {
                    status.hontoTextValue(L("Tile collision and jump are active.", "타일 충돌과 점프가 작동 중입니다."));
                }
            }
        );
    }

    void BuildAnimationDemo(honto::hontoStage& stage)
    {
        auto heroSheet = MakeHeroSheet(
            honto::hontoRGBA(122, 210, 255),
            honto::hontoRGBA(154, 226, 255),
            honto::hontoRGBA(88, 188, 242),
            honto::hontoRGBA(182, 238, 255));

        stage.hontoBackground(14, 18, 30);
        DrawPlaygroundHeader(stage, DemoKind::Animation, L("Move the animated sprite and touch the crystal.", "애니메이션 스프라이트를 움직여 크리스털에 닿아보세요."));

        auto player = stage.hontoImage("player", heroSheet, 16.0f, 16.0f)
            .hontoAt(22.0f, 108.0f)
            .hontoLayer(4)
            .hontoMoveWithArrows(118.0f)
            .hontoKeepInside(8.0f, 58.0f, 296.0f, 146.0f);
        ApplyWalkAnimation(player, heroSheet, 10.0f);

        auto crystal = stage.hontoChecker("crystal", 16.0f, 16.0f, DemoColor(DemoKind::Animation), honto::hontoRGBA(255, 255, 255), 3)
            .hontoAt(236.0f, 92.0f)
            .hontoLayer(4);
        crystal.hontoAnimate()
            .hontoScaleTo(1.18f)
            .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
            .hontoIn(0.75f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto burst = stage.hontoParticles("burst", 18.0f, 18.0f);
        burst.hontoAt(236.0f, 92.0f).hontoLayer(5);
        burst.hontoSizeRange(2.0f, 4.0f);
        burst.hontoLifetimeRange(0.25f, 0.65f);
        burst.hontoVelocityRange({ -26.0f, -24.0f }, { 26.0f, -86.0f });
        burst.hontoColorRange(DemoColor(DemoKind::Animation), honto::hontoRGBA(255, 255, 255, 0));

        auto status = stage.hontoText("status", L("Animation is always playing.", "애니메이션은 항상 재생됩니다."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [player, crystal, burst, status](float)
            {
                if (player.hontoTouching(crystal))
                {
                    burst.hontoBurst(18);
                    status.hontoTextValue(L("Frame animation and particles fired.", "프레임 애니메이션과 파티클이 실행됐습니다."));
                }
            }
        );
    }

    void BuildParticleDemo(honto::hontoStage& stage)
    {
        auto timer = std::make_shared<float>(0.0f);
        stage.hontoCode("Background(18, 16, 30)");
        DrawPlaygroundHeader(stage, DemoKind::Particle, L("Particles burst from code every second.", "파티클이 코드로 주기적으로 터집니다."));

        auto core = stage.hontoChecker("core", 18.0f, 18.0f, DemoColor(DemoKind::Particle), honto::hontoRGBA(255, 255, 255), 3)
            .hontoAt(148.0f, 92.0f)
            .hontoLayer(4);
        core.hontoAnimate()
            .hontoScaleTo(1.24f)
            .hontoIn(0.72f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto burst = stage.hontoParticles("burst", 18.0f, 18.0f);
        burst.hontoAt(148.0f, 92.0f).hontoLayer(5);
        burst.hontoCode("SizeRange(2, 5)");
        burst.hontoCode("LifetimeRange(0.25, 0.7)");
        burst.hontoCode("VelocityRange(-28, -32, 28, -92)");
        burst.hontoCode("ColorRange(255, 132, 214, 255, 255, 255, 255, 0)");

        auto status = stage.hontoText("particle_status", L("Wait for the next burst pulse.", "다음 버스트 펄스를 기다려보세요."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [burst, status, timer](float deltaTime)
            {
                *timer += deltaTime;
                if (*timer >= 0.9f)
                {
                    *timer = 0.0f;
                    burst.hontoCode("Burst(20)");
                    status.hontoTextValue(L("stage.hontoParticles(...) fired a burst.", "stage.hontoParticles(...)가 버스트를 실행했습니다."));
                }
            }
        );
    }

    void BuildCameraDemo(honto::hontoStage& stage)
    {
        auto heroSheet = MakeHeroSheet(
            honto::hontoRGBA(255, 182, 126),
            honto::hontoRGBA(255, 208, 160),
            honto::hontoRGBA(224, 144, 88),
            honto::hontoRGBA(255, 226, 176));

        auto map = MakeMap(7, 40);
        FillSpan(map, 6, 0, 39, '#');
        FillSpan(map, 5, 10, 15, '#');
        FillSpan(map, 4, 20, 24, '#');
        FillSpan(map, 3, 31, 34, '#');

        stage.hontoCode("Background(16, 20, 30)");
        stage.hontoCode("Gravity(0, 760)");
        DrawPlaygroundHeader(stage, DemoKind::Camera, L("Avoid enemies and touch the trigger.", "적을 피하고 트리거에 닿아보세요."));

        stage.hontoFill("back", 640.0f, 112.0f, honto::hontoRGBA(34, 52, 74))
            .hontoAt(0.0f, 68.0f)
            .hontoLayer(0);

        auto world = stage.hontoTileMap("world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, 68.0f);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(102, 120, 96), true, true);

        auto player = stage.hontoImage("player", heroSheet, 16.0f, 16.0f)
            .hontoAt(16.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);
        ApplyWalkAnimation(player, heroSheet, 9.0f);
        stage.hontoCode("CameraFollowSmooth(\"player\", 1.0, 8.0)");

        auto patroller = stage.hontoBox("patroller", 16.0f, 16.0f, honto::hontoRGBA(255, 148, 96))
            .hontoAt(172.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoCode("PatrolX(172, 244, 44)");

        auto hunter = stage.hontoBox("hunter", 16.0f, 16.0f, honto::hontoRGBA(214, 86, 102))
            .hontoAt(364.0f, 116.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoCode("ChaseX(\"player\", 56, 18)");

        auto trigger = stage.hontoTrigger("trigger", 20.0f, 32.0f, true, DemoColor(DemoKind::Camera))
            .hontoAt(556.0f, 100.0f)
            .hontoLayer(4);
        auto status = stage.hontoText("status", L("Camera is following the player.", "카메라가 플레이어를 따라갑니다."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [player, patroller, hunter, trigger, status, stage](float)
            {
                if (player.Position().y > 180.0f)
                {
                    player.hontoAt(16.0f, 132.0f).hontoVelocity(0.0f, 0.0f);
                }

                if (player.hontoTouching(patroller) || player.hontoTouching(hunter))
                {
                    player.hontoAt(16.0f, 132.0f).hontoVelocity(0.0f, 0.0f);
                    stage.hontoCameraShake(2.4f, 0.2f, 24.0f);
                    status.hontoTextValue(L("Enemy hit. AI and camera shake fired.", "적에게 맞았습니다. AI와 카메라 흔들림이 작동했습니다."));
                }

                if (player.hontoTouching(trigger))
                {
                    status.hontoTextValue(L("Trigger entered. Scene logic works.", "트리거 진입. 씬 로직이 작동했습니다."));
                }
            }
        );
    }

    void BuildUiDemo(honto::hontoStage& stage)
    {
        auto musicOn = std::make_shared<bool>(false);
        auto effectOn = std::make_shared<bool>(false);
        auto uiOn = std::make_shared<bool>(false);

        stage.hontoCode("Background(18, 20, 30)");
        stage.hontoCode("SetMasterVolume(0.85)");
        stage.hontoCode("SetBusVolume(\"music\", 0.72)");
        stage.hontoCode("SetBusVolume(\"effect\", 0.92)");
        DrawPlaygroundHeader(stage, DemoKind::Ui, L("Click the console buttons.", "콘솔 버튼을 눌러보세요."));

        stage.hontoFill("console", 288.0f, 92.0f, honto::hontoRGBA(20, 28, 42))
            .hontoAt(16.0f, 70.0f)
            .hontoLayer(1);
        stage.hontoOutline("console_border", 288.0f, 92.0f, honto::hontoRGBA(86, 102, 132), 1)
            .hontoAt(16.0f, 70.0f)
            .hontoLayer(2);

        auto musicButton = stage.hontoButton("music", L("PLAY MUSIC", "음악 재생"), 84.0f, 16.0f)
            .hontoAt(28.0f, 84.0f)
            .hontoLayer(3);
        auto effectButton = stage.hontoButton("effect", L("FX PULSE", "효과음"), 84.0f, 16.0f)
            .hontoAt(118.0f, 84.0f)
            .hontoLayer(3);
        auto uiButton = stage.hontoButton("ui", L("UI BOOST", "UI 강화"), 84.0f, 16.0f)
            .hontoAt(208.0f, 84.0f)
            .hontoLayer(3);

        auto musicBar = stage.hontoBar("music_bar", 96.0f, 8.0f, 0.0f, DemoColor(DemoKind::Ui), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(40.0f, 118.0f)
            .hontoLayer(3);
        auto effectBar = stage.hontoBar("effect_bar", 96.0f, 8.0f, 0.0f, honto::hontoRGBA(255, 182, 96), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(40.0f, 136.0f)
            .hontoLayer(3);
        auto uiBar = stage.hontoBar("ui_bar", 96.0f, 8.0f, 0.0f, honto::hontoRGBA(132, 255, 226), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(184.0f, 118.0f)
            .hontoLayer(3);
        auto status = stage.hontoText("status", L("Mouse clicks drive UI code.", "마우스 클릭으로 UI 코드를 실행합니다."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 166.0f)
            .hontoLayer(10);

        stage.hontoWhenClicked(
            musicButton,
            [stage, musicOn, musicBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                *musicOn = true;
                musicBar.hontoBarValue(1.0f);
                stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue(L("Music bus is active.", "음악 버스가 활성화되었습니다."));
            }
        );

        stage.hontoWhenClicked(
            effectButton,
            [stage, effectOn, effectBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                *effectOn = true;
                effectBar.hontoBarValue(1.0f);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue(L("Effect bus is active.", "효과음 버스가 활성화되었습니다."));
            }
        );

        stage.hontoWhenClicked(
            uiButton,
            [stage, uiOn, uiBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                *uiOn = true;
                uiBar.hontoBarValue(1.0f);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue(L("UI state changed from code.", "코드로 UI 상태를 바꿨습니다."));
            }
        );
    }

    void BuildAudioDemo(honto::hontoStage& stage)
    {
        stage.hontoCode("Background(16, 18, 30)");
        stage.hontoCode("SetMasterVolume(0.85)");
        stage.hontoCode("SetBusVolume(\"music\", 0.72)");
        stage.hontoCode("SetBusVolume(\"effect\", 0.92)");
        DrawPlaygroundHeader(stage, DemoKind::Audio, L("Click the pads to play audio code.", "패드를 눌러 오디오 코드를 실행해보세요."));

        auto musicPad = stage.hontoButton("music_pad", L("MUSIC", "음악"), 86.0f, 18.0f)
            .hontoAt(26.0f, 82.0f)
            .hontoLayer(3);
        auto fxPad = stage.hontoButton("fx_pad", L("EFFECT", "효과음"), 86.0f, 18.0f)
            .hontoAt(118.0f, 82.0f)
            .hontoLayer(3);
        auto tonePad = stage.hontoButton("tone_pad", L("TONE", "톤"), 86.0f, 18.0f)
            .hontoAt(210.0f, 82.0f)
            .hontoLayer(3);
        auto musicBar = stage.hontoBar("audio_music_bar", 118.0f, 8.0f, 0.0f, DemoColor(DemoKind::Audio), honto::hontoRGBA(12, 16, 24), honto::hontoRGBA(236, 245, 255))
            .hontoAt(26.0f, 122.0f)
            .hontoLayer(3);
        auto fxBar = stage.hontoBar("audio_fx_bar", 118.0f, 8.0f, 0.0f, honto::hontoRGBA(255, 182, 96), honto::hontoRGBA(12, 16, 24), honto::hontoRGBA(236, 245, 255))
            .hontoAt(182.0f, 122.0f)
            .hontoLayer(3);
        auto status = stage.hontoText("audio_status", L("Use mouse clicks to test music, effects, and tones.", "마우스 클릭으로 음악, 효과음, 톤을 테스트하세요."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 160.0f)
            .hontoLayer(10);

        stage.hontoWhenClicked(
            musicPad,
            [stage, musicBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                musicBar.hontoBarValue(1.0f);
                stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
                status.hontoTextValue(L("Music playback started.", "배경음 재생이 시작되었습니다."));
            }
        );

        stage.hontoWhenClicked(
            fxPad,
            [stage, fxBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                fxBar.hontoBarValue(1.0f);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue(L("Effect bus played a click.", "효과음 버스가 클릭음을 재생했습니다."));
            }
        );

        stage.hontoWhenClicked(
            tonePad,
            [stage, musicBar, fxBar, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                musicBar.hontoBarValue(0.62f);
                fxBar.hontoBarValue(0.78f);
                stage.hontoPlayTone(880, 90);
                status.hontoTextValue(L("Generated tone playback fired.", "생성된 톤 재생이 실행되었습니다."));
            }
        );
    }

    void BuildSaveLoadDemo(honto::hontoStage& stage)
    {
        auto level = std::make_shared<honto::hontoLevel>(MakeBridgeLevel());

        stage.hontoBackground(16, 22, 32);
        stage.hontoGravity(0.0f, 760.0f);
        DrawPlaygroundHeader(stage, DemoKind::SaveLoad, L("Paint the bridge, then save or load.", "다리를 칠하고 저장하거나 불러오세요."));

        stage.hontoFill("editor_back", 80.0f, 26.0f, honto::hontoRGBA(22, 30, 46))
            .hontoAt(58.0f, 58.0f)
            .hontoLayer(1);
        for (int i = 0; i < 3; ++i)
        {
            stage.hontoOutline("cell_" + std::to_string(i), 18.0f, 18.0f, honto::hontoRGBA(86, 104, 132), 1)
                .hontoAt(66.0f + (static_cast<float>(i) * 18.0f), 62.0f)
                .hontoLayer(2);
        }

        auto editor = stage.hontoTileMap("editor", MakeBridgeEditorMap(*level), 18.0f, 18.0f);
        editor.hontoAt(66.0f, 62.0f);
        editor.hontoLayer(3);
        editor.hontoTile('#', DemoColor(DemoKind::SaveLoad), true, true);

        auto world = stage.hontoTileMap("world", level->map, 16.0f, 16.0f);
        world.hontoAt(0.0f, 92.0f);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(96, 124, 112), true, true);

        auto player = stage.hontoBox("player", 16.0f, 16.0f, DemoColor(DemoKind::SaveLoad))
            .hontoAt(18.0f, 124.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 285.0f);

        auto saveButton = stage.hontoButton("save", L("SAVE JSON", "JSON 저장"), 92.0f, 16.0f)
            .hontoAt(184.0f, 60.0f)
            .hontoLayer(3);
        auto loadButton = stage.hontoButton("load", L("LOAD JSON", "JSON 불러오기"), 92.0f, 16.0f)
            .hontoAt(184.0f, 80.0f)
            .hontoLayer(3);
        auto status = stage.hontoText("status", L("Click the three editor cells to build the bridge.", "위의 세 칸을 눌러 다리를 만드세요."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 166.0f)
            .hontoLayer(10);

        stage.hontoWhenClicked(
            saveButton,
            [stage, level, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                if (honto::hontoSaveLevel("sandbox/levels/bridge_demo.json", *level))
                {
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    status.hontoTextValue(L("Saved bridge_demo.json", "bridge_demo.json 저장 완료"));
                }
            }
        );

        stage.hontoWhenClicked(
            loadButton,
            [stage, level, world, editor, status]()
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    return;
                }

                honto::hontoLevel loaded = honto::hontoLoadLevel("sandbox/levels/bridge_demo.json");
                if (loaded.IsValid())
                {
                    *level = loaded;
                    ApplyBridgeEditorState(*level, world, editor);
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    status.hontoTextValue(L("Loaded bridge_demo.json", "bridge_demo.json 불러오기 완료"));
                }
            }
        );

        stage.hontoEveryFrame(
            [stage, editor, world, level, player, status](float)
            {
                if (IsSettingsVisibleFor(kPlaygroundWindow))
                {
                    if (player.Position().y > 180.0f)
                    {
                        player.hontoAt(18.0f, 124.0f).hontoVelocity(0.0f, 0.0f);
                    }

                    return;
                }

                int column = 0;
                int row = 0;
                if (stage.hontoMousePressed(honto::hontoMouse::Left) && editor.hontoWorldToCell(stage.hontoMousePosition(), column, row))
                {
                    if (column >= 0 && column < 3)
                    {
                        editor.hontoCell(column, 0, '#');
                        world.hontoCell(8 + column, 4, '#');
                        level->map[4][8 + column] = '#';
                        stage.hontoPlayTone(720 + (column * 60), 60);
                        status.hontoTextValue(L("Bridge updated in memory.", "메모리 안의 브리지가 수정되었습니다."));
                    }
                }

                if (player.Position().y > 180.0f)
                {
                    player.hontoAt(18.0f, 124.0f).hontoVelocity(0.0f, 0.0f);
                }
            }
        );
    }

    void BuildRaycastDemo(honto::hontoStage& stage)
    {
        auto wallA = honto::hontoCheckerTexture(32, 32, honto::hontoRGBA(152, 92, 76), honto::hontoRGBA(118, 60, 46), 8);
        auto wallB = honto::hontoCheckerTexture(32, 32, honto::hontoRGBA(82, 126, 164), honto::hontoRGBA(58, 88, 124), 6);
        auto doorTexture = honto::hontoCheckerTexture(32, 32, honto::hontoRGBA(196, 156, 94), honto::hontoRGBA(128, 82, 42), 4);
        auto impTexture = honto::hontoFrameSheetTexture(32, 48, { honto::hontoRGBA(222, 96, 72) }, 1);
        auto weaponTexture = honto::hontoCheckerTexture(96, 64, honto::hontoRGBA(64, 64, 72), honto::hontoRGBA(132, 132, 148), 6);

        stage.hontoCode("Background(10, 12, 18)");
        DrawPlaygroundHeader(stage, DemoKind::Raycast, L("Use E to open the door.", "E로 문을 열어보세요."));

        auto doom = stage.hontoRaycast("raycast", static_cast<float>(kPlaygroundRenderWidth), 112.0f);
        doom.hontoAt(0.0f, 52.0f);
        doom.hontoLayer(1);
        doom.hontoCode("Map(\"##########|#...D....#|#.###.##.#|#.#.....##|#.#.###..#|#...#....#|###.#.##.#|#.......##|##########\")")
            .hontoCode("Player(1.5, 1.5, 0.0)")
            .hontoCode("ViewDegrees(68)")
            .hontoCode("MoveSpeed(2.8)")
            .hontoCode("TurnSpeed(2.2)")
            .hontoCode("RunMultiplier(1.8)")
            .hontoCode("Floor(38, 36, 44)")
            .hontoCode("Ceiling(18, 22, 34)")
            .hontoCode("Fog(14, 18, 28, 0.42)")
            .hontoCode("Wall('#', 174, 112, 92)")
            .hontoWallTexture('#', wallA)
            .hontoCode("Wall('D', 206, 168, 102)")
            .hontoWallTexture('D', wallB)
            .hontoCode("Door('D', 206, 168, 102, 0.65, 2.0)")
            .hontoDoorTexture('D', doorTexture)
            .hontoThingTexture("imp", 7.4f, 2.0f, 0.9f, 1.3f, impTexture, honto::hontoRGBA(255, 190, 170), 0.08f, 4.6f)
            .hontoWeapon(weaponTexture, 136.0f, 80.0f, honto::hontoRGBA(255, 255, 255))
            .hontoCode("WeaponBob(3.8, 10.0)")
            .hontoCode("MiniMap(true, 7.0)")
            .hontoCode("DoomControls(3.0, 2.0)");

        stage.hontoText("status", L("W/S move, A/D strafe, arrows turn.", "W/S 이동, A/D 횡이동, 방향키 회전."), honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, 166.0f)
            .hontoLayer(10);
    }

    void BuildWindowDemo(honto::hontoStage& stage)
    {
        stage.hontoBackground(14, 18, 28);
        DrawPlaygroundHeader(stage, DemoKind::Window, L("Click code to open a runtime window.", "코드를 클릭해 런타임 창을 여세요."));
        stage.hontoFill("panel", 288.0f, 98.0f, honto::hontoRGBA(18, 24, 40))
            .hontoAt(16.0f, 62.0f)
            .hontoLayer(1);
        stage.hontoOutline("panel_border", 288.0f, 98.0f, DemoColor(DemoKind::Window), 1)
            .hontoAt(16.0f, 62.0f)
            .hontoLayer(2);
        stage.hontoText("title", L("Click the code again in Code Lab.", "Code Lab에서 코드를 다시 눌러보세요."), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(24.0f, 76.0f)
            .hontoLayer(3);
        stage.hontoText("desc", L("stage.hontoOpenWindow(...)\nspawns a new window at runtime.", "stage.hontoOpenWindow(...)\n코드로 런타임 창을 생성합니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(24.0f, 92.0f)
            .hontoLayer(3);
    }

    void BuildRuntimeWindowScene(honto::hontoStage& stage)
    {
        ApplyWindowLayout(stage, kRuntimeWindow);
        stage.hontoBackground(10, 14, 22);
        stage.hontoFill("blank", static_cast<float>(kPlaygroundRenderWidth), static_cast<float>(kPlaygroundRenderHeight), honto::hontoRGBA(10, 14, 22))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(0);
        stage.hontoText("title", L("Runtime Window Created", "런타임 창이 생성되었습니다"), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(10.0f, 10.0f)
            .hontoLayer(2);
        stage.hontoText("desc", L("This blank window came from\nstage.hontoOpenWindow(...)", "이 빈 창은\nstage.hontoOpenWindow(...)로 생성되었습니다"), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(10.0f, 24.0f)
            .hontoLayer(2);
        AttachSettingsOverlay(stage, BuildRuntimeWindowScene, kRuntimeWindow);
    }

    void BuildPlaygroundScene(honto::hontoStage& stage)
    {
        ApplyWindowLayout(stage, kPlaygroundWindow);

        switch (gCurrentDemo)
        {
        case DemoKind::Game:
            BuildGameDemo(stage);
            break;
        case DemoKind::Scene:
            BuildSceneDemo(stage);
            break;
        case DemoKind::Label:
            BuildLabelDemo(stage);
            break;
        case DemoKind::Sprite:
            BuildSpriteDemo(stage);
            break;
        case DemoKind::Actor:
            BuildActorDemo(stage);
            break;
        case DemoKind::Physics:
            BuildPhysicsDemo(stage);
            break;
        case DemoKind::Animation:
            BuildAnimationDemo(stage);
            break;
        case DemoKind::Particle:
            BuildParticleDemo(stage);
            break;
        case DemoKind::Camera:
            BuildCameraDemo(stage);
            break;
        case DemoKind::Ui:
            BuildUiDemo(stage);
            break;
        case DemoKind::Audio:
            BuildAudioDemo(stage);
            break;
        case DemoKind::SaveLoad:
            BuildSaveLoadDemo(stage);
            break;
        case DemoKind::Raycast:
            BuildRaycastDemo(stage);
            break;
        case DemoKind::Window:
            BuildWindowDemo(stage);
            break;
        }

        AttachSettingsOverlay(stage, BuildPlaygroundScene, kPlaygroundWindow);
    }

    void RunDemoFromCodeLab(const honto::hontoStage& stage, DemoKind demo)
    {
        if (IsSettingsVisibleFor(kCodeLabWindow))
        {
            return;
        }

        gSelectedDemo = demo;
        gCurrentDemo = demo;

        if (demo == DemoKind::Window)
        {
            stage.hontoCode("OpenWindow(\"honto Runtime Window\", 720, 420, 320, 180)");
        }

        stage.hontoGoWindowWithFade(kPlaygroundWindow, BuildPlaygroundScene, 0.25f, honto::hontoRGBA(8, 10, 16), true);
        stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
    }

    void BuildCodeLabScene(honto::hontoStage& stage)
    {
        ApplyWindowLayout(stage, kCodeLabWindow);
        stage.hontoBackground(12, 16, 24);

        const honto::hontoVec2 visible = stage.hontoVisibleSize();
        const float panelX = 6.0f;
        const float panelY = 6.0f;
        const float panelWidth = visible.x - 12.0f;
        const float panelHeight = visible.y - 12.0f;
        const int gridColumns = 4;
        const float gridGapX = 4.0f;
        const float gridGapY = 4.0f;
        const float buttonX = 12.0f;
        const float buttonY = 32.0f;
        const float buttonWidth = (visible.x - 24.0f - (gridGapX * static_cast<float>(gridColumns - 1))) / static_cast<float>(gridColumns);
        const float buttonHeight = 16.0f;
        const int gridRows = static_cast<int>((kDemoEntries.size() + static_cast<std::size_t>(gridColumns) - 1) / static_cast<std::size_t>(gridColumns));
        const float detailPanelY = buttonY + (static_cast<float>(gridRows) * buttonHeight) + (static_cast<float>(gridRows - 1) * gridGapY) + 10.0f;
        const float detailPanelHeight = std::max(112.0f, visible.y - detailPanelY - 10.0f);

        stage.hontoFill("lab_panel", panelWidth, panelHeight, honto::hontoRGBA(16, 22, 34))
            .hontoAt(panelX, panelY)
            .hontoLayer(1);
        stage.hontoOutline("lab_border", panelWidth, panelHeight, honto::hontoRGBA(72, 94, 126), 1)
            .hontoAt(panelX, panelY)
            .hontoLayer(2);
        stage.hontoFill("detail_panel", panelWidth - 8.0f, detailPanelHeight, honto::hontoRGBA(20, 26, 40))
            .hontoAt(12.0f, detailPanelY)
            .hontoLayer(1);
        stage.hontoOutline("detail_border", panelWidth - 8.0f, detailPanelHeight, honto::hontoRGBA(72, 94, 126), 1)
            .hontoAt(12.0f, detailPanelY)
            .hontoLayer(2);

        stage.hontoText("title", L("HONTO CODE LAB", "HONTO 코드 연구실"), honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(12.0f, 10.0f)
            .hontoLayer(3);
        stage.hontoText("hint", L("Click to run in Playground.", "클릭하면 Playground에서 실행됩니다."), honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(12.0f, 22.0f)
            .hontoLayer(3);

        std::vector<honto::hontoActor> codeButtons;
        codeButtons.reserve(kDemoEntries.size());

        for (std::size_t i = 0; i < kDemoEntries.size(); ++i)
        {
            const DemoEntry& entry = kDemoEntries[i];
            const int column = static_cast<int>(i % static_cast<std::size_t>(gridColumns));
            const int row = static_cast<int>(i / static_cast<std::size_t>(gridColumns));
            auto button = stage.hontoButton(
                    "code_" + std::to_string(i),
                    L(ShortEnglishLabel(entry.kind), ShortKoreanLabel(entry.kind)),
                    buttonWidth,
                    buttonHeight,
                    honto::hontoRGBA(34, 46, 70),
                    honto::hontoRGBA(60, 86, 130),
                    honto::hontoRGBA(88, 126, 188),
                    honto::hontoRGBA(236, 245, 255),
                    honto::hontoRGBA(255, 255, 255),
                    1)
                .hontoAt(
                    buttonX + (static_cast<float>(column) * (buttonWidth + gridGapX)),
                    buttonY + (static_cast<float>(row) * (buttonHeight + gridGapY)))
                .hontoLayer(3);
            codeButtons.push_back(button);

            stage.hontoWhenClicked(
                button,
                [stage, entry]()
                {
                    RunDemoFromCodeLab(stage, entry.kind);
                }
            );
        }

        auto detailTitle = stage.hontoText("detail_title", "", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(18.0f, detailPanelY + 8.0f)
            .hontoLayer(3);
        auto detailDesc = stage.hontoText("detail_desc", "", honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(18.0f, detailPanelY + 26.0f)
            .hontoLayer(3);
        auto detailCodeTitle = stage.hontoText("detail_code_title", "", DemoColor(gSelectedDemo), 1)
            .hontoAt(18.0f, detailPanelY + 62.0f)
            .hontoLayer(3);
        auto detailCode = stage.hontoText("detail_code", "", honto::hontoRGBA(255, 236, 162), 1)
            .hontoAt(18.0f, detailPanelY + 76.0f)
            .hontoLayer(3);
        auto detailRun = stage.hontoText("detail_run", "", honto::hontoRGBA(132, 255, 226), 1)
            .hontoAt(18.0f, detailPanelY + detailPanelHeight - 22.0f)
            .hontoLayer(3);

        stage.hontoEveryFrame(
            [codeButtons, detailTitle, detailDesc, detailCodeTitle, detailCode, detailRun](float)
            {
                const DemoEntry& entry = FindDemo(gSelectedDemo);

                for (std::size_t i = 0; i < codeButtons.size(); ++i)
                {
                    const bool selected = kDemoEntries[i].kind == gSelectedDemo;
                    codeButtons[i].hontoButtonColors(
                        selected ? DemoColor(kDemoEntries[i].kind) : honto::hontoRGBA(34, 46, 70),
                        honto::hontoRGBA(60, 86, 130),
                        honto::hontoRGBA(88, 126, 188),
                        honto::hontoRGBA(236, 245, 255)
                    );
                }

                detailTitle.hontoTextValue(L(entry.titleEnglish, entry.titleKorean));
                detailDesc.hontoTextValue(L(entry.descriptionEnglish, entry.descriptionKorean));
                detailCodeTitle.hontoTextValue(L("COMMAND", "명령"));
                detailCode.hontoTextValue(entry.buttonCode);
                detailCode.hontoPaint(DemoColor(entry.kind));
                detailRun.hontoTextValue(
                    entry.kind == DemoKind::Window
                        ? L("Click to open the runtime window.", "클릭하면 런타임 창을 엽니다.")
                        : L("Click to load this demo.", "클릭하면 이 데모를 불러옵니다.")
                );
            }
        );

        AttachSettingsOverlay(stage, BuildCodeLabScene, kCodeLabWindow);
    }
    int RunHonToSample()
    {
        return honto::hontoGame(kPlaygroundWindow)
            .hontoCode("WindowId(\"honto Playground\")")
            .hontoCode("Window(1620, 1080)")
            .hontoCode("Borderless(false)")
            .hontoCode("Resizable(false)")
            .hontoCode("Render(320, 180)")
            .hontoCode("Clear(8, 10, 16)")
            .hontoPlay(BuildPlaygroundScene)
            .hontoOpenWindow(
                kCodeLabWindow,
                kCodeLabWindowWidth,
                kCodeLabWindowHeight,
                kCodeLabRenderWidth,
                kCodeLabRenderHeight,
                BuildCodeLabScene,
                honto::hontoRGBA(12, 16, 24),
                false
            )
            .hontoRun();
    }
}

int main()
{
    return RunHonToSample();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return RunHonToSample();
}
