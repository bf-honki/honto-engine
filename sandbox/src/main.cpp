#include "honto/HonTo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr float kRenderWidth = 320.0f;
    constexpr float kRenderHeight = 180.0f;
    constexpr float kHubPanelHeight = 40.0f;
    constexpr float kLessonPanelHeight = 56.0f;
    constexpr float kLessonStatusY = 58.0f;
    constexpr float kLessonWorldTop = 68.0f;
    constexpr std::size_t kLessonCount = 7;

    struct LessonInfo
    {
        std::string title;
        std::string summary;
        std::array<std::string, 3> code;
    };

    const std::array<LessonInfo, kLessonCount> kLessons {
        LessonInfo {
            "STAGE AND ACTOR",
            "Move an actor and collect stage goals.",
            {
                "auto player = stage.hontoBox(\"player\", 16, 16, color);",
                "player.hontoAt(24, 96).hontoMoveWithArrows(120);",
                "if (player.hontoTouching(goal)) { status = \"CLEAR\"; }"
            }
        },
        LessonInfo {
            "PHYSICS AND TILEMAP",
            "Use gravity, jumping, and solid tiles.",
            {
                "stage.hontoGravity(0.0f, 760.0f);",
                "auto world = stage.hontoTileMap(\"world\", map, 16, 16);",
                "player.hontoUseGravity().hontoCollideWithMap(world);"
            }
        },
        LessonInfo {
            "ANIMATION AND PARTICLES",
            "Play frame animations and burst effects.",
            {
                "player.hontoAnimateFrames().hontoFrames({0,1,2,3});",
                "auto fx = stage.hontoParticles(\"fx\", 18, 18);",
                "fx.hontoBurst(20);"
            }
        },
        LessonInfo {
            "CAMERA, TRIGGER, AI",
            "Follow the player and react to enemies.",
            {
                "stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);",
                "enemy.hontoPatrolX(160, 260, 44).hontoChaseX(player, 58);",
                "stage.hontoWhenTouching(player, trigger, [](){ ... });"
            }
        },
        LessonInfo {
            "UI, BUTTON, AUDIO",
            "Drive a console with buttons, bars, and sound.",
            {
                "auto run = stage.hontoButton(\"run\", \"RUN\", 84, 16);",
                "stage.hontoWhenClicked(run, [](){ stage.hontoPlayMusic(...); });",
                "meter.hontoBarValue(1.0f);"
            }
        },
        LessonInfo {
            "LEVEL, SAVE, LOAD",
            "Edit tiles at runtime and save the map file.",
            {
                "honto::hontoLevel level = honto::hontoLoadLevel(path);",
                "world.hontoCell(column, row, '#');",
                "honto::hontoSaveLevel(path, level);"
            }
        },
        LessonInfo {
            "2.5D RAYCAST",
            "Use a 2D map to build a DOOM-style view.",
            {
                "auto doom = stage.hontoRaycast(\"doom\", 320, 112);",
                "doom.hontoMap(map).hontoDoor('D', color).hontoDoomControls();",
                "if (doom.hontoPlayerPosition().x > 7.3f) { ... }"
            }
        }
    };

    const std::array<float, kLessonCount> kHubDoorX {
        44.0f,
        194.0f,
        344.0f,
        494.0f,
        644.0f,
        794.0f,
        944.0f
    };

    const std::vector<std::string> kDoomLessonMap {
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

    std::array<bool, kLessonCount> gLessonCleared {};

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

    honto::hontoColor LessonColor(std::size_t index)
    {
        switch (index)
        {
        case 0:
            return honto::hontoRGBA(114, 236, 148);
        case 1:
            return honto::hontoRGBA(255, 196, 92);
        case 2:
            return honto::hontoRGBA(122, 210, 255);
        case 3:
            return honto::hontoRGBA(255, 142, 96);
        case 4:
            return honto::hontoRGBA(216, 150, 255);
        case 5:
            return honto::hontoRGBA(132, 255, 226);
        default:
            return honto::hontoRGBA(255, 112, 112);
        }
    }

    bool LessonUnlocked(std::size_t index)
    {
        return index == 0 || gLessonCleared[index - 1];
    }

    bool AcademyCompleted()
    {
        return std::all_of(gLessonCleared.begin(), gLessonCleared.end(), [](bool cleared) { return cleared; });
    }

    std::string LessonStateText(std::size_t index)
    {
        if (gLessonCleared[index])
        {
            return "CLEAR";
        }

        if (LessonUnlocked(index))
        {
            return "OPEN";
        }

        return "LOCK";
    }

    honto::hontoColor LessonStateColor(std::size_t index)
    {
        if (gLessonCleared[index])
        {
            return LessonColor(index);
        }

        if (LessonUnlocked(index))
        {
            return honto::hontoRGBA(236, 245, 255);
        }

        return honto::hontoRGBA(104, 118, 142);
    }

    void ResetActor(const honto::hontoActor& actor, const honto::hontoVec2& position)
    {
        actor.hontoAt(position).hontoVelocity(0.0f, 0.0f);
    }

    void PlaySuccess(const honto::hontoStage& stage)
    {
        stage.hontoPlayTone(920, 90);
        stage.hontoPlayAlias("SystemAsterisk");
    }

    void PlayLocked(const honto::hontoStage& stage)
    {
        stage.hontoPlayTone(240, 110);
        stage.hontoPlayAlias("SystemHand");
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

    void DrawCodeLines(
        honto::hontoStage& stage,
        const std::string& prefix,
        float x,
        float y,
        const std::array<std::string, 3>& lines,
        honto::hontoColor color,
        int layer)
    {
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            stage.hontoText(prefix + std::to_string(i), lines[i], color, 1)
                .hontoAt(x, y + (static_cast<float>(i) * 8.0f))
                .hontoLayer(layer);
        }
    }

    void DrawLessonPanel(honto::hontoStage& stage, std::size_t lessonIndex)
    {
        const LessonInfo& lesson = kLessons[lessonIndex];

        stage.hontoFill("lesson_panel", kRenderWidth, kLessonPanelHeight, honto::hontoRGBA(10, 14, 24, 232))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(30);
        stage.hontoOutline("lesson_panel_border", kRenderWidth, kLessonPanelHeight, LessonColor(lessonIndex), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(31);
        stage.hontoText(
                "lesson_title",
                "LEVEL " + std::to_string(lessonIndex + 1) + "  " + lesson.title,
                honto::hontoRGBA(238, 245, 255),
                1)
            .hontoAt(6.0f, 4.0f)
            .hontoLayer(32);
        stage.hontoText("lesson_summary", lesson.summary + "  ESC HUB", honto::hontoRGBA(178, 196, 222), 1)
            .hontoAt(6.0f, 14.0f)
            .hontoLayer(32);
        DrawCodeLines(stage, "lesson_code_", 6.0f, 26.0f, lesson.code, honto::hontoRGBA(255, 236, 162), 32);
    }

    honto::hontoLevel MakeBridgeLessonLevel()
    {
        honto::hontoLevel level;
        level.title = "BRIDGE BUILDER";
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

    void BuildAcademyHubScene(honto::hontoStage& stage);
    void BuildLessonStageScene(honto::hontoStage& stage);
    void BuildLessonPhysicsScene(honto::hontoStage& stage);
    void BuildLessonAnimationScene(honto::hontoStage& stage);
    void BuildLessonCameraScene(honto::hontoStage& stage);
    void BuildLessonUiScene(honto::hontoStage& stage);
    void BuildLessonLevelScene(honto::hontoStage& stage);
    void BuildLessonDoomScene(honto::hontoStage& stage);

    void ReturnToHub(const honto::hontoStage& stage)
    {
        stage.hontoStopAudioBus("music");
        stage.hontoGoWithFade(BuildAcademyHubScene, 0.6f);
    }

    void GoToLesson(const honto::hontoStage& stage, std::size_t lessonIndex)
    {
        switch (lessonIndex)
        {
        case 0:
            stage.hontoGoWithFade(BuildLessonStageScene, 0.6f);
            break;
        case 1:
            stage.hontoGoWithFade(BuildLessonPhysicsScene, 0.6f);
            break;
        case 2:
            stage.hontoGoWithFade(BuildLessonAnimationScene, 0.6f);
            break;
        case 3:
            stage.hontoGoWithFade(BuildLessonCameraScene, 0.6f);
            break;
        case 4:
            stage.hontoGoWithFade(BuildLessonUiScene, 0.6f);
            break;
        case 5:
            stage.hontoGoWithFade(BuildLessonLevelScene, 0.6f);
            break;
        default:
            stage.hontoGoWithFade(BuildLessonDoomScene, 0.6f);
            break;
        }
    }

    void BuildAcademyHubScene(honto::hontoStage& stage)
    {
        auto selectedLesson = std::make_shared<int>(-1);
        auto heroSheet = MakeHeroSheet(
            honto::hontoRGBA(114, 236, 148),
            honto::hontoRGBA(140, 255, 176),
            honto::hontoRGBA(88, 204, 122),
            honto::hontoRGBA(168, 255, 198));

        std::vector<std::string> map = MakeMap(8, 72);
        FillSpan(map, 6, 0, 71, '#');
        FillSpan(map, 7, 0, 71, '#');

        stage.hontoStopAudioBus("music");
        stage.hontoBackground(14, 18, 28);

        stage.hontoFill("academy_sky", 1152.0f, 128.0f, honto::hontoRGBA(28, 40, 72))
            .hontoAt(0.0f, 52.0f)
            .hontoLayer(0);
        stage.hontoFill("academy_haze", 1152.0f, 36.0f, honto::hontoRGBA(62, 86, 138, 120))
            .hontoAt(0.0f, 98.0f)
            .hontoLayer(0);

        auto world = stage.hontoTileMap("academy_world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, 52.0f);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(88, 114, 82), true, true);

        auto player = stage.hontoImage("academy_player", heroSheet, 16.0f, 16.0f)
            .hontoAt(20.0f, 132.0f)
            .hontoLayer(5)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 280.0f);
        ApplyWalkAnimation(player, heroSheet, 9.0f);
        stage.hontoCameraFollowSmooth(player, 1.0f, 7.0f);

        stage.hontoFill("hub_panel", kRenderWidth, kHubPanelHeight, honto::hontoRGBA(10, 14, 24, 226))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(20);
        stage.hontoOutline("hub_panel_border", kRenderWidth, kHubPanelHeight, honto::hontoRGBA(96, 122, 168), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(21);
        stage.hontoText("hub_title", "HONTO ENGINE ACADEMY", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(6.0f, 4.0f)
            .hontoLayer(22);
        stage.hontoText("hub_hint", "A/D MOVE  SPACE JUMP  ENTER LESSON  LEFT TO RIGHT", honto::hontoRGBA(190, 204, 224), 1)
            .hontoAt(6.0f, 14.0f)
            .hontoLayer(22);
        stage.hontoText("hub_progress", "CLEAR EACH LESSON TO OPEN THE NEXT ONE", honto::hontoRGBA(255, 236, 162), 1)
            .hontoAt(6.0f, 24.0f)
            .hontoLayer(22);

        stage.hontoFill("hub_preview_back", kRenderWidth, 32.0f, honto::hontoRGBA(10, 14, 24, 210))
            .hontoAt(0.0f, 148.0f)
            .hontoLayer(20);
        stage.hontoOutline("hub_preview_border", kRenderWidth, 32.0f, honto::hontoRGBA(74, 90, 116), 1)
            .hontoAt(0.0f, 148.0f)
            .hontoLayer(21);

        auto previewTitle = stage.hontoText("hub_preview_title", "TOUCH A LESSON GATE", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(6.0f, 152.0f)
            .hontoLayer(22);
        auto previewSummary = stage.hontoText("hub_preview_summary", "Enter a gate to learn one core engine idea.", honto::hontoRGBA(190, 204, 224), 1)
            .hontoAt(6.0f, 162.0f)
            .hontoLayer(22);
        auto previewState = stage.hontoText("hub_preview_state", "STATE: LEVEL 1 IS READY", honto::hontoRGBA(255, 236, 162), 1)
            .hontoAt(180.0f, 152.0f)
            .hontoLayer(22);

        std::vector<honto::hontoActor> gateActors;
        std::vector<honto::hontoActor> gateTriggers;
        std::vector<honto::hontoActor> gateLabels;
        std::vector<honto::hontoActor> badgeLabels;
        gateActors.reserve(kLessonCount);
        gateTriggers.reserve(kLessonCount);
        gateLabels.reserve(kLessonCount);
        badgeLabels.reserve(kLessonCount);

        for (std::size_t i = 0; i < kLessonCount; ++i)
        {
            const float x = kHubDoorX[i];
            const honto::hontoColor accent = LessonColor(i);

            auto gate = stage.hontoChecker(
                "academy_gate_" + std::to_string(i),
                20.0f,
                32.0f,
                accent,
                honto::hontoRGBA(20, 26, 38),
                4)
                .hontoAt(x, 116.0f)
                .hontoLayer(4);
            gate.hontoAnimate()
                .hontoScaleTo(1.08f)
                .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
                .hontoIn(0.95f)
                .hontoPingPong()
                .hontoLoop()
                .hontoPlay();

            auto trigger = stage.hontoTrigger("academy_trigger_" + std::to_string(i), 24.0f, 36.0f)
                .hontoAt(x - 2.0f, 114.0f)
                .hontoLayer(4);

            auto label = stage.hontoText(
                    "academy_label_" + std::to_string(i),
                    "L" + std::to_string(i + 1),
                    honto::hontoRGBA(238, 245, 255),
                    1,
                    true)
                .hontoAt(x + 4.0f, 104.0f)
                .hontoLayer(6);

            auto badge = stage.hontoText(
                    "academy_badge_" + std::to_string(i),
                    "L" + std::to_string(i + 1) + " " + LessonStateText(i),
                    LessonStateColor(i),
                    1)
                .hontoAt(6.0f + (static_cast<float>(i % 4) * 78.0f), 42.0f + (static_cast<float>(i / 4) * 8.0f))
                .hontoLayer(22);

            gateActors.push_back(gate);
            gateTriggers.push_back(trigger);
            gateLabels.push_back(label);
            badgeLabels.push_back(badge);
        }

        auto finale = stage.hontoText("academy_finale", "MASTERED", honto::hontoRGBA(255, 242, 184), 1, true)
            .hontoAt(1046.0f, 96.0f)
            .hontoLayer(6)
            .hontoShow(false);

        stage.hontoImage("hub_badge", "sandbox/assets/honto_badge.png", 18.0f, 18.0f)
            .hontoAt(294.0f, 4.0f)
            .hontoLayer(22);

        stage.hontoEveryFrame(
            [player, gateTriggers, gateActors, gateLabels, badgeLabels, previewTitle, previewSummary, previewState, selectedLesson, finale](float)
            {
                int active = -1;
                for (std::size_t i = 0; i < gateTriggers.size(); ++i)
                {
                    if (player.hontoTouching(gateTriggers[i]))
                    {
                        active = static_cast<int>(i);
                        break;
                    }
                }

                *selectedLesson = active;

                for (std::size_t i = 0; i < gateActors.size(); ++i)
                {
                    const honto::hontoColor tint = gLessonCleared[i]
                        ? LessonColor(i)
                        : (LessonUnlocked(i) ? honto::hontoRGBA(220, 230, 246) : honto::hontoRGBA(70, 82, 104));
                    gateActors[i].hontoPaint(tint);
                    gateLabels[i].hontoPaint(i == static_cast<std::size_t>(std::max(active, 0)) && active >= 0
                        ? honto::hontoRGBA(255, 242, 184)
                        : LessonStateColor(i));
                    badgeLabels[i].hontoTextValue("L" + std::to_string(i + 1) + " " + LessonStateText(i));
                    badgeLabels[i].hontoPaint(LessonStateColor(i));
                }

                if (active < 0)
                {
                    if (AcademyCompleted())
                    {
                        previewTitle.hontoTextValue("ACADEMY COMPLETE");
                        previewSummary.hontoTextValue("Every lesson is clear. The sample now covers the whole path.");
                        previewState.hontoTextValue("STATE: MASTERED");
                    }
                    else
                    {
                        previewTitle.hontoTextValue("TOUCH A LESSON GATE");
                        previewSummary.hontoTextValue("Each gate teaches one engine feature with a playable example.");
                        previewState.hontoTextValue("STATE: LEVEL " + std::to_string(static_cast<int>(std::count(gLessonCleared.begin(), gLessonCleared.end(), true)) + 1) + " IS NEXT");
                    }
                }
                else
                {
                    const std::size_t index = static_cast<std::size_t>(active);
                    previewTitle.hontoTextValue("LEVEL " + std::to_string(index + 1) + "  " + kLessons[index].title);
                    previewSummary.hontoTextValue(kLessons[index].summary);
                    if (gLessonCleared[index])
                    {
                        previewState.hontoTextValue("STATE: CLEAR  ENTER TO REPLAY");
                    }
                    else if (LessonUnlocked(index))
                    {
                        previewState.hontoTextValue("STATE: OPEN  ENTER TO START");
                    }
                    else
                    {
                        previewState.hontoTextValue("STATE: LOCKED  CLEAR THE PREVIOUS LESSON");
                    }
                }

                finale.hontoShow(AcademyCompleted());
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage, selectedLesson]()
            {
                if (*selectedLesson < 0)
                {
                    PlayLocked(stage);
                    return;
                }

                const std::size_t lessonIndex = static_cast<std::size_t>(*selectedLesson);
                if (!LessonUnlocked(lessonIndex))
                {
                    PlayLocked(stage);
                    return;
                }

                PlaySuccess(stage);
                GoToLesson(stage, lessonIndex);
            }
        );
    }

    void BuildLessonStageScene(honto::hontoStage& stage)
    {
        auto collected = std::make_shared<int>(0);
        auto exitOpen = std::make_shared<bool>(false);

        DrawLessonPanel(stage, 0);
        stage.hontoBackground(18, 24, 34);

        stage.hontoFill("lesson1_floor", kRenderWidth - 16.0f, 16.0f, honto::hontoRGBA(28, 42, 64))
            .hontoAt(8.0f, 148.0f)
            .hontoLayer(0);
        stage.hontoOutline("lesson1_arena", kRenderWidth - 16.0f, 92.0f, honto::hontoRGBA(72, 94, 128), 1)
            .hontoAt(8.0f, 72.0f)
            .hontoLayer(1);

        auto player = stage.hontoBox("lesson1_player", 16.0f, 16.0f, LessonColor(0))
            .hontoAt(16.0f, 118.0f)
            .hontoLayer(4)
            .hontoMoveWithArrows(124.0f)
            .hontoKeepInside(10.0f, 74.0f, 294.0f, 132.0f);

        std::vector<honto::hontoActor> goals;
        goals.push_back(stage.hontoChecker("goal_a", 10.0f, 10.0f, honto::hontoRGBA(255, 228, 112), LessonColor(0), 2).hontoAt(86.0f, 88.0f).hontoLayer(3));
        goals.push_back(stage.hontoChecker("goal_b", 10.0f, 10.0f, honto::hontoRGBA(255, 228, 112), LessonColor(0), 2).hontoAt(156.0f, 122.0f).hontoLayer(3));
        goals.push_back(stage.hontoChecker("goal_c", 10.0f, 10.0f, honto::hontoRGBA(255, 228, 112), LessonColor(0), 2).hontoAt(240.0f, 92.0f).hontoLayer(3));
        for (const auto& goal : goals)
        {
            goal.hontoAnimate()
                .hontoScaleTo(1.15f)
                .hontoPaintTo(honto::hontoRGBA(255, 248, 184))
                .hontoIn(0.8f)
                .hontoPingPong()
                .hontoLoop()
                .hontoPlay();
        }

        auto status = stage.hontoText("lesson1_status", "OBJECTIVE: COLLECT 3 GOALS", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);
        auto meter = stage.hontoBar(
            "lesson1_bar",
            120.0f,
            8.0f,
            0.0f,
            LessonColor(0),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255))
            .hontoAt(192.0f, 58.0f)
            .hontoLayer(10);

        auto exitPortal = stage.hontoChecker("lesson1_exit", 18.0f, 28.0f, LessonColor(0), honto::hontoRGBA(255, 242, 184), 3)
            .hontoAt(-80.0f, -80.0f)
            .hontoLayer(4)
            .hontoShow(false);
        exitPortal.hontoAnimate()
            .hontoScaleTo(1.08f)
            .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
            .hontoIn(0.9f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoEveryFrame(
            [player, goals, collected, exitOpen, exitPortal, status, meter, stage](float)
            {
                for (const auto& goal : goals)
                {
                    if (goal.Position().x > -50.0f && player.hontoTouching(goal))
                    {
                        goal.hontoAt(-120.0f, -120.0f).hontoShow(false);
                        *collected += 1;
                        meter.hontoBarValue(static_cast<float>(*collected) / 3.0f);
                        stage.hontoPlayTone(720 + (*collected * 80), 60);
                    }
                }

                if (*collected >= 3 && !*exitOpen)
                {
                    *exitOpen = true;
                    exitPortal.hontoAt(284.0f, 108.0f).hontoShow(true);
                    status.hontoTextValue("OBJECTIVE: TOUCH THE EXIT PORTAL");
                    PlaySuccess(stage);
                }

                if (*exitOpen && player.hontoTouching(exitPortal))
                {
                    gLessonCleared[0] = true;
                    ReturnToHub(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonPhysicsScene(honto::hontoStage& stage)
    {
        auto map = MakeMap(7, 20);
        FillSpan(map, 6, 0, 19, '#');
        FillSpan(map, 4, 3, 5, '#');
        FillSpan(map, 3, 9, 11, '#');
        FillSpan(map, 2, 15, 17, '#');

        DrawLessonPanel(stage, 1);
        stage.hontoBackground(20, 22, 34);
        stage.hontoGravity(0.0f, 760.0f);

        auto world = stage.hontoTileMap("lesson2_world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, kLessonWorldTop);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(142, 110, 82), true, true);

        auto player = stage.hontoBox("lesson2_player", 16.0f, 16.0f, LessonColor(1))
            .hontoAt(16.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);

        auto exitBeacon = stage.hontoChecker("lesson2_exit", 16.0f, 24.0f, LessonColor(1), honto::hontoRGBA(255, 248, 184), 2)
            .hontoAt(276.0f, 98.0f)
            .hontoLayer(3);
        exitBeacon.hontoAnimate()
            .hontoScaleTo(1.12f)
            .hontoIn(0.8f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto status = stage.hontoText("lesson2_status", "OBJECTIVE: JUMP ACROSS THE LEDGES", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);

        stage.hontoEveryFrame(
            [player, exitBeacon, status, stage](float)
            {
                if (player.Position().y > 180.0f)
                {
                    ResetActor(player, { 16.0f, 132.0f });
                    status.hontoTextValue("OBJECTIVE: TRY THE JUMP CHAIN AGAIN");
                    PlayLocked(stage);
                }

                if (player.hontoTouching(exitBeacon))
                {
                    gLessonCleared[1] = true;
                    PlaySuccess(stage);
                    ReturnToHub(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonAnimationScene(honto::hontoStage& stage)
    {
        auto activated = std::make_shared<int>(0);
        auto exitOpen = std::make_shared<bool>(false);
        auto heroSheet = MakeHeroSheet(
            honto::hontoRGBA(122, 210, 255),
            honto::hontoRGBA(154, 226, 255),
            honto::hontoRGBA(88, 188, 242),
            honto::hontoRGBA(182, 238, 255));

        auto map = MakeMap(7, 20);
        FillSpan(map, 6, 0, 19, '#');
        FillSpan(map, 4, 3, 5, '#');
        FillSpan(map, 3, 9, 11, '#');
        FillSpan(map, 4, 14, 16, '#');

        DrawLessonPanel(stage, 2);
        stage.hontoBackground(16, 20, 30);
        stage.hontoGravity(0.0f, 760.0f);

        auto world = stage.hontoTileMap("lesson3_world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, kLessonWorldTop);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(78, 108, 146), true, true);

        auto player = stage.hontoImage("lesson3_player", heroSheet, 16.0f, 16.0f)
            .hontoAt(16.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);
        ApplyWalkAnimation(player, heroSheet, 10.0f);

        std::vector<honto::hontoActor> crystals;
        crystals.push_back(stage.hontoChecker("crystal_a", 12.0f, 12.0f, LessonColor(2), honto::hontoRGBA(255, 255, 255), 3).hontoAt(54.0f, 116.0f).hontoLayer(4));
        crystals.push_back(stage.hontoChecker("crystal_b", 12.0f, 12.0f, LessonColor(2), honto::hontoRGBA(255, 255, 255), 3).hontoAt(150.0f, 100.0f).hontoLayer(4));
        crystals.push_back(stage.hontoChecker("crystal_c", 12.0f, 12.0f, LessonColor(2), honto::hontoRGBA(255, 255, 255), 3).hontoAt(232.0f, 116.0f).hontoLayer(4));
        for (const auto& crystal : crystals)
        {
            crystal.hontoAnimate()
                .hontoScaleTo(1.2f)
                .hontoPaintTo(honto::hontoRGBA(232, 246, 255))
                .hontoIn(0.75f)
                .hontoPingPong()
                .hontoLoop()
                .hontoPlay();
        }

        auto burst = stage.hontoParticles("lesson3_burst", 16.0f, 16.0f);
        burst.hontoAt(0.0f, 0.0f).hontoLayer(5);
        burst.hontoSizeRange(2.0f, 4.0f);
        burst.hontoLifetimeRange(0.25f, 0.6f);
        burst.hontoVelocityRange({ -28.0f, -26.0f }, { 28.0f, -92.0f });
        burst.hontoColorRange(LessonColor(2), honto::hontoRGBA(255, 255, 255, 0));

        auto status = stage.hontoText("lesson3_status", "OBJECTIVE: ACTIVATE 3 ANIMATED CRYSTALS", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);
        auto meter = stage.hontoBar(
            "lesson3_bar",
            120.0f,
            8.0f,
            0.0f,
            LessonColor(2),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255))
            .hontoAt(192.0f, 58.0f)
            .hontoLayer(10);

        auto exitPortal = stage.hontoChecker("lesson3_exit", 18.0f, 28.0f, LessonColor(2), honto::hontoRGBA(255, 242, 184), 3)
            .hontoAt(-80.0f, -80.0f)
            .hontoLayer(4)
            .hontoShow(false);
        exitPortal.hontoAnimate()
            .hontoScaleTo(1.08f)
            .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
            .hontoIn(0.85f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoEveryFrame(
            [player, crystals, activated, exitOpen, exitPortal, burst, status, meter, stage](float)
            {
                if (player.Position().y > 180.0f)
                {
                    ResetActor(player, { 16.0f, 132.0f });
                    status.hontoTextValue("OBJECTIVE: COLLECT THE CRYSTALS AGAIN");
                }

                for (const auto& crystal : crystals)
                {
                    if (crystal.Position().x > -50.0f && player.hontoTouching(crystal))
                    {
                        burst.hontoAt(crystal.Position());
                        burst.hontoBurst(18);
                        crystal.hontoAt(-120.0f, -120.0f).hontoShow(false);
                        *activated += 1;
                        meter.hontoBarValue(static_cast<float>(*activated) / 3.0f);
                        stage.hontoPlayTone(760 + (*activated * 60), 70);
                    }
                }

                if (*activated >= 3 && !*exitOpen)
                {
                    *exitOpen = true;
                    exitPortal.hontoAt(284.0f, 108.0f).hontoShow(true);
                    status.hontoTextValue("OBJECTIVE: TAKE THE FX PORTAL");
                    PlaySuccess(stage);
                }

                if (*exitOpen && player.hontoTouching(exitPortal))
                {
                    gLessonCleared[2] = true;
                    ReturnToHub(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonCameraScene(honto::hontoStage& stage)
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

        DrawLessonPanel(stage, 3);
        stage.hontoBackground(18, 22, 32);
        stage.hontoGravity(0.0f, 760.0f);

        stage.hontoFill("lesson4_back", 640.0f, 112.0f, honto::hontoRGBA(34, 52, 74))
            .hontoAt(0.0f, kLessonWorldTop)
            .hontoLayer(0);

        auto world = stage.hontoTileMap("lesson4_world", map, 16.0f, 16.0f);
        world.hontoAt(0.0f, kLessonWorldTop);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(102, 120, 96), true, true);

        auto player = stage.hontoImage("lesson4_player", heroSheet, 16.0f, 16.0f)
            .hontoAt(16.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 290.0f);
        ApplyWalkAnimation(player, heroSheet, 9.0f);
        stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);

        auto patroller = stage.hontoBox("lesson4_patroller", 16.0f, 16.0f, honto::hontoRGBA(255, 148, 96))
            .hontoAt(172.0f, 132.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoPatrolX(172.0f, 244.0f, 44.0f);

        auto hunter = stage.hontoBox("lesson4_hunter", 16.0f, 16.0f, honto::hontoRGBA(214, 86, 102))
            .hontoAt(364.0f, 116.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoChaseX(player, 56.0f, 18.0f);

        auto trigger = stage.hontoTrigger("lesson4_trigger", 20.0f, 32.0f, true, LessonColor(3))
            .hontoAt(556.0f, 100.0f)
            .hontoLayer(4);
        trigger.hontoAnimate()
            .hontoScaleTo(1.1f)
            .hontoPaintTo(honto::hontoRGBA(255, 228, 188))
            .hontoIn(0.8f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto status = stage.hontoText("lesson4_status", "OBJECTIVE: REACH THE SWITCH WITHOUT GETTING HIT", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);
        auto switchFx = stage.hontoParticles("lesson4_fx", 20.0f, 28.0f);
        switchFx.hontoAt(556.0f, 100.0f).hontoLayer(5);
        switchFx.hontoEmissionRate(12.0f);
        switchFx.hontoSpawnArea(20.0f, 28.0f);
        switchFx.hontoVelocityRange({ -10.0f, -24.0f }, { 10.0f, -58.0f });
        switchFx.hontoLifetimeRange(0.3f, 0.8f);
        switchFx.hontoSizeRange(2.0f, 4.0f);
        switchFx.hontoColorRange(LessonColor(3), honto::hontoRGBA(255, 255, 255, 0));

        stage.hontoEveryFrame(
            [player, patroller, hunter, trigger, status, switchFx, stage](float)
            {
                if (player.Position().y > 180.0f)
                {
                    ResetActor(player, { 16.0f, 132.0f });
                    status.hontoTextValue("OBJECTIVE: THE CAMERA FOLLOWS YOU BACK TO START");
                }

                if (player.hontoTouching(patroller) || player.hontoTouching(hunter))
                {
                    ResetActor(player, { 16.0f, 132.0f });
                    status.hontoTextValue("OBJECTIVE: ENEMY HIT  TRY AGAIN");
                    stage.hontoCameraShake(2.4f, 0.2f, 24.0f);
                    PlayLocked(stage);
                }

                if (player.hontoTouching(trigger))
                {
                    gLessonCleared[3] = true;
                    switchFx.hontoBurst(22);
                    stage.hontoCameraShake(3.0f, 0.28f, 22.0f);
                    PlaySuccess(stage);
                    ReturnToHub(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonUiScene(honto::hontoStage& stage)
    {
        auto musicOn = std::make_shared<bool>(false);
        auto effectOn = std::make_shared<bool>(false);
        auto uiOn = std::make_shared<bool>(false);
        auto unlocked = std::make_shared<bool>(false);

        DrawLessonPanel(stage, 4);
        stage.hontoBackground(18, 20, 30);
        stage.hontoSetMasterVolume(0.85f);
        stage.hontoSetBusVolume("music", 0.72f);
        stage.hontoSetBusVolume("effect", 0.92f);

        stage.hontoFill("lesson5_console", 288.0f, 92.0f, honto::hontoRGBA(20, 28, 42))
            .hontoAt(16.0f, 76.0f)
            .hontoLayer(1);
        stage.hontoOutline("lesson5_console_border", 288.0f, 92.0f, honto::hontoRGBA(86, 102, 132), 1)
            .hontoAt(16.0f, 76.0f)
            .hontoLayer(2);

        auto status = stage.hontoText("lesson5_status", "OBJECTIVE: POWER THREE CONSOLE SYSTEMS", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);
        auto mouseText = stage.hontoText("lesson5_mouse", "MOUSE: --,--", honto::hontoRGBA(182, 198, 224), 1)
            .hontoAt(196.0f, 58.0f)
            .hontoLayer(10);

        auto musicButton = stage.hontoButton("lesson5_music", "PLAY MUSIC", 84.0f, 16.0f)
            .hontoAt(28.0f, 90.0f)
            .hontoLayer(3);
        auto effectButton = stage.hontoButton("lesson5_fx", "FX PULSE", 84.0f, 16.0f)
            .hontoAt(118.0f, 90.0f)
            .hontoLayer(3);
        auto uiButton = stage.hontoButton("lesson5_ui", "UI BOOST", 84.0f, 16.0f)
            .hontoAt(208.0f, 90.0f)
            .hontoLayer(3);
        auto returnButton = stage.hontoButton("lesson5_return", "RETURN HUB", 96.0f, 16.0f)
            .hontoAt(-160.0f, -60.0f)
            .hontoLayer(3)
            .hontoShow(false);

        auto musicBar = stage.hontoBar("lesson5_music_bar", 96.0f, 8.0f, 0.0f, LessonColor(4), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(40.0f, 122.0f)
            .hontoLayer(3);
        auto effectBar = stage.hontoBar("lesson5_effect_bar", 96.0f, 8.0f, 0.0f, honto::hontoRGBA(255, 182, 96), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(40.0f, 140.0f)
            .hontoLayer(3);
        auto uiBar = stage.hontoBar("lesson5_ui_bar", 96.0f, 8.0f, 0.0f, honto::hontoRGBA(132, 255, 226), honto::hontoRGBA(12, 16, 24, 220), honto::hontoRGBA(236, 245, 255))
            .hontoAt(184.0f, 122.0f)
            .hontoLayer(3);

        stage.hontoText("lesson5_music_label", "MUSIC", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(40.0f, 112.0f)
            .hontoLayer(3);
        stage.hontoText("lesson5_fx_label", "EFFECT", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(40.0f, 130.0f)
            .hontoLayer(3);
        stage.hontoText("lesson5_ui_label", "UI", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(184.0f, 112.0f)
            .hontoLayer(3);

        auto core = stage.hontoChecker("lesson5_core", 24.0f, 24.0f, LessonColor(4), honto::hontoRGBA(255, 255, 255), 4)
            .hontoAt(232.0f, 132.0f)
            .hontoLayer(3);
        core.hontoAnimate()
            .hontoScaleTo(1.18f)
            .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
            .hontoIn(0.8f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto fx = stage.hontoParticles("lesson5_fx_particles", 20.0f, 20.0f);
        fx.hontoAt(150.0f, 132.0f).hontoLayer(4);
        fx.hontoSizeRange(2.0f, 4.0f);
        fx.hontoLifetimeRange(0.2f, 0.55f);
        fx.hontoVelocityRange({ -32.0f, -22.0f }, { 32.0f, -86.0f });
        fx.hontoColorRange(honto::hontoRGBA(255, 220, 172, 255), honto::hontoRGBA(255, 128, 82, 0));

        stage.hontoWhenClicked(
            musicButton,
            [stage, musicOn, musicBar, status]()
            {
                *musicOn = true;
                musicBar.hontoBarValue(1.0f);
                stage.hontoPlayMusic("sandbox/assets/honto_theme.wav");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue("OBJECTIVE: MUSIC BUS ONLINE");
            }
        );

        stage.hontoWhenClicked(
            effectButton,
            [stage, effectOn, effectBar, status, fx]()
            {
                *effectOn = true;
                effectBar.hontoBarValue(1.0f);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                stage.hontoPlayTone(860, 70);
                fx.hontoBurst(24);
                status.hontoTextValue("OBJECTIVE: EFFECT BUS ONLINE");
            }
        );

        stage.hontoWhenClicked(
            uiButton,
            [stage, uiOn, uiBar, status, core]()
            {
                *uiOn = true;
                uiBar.hontoBarValue(1.0f);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                core.hontoPaint(honto::hontoRGBA(132, 255, 226));
                status.hontoTextValue("OBJECTIVE: UI SYSTEM ONLINE");
            }
        );

        stage.hontoWhenClicked(
            returnButton,
            [stage]()
            {
                gLessonCleared[4] = true;
                PlaySuccess(stage);
                ReturnToHub(stage);
            }
        );

        stage.hontoEveryFrame(
            [stage, musicOn, effectOn, uiOn, unlocked, returnButton, status, mouseText](float)
            {
                if (stage.hontoHasMouse())
                {
                    const honto::hontoVec2 mouse = stage.hontoMousePosition();
                    mouseText.hontoTextValue("MOUSE: " + std::to_string(static_cast<int>(mouse.x)) + "," + std::to_string(static_cast<int>(mouse.y)));
                }
                else
                {
                    mouseText.hontoTextValue("MOUSE: --,--");
                }

                if (!*unlocked && *musicOn && *effectOn && *uiOn)
                {
                    *unlocked = true;
                    returnButton.hontoAt(112.0f, 148.0f).hontoShow(true);
                    status.hontoTextValue("OBJECTIVE: CLICK RETURN HUB TO FINISH");
                    PlaySuccess(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonLevelScene(honto::hontoStage& stage)
    {
        auto level = std::make_shared<honto::hontoLevel>(MakeBridgeLessonLevel());
        auto bridgeReady = std::make_shared<bool>(false);

        DrawLessonPanel(stage, 5);
        stage.hontoBackground(18, 24, 34);
        stage.hontoGravity(0.0f, 760.0f);

        stage.hontoFill("lesson6_editor_back", 80.0f, 26.0f, honto::hontoRGBA(22, 30, 46))
            .hontoAt(58.0f, 74.0f)
            .hontoLayer(1);
        for (int i = 0; i < 3; ++i)
        {
            stage.hontoOutline("lesson6_editor_cell_" + std::to_string(i), 18.0f, 18.0f, honto::hontoRGBA(86, 104, 132), 1)
                .hontoAt(66.0f + (static_cast<float>(i) * 18.0f), 78.0f)
                .hontoLayer(2);
        }

        auto editor = stage.hontoTileMap("lesson6_editor", MakeBridgeEditorMap(*level), 18.0f, 18.0f);
        editor.hontoAt(66.0f, 78.0f);
        editor.hontoLayer(3);
        editor.hontoTile('#', LessonColor(5), true, true);

        auto world = stage.hontoTileMap("lesson6_world", level->map, 16.0f, 16.0f);
        world.hontoAt(0.0f, 104.0f);
        world.hontoLayer(1);
        world.hontoTile('#', honto::hontoRGBA(96, 124, 112), true, true);

        auto player = stage.hontoBox("lesson6_player", 16.0f, 16.0f, LessonColor(5))
            .hontoAt(18.0f, 136.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(126.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 285.0f);

        auto saveButton = stage.hontoButton("lesson6_save", "SAVE JSON", 92.0f, 16.0f)
            .hontoAt(184.0f, 78.0f)
            .hontoLayer(3);
        auto loadButton = stage.hontoButton("lesson6_load", "LOAD JSON", 92.0f, 16.0f)
            .hontoAt(184.0f, 98.0f)
            .hontoLayer(3);
        auto resetButton = stage.hontoButton("lesson6_reset", "RESET", 92.0f, 16.0f)
            .hontoAt(184.0f, 118.0f)
            .hontoLayer(3);

        auto exitGate = stage.hontoChecker("lesson6_exit", 18.0f, 26.0f, LessonColor(5), honto::hontoRGBA(255, 242, 184), 3)
            .hontoAt(286.0f, 126.0f)
            .hontoLayer(4);
        exitGate.hontoAnimate()
            .hontoScaleTo(1.08f)
            .hontoPaintTo(honto::hontoRGBA(255, 255, 255))
            .hontoIn(0.85f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto status = stage.hontoText("lesson6_status", "OBJECTIVE: CLICK 3 EDITOR CELLS TO BUILD THE BRIDGE", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(10);
        auto buildFx = stage.hontoParticles("lesson6_fx", 18.0f, 18.0f);
        buildFx.hontoAt(0.0f, 0.0f).hontoLayer(5);
        buildFx.hontoSizeRange(2.0f, 4.0f);
        buildFx.hontoLifetimeRange(0.2f, 0.55f);
        buildFx.hontoVelocityRange({ -26.0f, -24.0f }, { 26.0f, -82.0f });
        buildFx.hontoColorRange(LessonColor(5), honto::hontoRGBA(255, 255, 255, 0));

        stage.hontoWhenClicked(
            saveButton,
            [stage, level, status]()
            {
                if (honto::hontoSaveLevel("sandbox/levels/academy_bridge.json", *level))
                {
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    status.hontoTextValue("OBJECTIVE: SAVED academy_bridge.json");
                }
                else
                {
                    PlayLocked(stage);
                    status.hontoTextValue("OBJECTIVE: SAVE FAILED");
                }
            }
        );

        stage.hontoWhenClicked(
            loadButton,
            [stage, level, world, editor, status]()
            {
                honto::hontoLevel loaded = honto::hontoLoadLevel("sandbox/levels/academy_bridge.json");
                if (loaded.IsValid())
                {
                    *level = loaded;
                    ApplyBridgeEditorState(*level, world, editor);
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    status.hontoTextValue("OBJECTIVE: LOADED academy_bridge.json");
                }
                else
                {
                    PlayLocked(stage);
                    status.hontoTextValue("OBJECTIVE: LOAD FAILED");
                }
            }
        );

        stage.hontoWhenClicked(
            resetButton,
            [stage, level, world, editor, status]()
            {
                *level = MakeBridgeLessonLevel();
                ApplyBridgeEditorState(*level, world, editor);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                status.hontoTextValue("OBJECTIVE: BRIDGE RESET");
            }
        );

        stage.hontoEveryFrame(
            [stage, editor, world, level, buildFx, player, exitGate, status, bridgeReady](float)
            {
                int column = 0;
                int row = 0;
                if (stage.hontoMousePressed(honto::hontoMouse::Left) && editor.hontoWorldToCell(stage.hontoMousePosition(), column, row))
                {
                    if (column >= 0 && column < 3)
                    {
                        editor.hontoCell(column, 0, '#');
                        world.hontoCell(8 + column, 4, '#');
                        level->map[4][8 + column] = '#';
                        buildFx.hontoAt(66.0f + (static_cast<float>(column) * 18.0f), 78.0f);
                        buildFx.hontoBurst(14);
                        stage.hontoPlayTone(720 + (column * 60), 60);
                    }
                }

                const bool ready = level->map[4][8] == '#' && level->map[4][9] == '#' && level->map[4][10] == '#';
                *bridgeReady = ready;

                if (ready)
                {
                    status.hontoTextValue("OBJECTIVE: CROSS THE BRIDGE TO THE EXIT");
                }

                if (player.Position().y > 180.0f)
                {
                    ResetActor(player, { 18.0f, 136.0f });
                    if (!ready)
                    {
                        status.hontoTextValue("OBJECTIVE: YOU NEED THE BRIDGE FIRST");
                    }
                }

                if (player.hontoTouching(exitGate))
                {
                    if (!ready)
                    {
                        PlayLocked(stage);
                        status.hontoTextValue("OBJECTIVE: BUILD THE BRIDGE BEFORE EXITING");
                    }
                    else
                    {
                        gLessonCleared[5] = true;
                        PlaySuccess(stage);
                        ReturnToHub(stage);
                    }
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }

    void BuildLessonDoomScene(honto::hontoStage& stage)
    {
        auto wallA = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(152, 92, 76),
            honto::hontoRGBA(118, 60, 46),
            8);
        auto wallB = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(82, 126, 164),
            honto::hontoRGBA(58, 88, 124),
            6);
        auto doorTexture = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(196, 156, 94),
            honto::hontoRGBA(128, 82, 42),
            4);
        auto impTexture = honto::hontoFrameSheetTexture(32, 48, { honto::hontoRGBA(222, 96, 72) }, 1);
        auto weaponTexture = honto::hontoCheckerTexture(
            96,
            64,
            honto::hontoRGBA(64, 64, 72),
            honto::hontoRGBA(132, 132, 148),
            6);

        DrawLessonPanel(stage, 6);
        stage.hontoBackground(10, 12, 18);
        stage.hontoFill("lesson7_status_back", kRenderWidth, 10.0f, honto::hontoRGBA(10, 14, 24, 210))
            .hontoAt(0.0f, kLessonStatusY)
            .hontoLayer(10);
        auto status = stage.hontoText("lesson7_status", "OBJECTIVE: OPEN THE DOOR WITH E AND REACH THE EXIT", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(8.0f, kLessonStatusY)
            .hontoLayer(11);

        auto doom = stage.hontoRaycast("lesson7_doom", kRenderWidth, 112.0f);
        doom.hontoAt(0.0f, kLessonWorldTop);
        doom.hontoLayer(1);
        doom.hontoMap(kDoomLessonMap)
            .hontoPlayer(1.5f, 1.5f, 0.0f)
            .hontoViewDegrees(68.0f)
            .hontoMoveSpeed(2.8f)
            .hontoTurnSpeed(2.2f)
            .hontoRunMultiplier(1.8f)
            .hontoFloor(honto::hontoRGBA(38, 36, 44))
            .hontoCeiling(honto::hontoRGBA(18, 22, 34))
            .hontoFog(honto::hontoRGBA(14, 18, 28), 0.42f)
            .hontoWall('#', honto::hontoRGBA(174, 112, 92))
            .hontoWallTexture('#', wallA)
            .hontoWall('D', honto::hontoRGBA(206, 168, 102))
            .hontoWallTexture('D', wallB)
            .hontoDoor('D', honto::hontoRGBA(206, 168, 102), 0.65f, 2.0f)
            .hontoDoorTexture('D', doorTexture)
            .hontoThingTexture("imp_1", 7.4f, 2.0f, 0.9f, 1.3f, impTexture, honto::hontoRGBA(255, 190, 170), 0.08f, 4.6f)
            .hontoThingTexture("imp_2", 6.6f, 6.6f, 0.9f, 1.3f, impTexture, honto::hontoRGBA(255, 170, 152), 0.1f, 5.2f)
            .hontoWeapon(weaponTexture, 136.0f, 80.0f, honto::hontoRGBA(255, 255, 255))
            .hontoWeaponBob(3.8f, 10.0f)
            .hontoMiniMap(true, 7.0f)
            .hontoDoomControls();

        stage.hontoEveryFrame(
            [doom, status, stage](float)
            {
                const honto::hontoVec2 player = doom.hontoPlayerPosition();
                if (player.x > 7.3f && player.y > 6.8f)
                {
                    gLessonCleared[6] = true;
                    status.hontoTextValue("OBJECTIVE: EXIT FOUND  RETURNING TO HUB");
                    PlaySuccess(stage);
                    ReturnToHub(stage);
                }
            }
        );

        stage.hontoWhenPressed(honto::hontoKey::Escape, [stage]() { ReturnToHub(stage); });
    }
}

int main()
{
    return honto::hontoGame("honto Engine Academy")
        .hontoWindow(1280, 720)
        .hontoRender(static_cast<int>(kRenderWidth), static_cast<int>(kRenderHeight))
        .hontoClear(honto::hontoRGBA(8, 10, 16))
        .hontoPlay(BuildAcademyHubScene)
        .hontoRun();
}
