#include "honto/HonTo.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
    float Clamp01(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    std::string MakeMouseText(const honto::hontoStage& stage)
    {
        if (!stage.hontoHasMouse())
        {
            return "MOUSE: --, --";
        }

        const honto::hontoVec2 mouse = stage.hontoMousePosition();
        return "MOUSE: " + std::to_string(static_cast<int>(mouse.x)) + ", " + std::to_string(static_cast<int>(mouse.y));
    }

    const std::vector<std::string> kFallbackPlatformMap {
        "........................................",
        "........................................",
        ".......................C................",
        "........................................",
        "......B.....................CCC.........",
        "............####........................",
        "....................#####...............",
        "...####.........................##......",
        "...............B........................",
        ".........#####.............####.........",
        "........................................",
        "########################################"
    };

    const std::vector<std::string> kDungeonMap {
        "##########",
        "#...A..D.#",
        "#.#.#.##A#",
        "#.#...#D.#",
        "#.###.#A.#",
        "#.....#..#",
        "#A#####..#",
        "#..D.....#",
        "##########"
    };

    honto::hontoLevel LoadPlatformLevel()
    {
        honto::hontoLevel level = honto::hontoLoadLevel("sandbox/levels/platform.json");
        if (!level.IsValid())
        {
            level = honto::hontoLoadLevel("sandbox/levels/platform.honto");
        }
        if (level.IsValid())
        {
            return level;
        }

        level.title = "PORTAL PLAINS";
        level.tileSize = { 16.0f, 16.0f };
        level.map = kFallbackPlatformMap;

        honto::hontoLevelEntity playerSpawn;
        playerSpawn.name = "player_spawn";
        playerSpawn.kind = "spawn";
        playerSpawn.position = { 36.0f, 40.0f };
        playerSpawn.size = { 16.0f, 16.0f };
        playerSpawn.layer = 4;
        level.entities.push_back(playerSpawn);

        honto::hontoLevelEntity portalSpawn;
        portalSpawn.name = "portal_spawn";
        portalSpawn.kind = "portal";
        portalSpawn.position = { 594.0f, 140.0f };
        portalSpawn.size = { 20.0f, 36.0f };
        portalSpawn.layer = 5;
        portalSpawn.color = honto::hontoRGBA(255, 236, 162);
        level.entities.push_back(portalSpawn);

        return level;
    }

    void BuildPlatformScene(honto::hontoStage& stage);
    void BuildDoomScene(honto::hontoStage& stage);
    void BuildMultiverseScene(honto::hontoStage& stage);
    void BuildToolsWindow(honto::hontoStage& stage);

    void BuildPlatformScene(honto::hontoStage& stage)
    {
        auto level = std::make_shared<honto::hontoLevel>(LoadPlatformLevel());
        auto portalUnlocked = std::make_shared<bool>(false);
        auto jumpGate = std::make_shared<bool>(false);

        const honto::hontoLevelEntity* playerSpawn = honto::hontoFindLevelEntity(*level, "player_spawn");
        const honto::hontoLevelEntity* portalSpawn = honto::hontoFindLevelEntity(*level, "portal_spawn");
        const honto::hontoLevelEntity* titleEntity = honto::hontoFindLevelEntity(*level, "hud_title");
        const honto::hontoLevelEntity* hintEntity = honto::hontoFindLevelEntity(*level, "hud_hint");

        const honto::hontoVec2 playerStart = playerSpawn != nullptr ? playerSpawn->position : honto::hontoVec2 { 36.0f, 40.0f };
        const honto::hontoVec2 portalStart = portalSpawn != nullptr ? portalSpawn->position : honto::hontoVec2 { 594.0f, 140.0f };
        const std::string hudTitle = titleEntity != nullptr && !titleEntity->text.empty() ? titleEntity->text : level->title;
        const std::string hudHint = hintEntity != nullptr && !hintEntity->text.empty() ? hintEntity->text : "A/D MOVE  SPACE JUMP  ENTER DOOM  Q MULTIVERSE";

        auto tileSheet = honto::hontoFrameSheetTexture(
            16,
            16,
            {
                honto::hontoRGBA(82, 126, 86),
                honto::hontoRGBA(142, 102, 76),
                honto::hontoRGBA(74, 110, 164),
                honto::hontoRGBA(176, 186, 94)
            },
            4
        );

        auto playerSheet = honto::hontoFrameSheetTexture(
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

        honto::hontoPrint("Platform scene: A/D move, Space jump, F5 save level, Enter opens DOOM, Q travels to the multiverse window.");

        stage.hontoBackground(18, 22, 34);
        stage.hontoGravity(0.0f, 760.0f);

        stage.hontoFill("sky", 960.0f, 88.0f, honto::hontoRGBA(28, 40, 72)).hontoAt(0.0f, 0.0f).hontoLayer(0);
        stage.hontoFill("haze", 960.0f, 104.0f, honto::hontoRGBA(44, 66, 112, 120)).hontoAt(0.0f, 52.0f).hontoLayer(0);

        auto world = stage.hontoTileMap("world", *level);
        world.hontoAt(0.0f, 0.0f);
        world.hontoLayer(1);
        world.hontoTileTextureRegion('#', tileSheet, 0, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        world.hontoTileTextureRegion('B', tileSheet, 16, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        world.hontoTileTextureRegion('C', tileSheet, 32, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);

        auto player = stage.hontoImage("player", playerSheet, 16.0f, 16.0f)
            .hontoAt(playerStart)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoCollideWithMap(world)
            .hontoMoveLeftRight(132.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 285.0f);

        player.hontoAnimateFrames()
            .hontoTexture(playerSheet)
            .hontoFrameSize(16, 16)
            .hontoFrames({ 0, 1, 2, 3, 2, 1 })
            .hontoFPS(10.0f)
            .hontoLoop()
            .hontoPlay();

        stage.hontoCameraFollow(player, 1.0f);

        auto portal = stage.hontoChecker(
            "portal",
            portalSpawn != nullptr ? portalSpawn->size.x : 20.0f,
            portalSpawn != nullptr ? portalSpawn->size.y : 36.0f,
            honto::hontoRGBA(250, 184, 80),
            honto::hontoRGBA(112, 88, 255),
            3
        );
        portal.hontoAt(portalStart);
        portal.hontoLayer(portalSpawn != nullptr ? portalSpawn->layer : 5);
        portal.hontoAnimate()
            .hontoScaleTo(1.08f)
            .hontoPaintTo(honto::hontoRGBA(255, 236, 162))
            .hontoIn(0.85f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoText("hud_title", hudTitle, honto::hontoRGBA(238, 245, 255), 2)
            .hontoAt(8.0f, 8.0f)
            .hontoLayer(11);
        stage.hontoText("hud_hint", hudHint, honto::hontoRGBA(194, 208, 228), 1)
            .hontoAt(8.0f, 24.0f)
            .hontoLayer(11);

        auto statusLabel = stage.hontoText("hud_status", "STATUS: FIND PORTAL", honto::hontoRGBA(255, 244, 174), 1)
            .hontoAt(8.0f, 38.0f)
            .hontoLayer(11);
        auto saveLabel = stage.hontoText("hud_save", "F5 SAVE LEVEL", honto::hontoRGBA(168, 186, 210), 1)
            .hontoAt(8.0f, 50.0f)
            .hontoLayer(11);
        auto progressBar = stage.hontoBar(
            "hud_progress",
            126.0f,
            10.0f,
            0.0f,
            honto::hontoRGBA(104, 228, 136),
            honto::hontoRGBA(12, 18, 30, 220),
            honto::hontoRGBA(236, 245, 255)
        )
            .hontoAt(8.0f, 62.0f)
            .hontoLayer(11);

        stage.hontoImage("hud_badge", "sandbox/assets/honto_badge.png", 18.0f, 18.0f)
            .hontoAt(294.0f, 8.0f)
            .hontoLayer(11);

        stage.hontoWhenPressed(
            honto::hontoKey::Space,
            [player, jumpGate, stage]()
            {
                if (player.hontoIsOnGround() && !*jumpGate)
                {
                    *jumpGate = true;
                    stage.hontoPlayTone(740, 70);
                }
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::F5,
            [stage, level]()
            {
                if (honto::hontoSaveLevel("sandbox/levels/platform_exported.honto", *level))
                {
                    stage.hontoPlayTone(880, 80);
                    stage.hontoPlayAlias("SystemAsterisk");
                    honto::hontoPrint("Saved level to sandbox/levels/platform_exported.honto");
                }
                else
                {
                    stage.hontoPlayTone(220, 120);
                    honto::hontoPrint("Failed to save level.");
                }
            }
        );

        stage.hontoEveryFrame(
            [player, portal, portalUnlocked, jumpGate, progressBar, statusLabel, portalStart, stage](float)
            {
                player.hontoPaint(player.hontoIsOnGround() ? honto::hontoRGBA(255, 255, 255) : honto::hontoRGBA(196, 220, 255));

                if (player.hontoIsOnGround())
                {
                    *jumpGate = false;
                }

                const float portalDistance = std::max(1.0f, portalStart.x);
                const float progress = std::clamp(player.Position().x / portalDistance, 0.0f, 1.0f);
                progressBar.hontoBarValue(progress);

                if (player.hontoTouching(portal) && !*portalUnlocked)
                {
                    *portalUnlocked = true;
                    statusLabel.hontoTextValue("STATUS: ENTER DOOM  Q MULTIVERSE");
                    stage.hontoPlayTone(980, 120);
                    stage.hontoPlayAlias("SystemAsterisk");
                    honto::hontoPrint("Portal unlocked. Press Enter for DOOM or Q for the multiverse window.");
                }
                else if (!*portalUnlocked)
                {
                    statusLabel.hontoTextValue("STATUS: FIND PORTAL");
                }
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage, portalUnlocked]()
            {
                if (!*portalUnlocked)
                {
                    stage.hontoPlayTone(220, 100);
                    stage.hontoPlayAlias("SystemHand");
                    honto::hontoPrint("Reach the portal first.");
                    return;
                }

                stage.hontoPlayTone(920, 110);
                stage.hontoPlayAlias("SystemExclamation");
                stage.hontoGoWithFade(BuildDoomScene, 0.7f);
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Q,
            [stage, portalUnlocked]()
            {
                if (!*portalUnlocked)
                {
                    stage.hontoPlayTone(220, 90);
                    return;
                }

                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                stage.hontoGoWindowWithFade("honto Multiverse Window", BuildMultiverseScene, 0.7f, honto::hontoRGBA(22, 32, 58), true);
            }
        );

        (void)saveLabel;
    }

    void BuildDoomScene(honto::hontoStage& stage)
    {
        honto::hontoPrint("DOOM scene: W/S move, A/D strafe, Shift run, E open doors, Tab map, Q multiverse, Enter back.");

        auto wallA = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(152, 92, 76),
            honto::hontoRGBA(118, 60, 46),
            8
        );
        auto wallB = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(76, 118, 164),
            honto::hontoRGBA(48, 82, 126),
            6
        );
        auto doorTexture = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(196, 156, 94),
            honto::hontoRGBA(128, 82, 42),
            4
        );
        auto impTexture = honto::hontoFrameSheetTexture(
            32,
            48,
            {
                honto::hontoRGBA(222, 96, 72)
            },
            1
        );
        auto barrelTexture = honto::hontoFrameSheetTexture(
            24,
            32,
            {
                honto::hontoRGBA(152, 132, 86)
            },
            1
        );
        auto weaponTexture = honto::hontoCheckerTexture(
            96,
            64,
            honto::hontoRGBA(64, 64, 72),
            honto::hontoRGBA(132, 132, 148),
            6
        );

        stage.hontoBackground(10, 12, 16);

        auto raycast = stage.hontoRaycast("dungeon", 320.0f, 180.0f);
        raycast.hontoAt(0.0f, 0.0f);
        raycast.hontoLayer(1);
        raycast.hontoMap(kDungeonMap)
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
            .hontoWall('A', honto::hontoRGBA(92, 140, 196))
            .hontoWallTexture('A', wallB)
            .hontoDoor('D', honto::hontoRGBA(206, 168, 102), 0.65f, 2.0f)
            .hontoDoorTexture('D', doorTexture)
            .hontoThingTexture("imp_1", 7.6f, 1.8f, 0.9f, 1.3f, impTexture, honto::hontoRGBA(255, 196, 180), 0.06f, 4.8f)
            .hontoThingTexture("imp_2", 7.3f, 6.6f, 0.9f, 1.3f, impTexture, honto::hontoRGBA(255, 180, 164), 0.08f, 5.4f)
            .hontoThingTexture("barrel", 3.4f, 7.1f, 0.8f, 1.0f, barrelTexture, honto::hontoRGBA(220, 210, 188))
            .hontoWeapon(weaponTexture, 140.0f, 84.0f, honto::hontoRGBA(255, 255, 255))
            .hontoWeaponBob(3.8f, 10.0f)
            .hontoMiniMap(true, 7.0f)
            .hontoDoomControls();

        stage.hontoFill("hud", 320.0f, 24.0f, honto::hontoRGBA(8, 10, 14, 180))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(2);
        stage.hontoOutline("hud_border", 320.0f, 24.0f, honto::hontoRGBA(78, 88, 114), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(3);
        stage.hontoText("doom_hint", "W/S MOVE  A/D STRAFE  SHIFT RUN  E DOOR", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(8.0f, 4.0f)
            .hontoLayer(4);
        stage.hontoText("doom_hint_2", "TAB MAP  Q MULTIVERSE  ENTER BACK", honto::hontoRGBA(190, 204, 224), 1)
            .hontoAt(8.0f, 12.0f)
            .hontoLayer(4);

        stage.hontoWhenPressed(
            honto::hontoKey::Q,
            [stage]()
            {
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                stage.hontoGoWindowWithFade("honto Multiverse Window", BuildMultiverseScene, 0.7f, honto::hontoRGBA(22, 32, 58), true);
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage]()
            {
                stage.hontoPlayTone(620, 100);
                stage.hontoPlayAlias("SystemAsterisk");
                stage.hontoGoWithFade(BuildPlatformScene, 0.7f);
            }
        );

        (void)raycast;
    }

    void BuildMultiverseScene(honto::hontoStage& stage)
    {
        auto borderless = std::make_shared<bool>(true);
        auto resizable = std::make_shared<bool>(false);
        auto topMost = std::make_shared<bool>(false);
        auto opacity = std::make_shared<float>(0.92f);
        auto sizeIndex = std::make_shared<int>(0);
        const std::vector<honto::hontoVec2> windowSizes {
            { 900.0f, 540.0f },
            { 1120.0f, 640.0f },
            { 760.0f, 460.0f }
        };

        honto::hontoPrint("Multiverse window: F1 borderless, F2 opacity, F3 resizable, F4 size, M topmost, Q main, Enter doom.");

        stage.hontoWindowBorderless(*borderless)
            .hontoWindowResizable(*resizable)
            .hontoWindowOpacity(*opacity)
            .hontoWindowTopMost(*topMost)
            .hontoWindowCenter();

        stage.hontoBackground(12, 18, 34);
        stage.hontoFill("nebula", 320.0f, 180.0f, honto::hontoRGBA(26, 38, 72, 180))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(0);
        stage.hontoOutline("frame", 318.0f, 178.0f, honto::hontoRGBA(116, 162, 255), 1)
            .hontoAt(1.0f, 1.0f)
            .hontoLayer(1);

        auto orbit = stage.hontoChecker(
            "orbit",
            34.0f,
            34.0f,
            honto::hontoRGBA(104, 148, 255),
            honto::hontoRGBA(44, 84, 188),
            4
        )
            .hontoAt(146.0f, 74.0f)
            .hontoLayer(2);
        orbit.hontoAnimate()
            .hontoScaleTo(1.22f)
            .hontoPaintTo(honto::hontoRGBA(190, 226, 255))
            .hontoIn(1.0f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto traveler = stage.hontoBox("traveler", 16.0f, 16.0f, honto::hontoRGBA(126, 255, 196))
            .hontoAt(28.0f, 126.0f)
            .hontoLayer(3)
            .hontoMoveWithArrows(84.0f)
            .hontoKeepInside(8.0f, 52.0f, 296.0f, 156.0f);

        auto status = stage.hontoText("verse_status", "MULTIVERSE READY", honto::hontoRGBA(255, 242, 184), 1)
            .hontoAt(10.0f, 10.0f)
            .hontoLayer(4);
        auto controlsA = stage.hontoText("verse_a", "F1 FRAME  F2 GLASS  F3 RESIZE  F4 SIZE", honto::hontoRGBA(232, 240, 255), 1)
            .hontoAt(10.0f, 24.0f)
            .hontoLayer(4);
        auto controlsB = stage.hontoText("verse_b", "M TOPMOST  Q MAIN WORLD  ENTER DOOM WORLD", honto::hontoRGBA(194, 210, 230), 1)
            .hontoAt(10.0f, 36.0f)
            .hontoLayer(4);
        auto mouseText = stage.hontoText("verse_mouse", MakeMouseText(stage), honto::hontoRGBA(166, 188, 216), 1)
            .hontoAt(10.0f, 164.0f)
            .hontoLayer(4);

        (void)controlsA;
        (void)controlsB;

        stage.hontoWhenPressed(
            honto::hontoKey::F1,
            [stage, borderless, status]()
            {
                *borderless = !*borderless;
                stage.hontoWindowBorderless(*borderless).hontoWindowCenter();
                status.hontoTextValue(*borderless ? "MULTIVERSE: BORDERLESS" : "MULTIVERSE: FRAMED");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::F2,
            [stage, opacity, status]()
            {
                *opacity = *opacity > 0.95f ? 0.82f : 1.0f;
                stage.hontoWindowOpacity(*opacity);
                status.hontoTextValue(*opacity < 1.0f ? "MULTIVERSE: GLASS MODE" : "MULTIVERSE: SOLID MODE");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::F3,
            [stage, resizable, status]()
            {
                *resizable = !*resizable;
                stage.hontoWindowResizable(*resizable);
                status.hontoTextValue(*resizable ? "MULTIVERSE: RESIZABLE" : "MULTIVERSE: FIXED");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::F4,
            [stage, sizeIndex, status, windowSizes]()
            {
                *sizeIndex = (*sizeIndex + 1) % static_cast<int>(windowSizes.size());
                const honto::hontoVec2 size = windowSizes[static_cast<std::size_t>(*sizeIndex)];
                stage.hontoWindowSize(static_cast<int>(size.x), static_cast<int>(size.y)).hontoWindowCenter();
                status.hontoTextValue("MULTIVERSE: SIZE " + std::to_string(static_cast<int>(size.x)) + "x" + std::to_string(static_cast<int>(size.y)));
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::M,
            [stage, topMost, status]()
            {
                *topMost = !*topMost;
                stage.hontoWindowTopMost(*topMost);
                status.hontoTextValue(*topMost ? "MULTIVERSE: TOPMOST" : "MULTIVERSE: NORMAL");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Q,
            [stage]()
            {
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                stage.hontoGoWindowWithFade("honto Engine Feature Sandbox", BuildPlatformScene, 0.7f, honto::hontoRGBA(18, 22, 34), true);
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage]()
            {
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                stage.hontoGoWindowWithFade("honto Engine Feature Sandbox", BuildDoomScene, 0.7f, honto::hontoRGBA(12, 14, 20), true);
            }
        );

        stage.hontoEveryFrame(
            [stage, mouseText, traveler, orbit, t = 0.0f](float deltaTime) mutable
            {
                t += deltaTime;
                orbit.hontoAt(146.0f + (std::sin(t * 1.4f) * 18.0f), 74.0f + (std::cos(t * 1.2f) * 10.0f));
                traveler.hontoPaint(stage.hontoHasMouse() ? honto::hontoRGBA(166, 255, 220) : honto::hontoRGBA(126, 255, 196));
                mouseText.hontoTextValue(MakeMouseText(stage));
            }
        );
    }

    void BuildToolsWindow(honto::hontoStage& stage)
    {
        auto level = std::make_shared<honto::hontoLevel>(LoadPlatformLevel());
        auto previewSheet = honto::hontoFrameSheetTexture(
            16,
            16,
            {
                honto::hontoRGBA(122, 240, 172),
                honto::hontoRGBA(88, 204, 148),
                honto::hontoRGBA(68, 172, 132),
                honto::hontoRGBA(140, 255, 196)
            },
            4
        );

        stage.hontoSetMasterVolume(0.85f);
        stage.hontoSetBusVolume("music", 0.72f);
        stage.hontoSetBusVolume("effect", 0.94f);

        stage.hontoBackground(20, 22, 30);
        stage.hontoOutline(308.0f, 168.0f, honto::hontoRGBA(74, 90, 114), 1).hontoAt(6.0f, 6.0f).hontoLayer(1);
        stage.hontoText("tool_title", "MOUSE  JSON  MIXER", honto::hontoRGBA(236, 245, 255), 2)
            .hontoAt(12.0f, 10.0f)
            .hontoLayer(3);
        auto levelLabel = stage.hontoText("tool_level", level->title, honto::hontoRGBA(166, 188, 216), 1)
            .hontoAt(12.0f, 30.0f)
            .hontoLayer(3);
        auto mouseLabel = stage.hontoText("tool_mouse", MakeMouseText(stage), honto::hontoRGBA(196, 210, 230), 1)
            .hontoAt(12.0f, 42.0f)
            .hontoLayer(3);
        auto statusLabel = stage.hontoText("tool_status", "READY", honto::hontoRGBA(255, 236, 162), 1)
            .hontoAt(12.0f, 54.0f)
            .hontoLayer(3);

        stage.hontoImage("tool_badge", "sandbox/assets/honto_badge.png", 24.0f, 24.0f)
            .hontoAt(280.0f, 12.0f)
            .hontoLayer(3);

        auto playMusicButton = stage.hontoButton("music_play", "PLAY MUSIC", 92.0f, 16.0f)
            .hontoAt(12.0f, 70.0f)
            .hontoLayer(3);
        auto stopMusicButton = stage.hontoButton("music_stop", "STOP MUSIC", 92.0f, 16.0f)
            .hontoAt(112.0f, 70.0f)
            .hontoLayer(3);
        auto fxButton = stage.hontoButton("effect_test", "FX TEST", 92.0f, 16.0f)
            .hontoAt(212.0f, 70.0f)
            .hontoLayer(3);
        auto masterDownButton = stage.hontoButton("master_down", "MASTER -", 92.0f, 16.0f)
            .hontoAt(12.0f, 92.0f)
            .hontoLayer(3);
        auto masterUpButton = stage.hontoButton("master_up", "MASTER +", 92.0f, 16.0f)
            .hontoAt(112.0f, 92.0f)
            .hontoLayer(3);
        auto saveJsonButton = stage.hontoButton("save_json", "SAVE JSON", 92.0f, 16.0f)
            .hontoAt(212.0f, 92.0f)
            .hontoLayer(3);
        auto loadJsonButton = stage.hontoButton("load_json", "LOAD JSON", 92.0f, 16.0f)
            .hontoAt(12.0f, 114.0f)
            .hontoLayer(3);
        auto loadTiledButton = stage.hontoButton("load_tiled", "LOAD TILED", 92.0f, 16.0f)
            .hontoAt(112.0f, 114.0f)
            .hontoLayer(3);
        auto saveTextButton = stage.hontoButton("save_text", "SAVE HONTO", 92.0f, 16.0f)
            .hontoAt(212.0f, 114.0f)
            .hontoLayer(3);

        stage.hontoText("master_label", "MASTER", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(12.0f, 140.0f)
            .hontoLayer(3);
        stage.hontoText("music_label", "MUSIC", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(12.0f, 152.0f)
            .hontoLayer(3);
        stage.hontoText("effect_label", "EFFECT", honto::hontoRGBA(236, 245, 255), 1)
            .hontoAt(12.0f, 164.0f)
            .hontoLayer(3);

        auto masterBar = stage.hontoBar(
            "master_bar",
            118.0f,
            8.0f,
            stage.hontoMasterVolume(),
            honto::hontoRGBA(110, 236, 158),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255)
        )
            .hontoAt(66.0f, 140.0f)
            .hontoLayer(3);
        auto musicBar = stage.hontoBar(
            "music_bar",
            118.0f,
            8.0f,
            stage.hontoBusVolume("music"),
            honto::hontoRGBA(96, 208, 255),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255)
        )
            .hontoAt(66.0f, 152.0f)
            .hontoLayer(3);
        auto effectBar = stage.hontoBar(
            "effect_bar",
            118.0f,
            8.0f,
            stage.hontoBusVolume("effect"),
            honto::hontoRGBA(255, 184, 96),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255)
        )
            .hontoAt(66.0f, 164.0f)
            .hontoLayer(3);

        auto scout = stage.hontoImage("scout", previewSheet, 26.0f, 26.0f)
            .hontoAt(216.0f, 144.0f)
            .hontoLayer(3);
        scout.hontoAnimateFrames()
            .hontoTexture(previewSheet)
            .hontoFrameSize(16, 16)
            .hontoFrames({ 0, 1, 2, 3, 2, 1 })
            .hontoFPS(9.0f)
            .hontoLoop()
            .hontoPlay();
        scout.hontoAnimate()
            .hontoMoveTo(278.0f, 144.0f)
            .hontoIn(1.2f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoWhenClicked(
            playMusicButton,
            [stage, statusLabel]()
            {
                if (stage.hontoPlayMusic("sandbox/assets/honto_theme.wav"))
                {
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    statusLabel.hontoTextValue("STATUS: MUSIC PLAYING");
                }
                else
                {
                    statusLabel.hontoTextValue("STATUS: MUSIC FAILED");
                }
            }
        );

        stage.hontoWhenClicked(
            stopMusicButton,
            [stage, statusLabel]()
            {
                stage.hontoStopAudioBus("music");
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                statusLabel.hontoTextValue("STATUS: MUSIC STOPPED");
            }
        );

        stage.hontoWhenClicked(
            fxButton,
            [stage, statusLabel]()
            {
                if (stage.hontoPlayEffect("sandbox/assets/honto_click.wav"))
                {
                    statusLabel.hontoTextValue("STATUS: EFFECT BUS TEST");
                }
                else
                {
                    statusLabel.hontoTextValue("STATUS: EFFECT FAILED");
                }
            }
        );

        stage.hontoWhenClicked(
            masterDownButton,
            [stage, statusLabel]()
            {
                const float nextVolume = Clamp01(stage.hontoMasterVolume() - 0.1f);
                stage.hontoSetMasterVolume(nextVolume);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                statusLabel.hontoTextValue("STATUS: MASTER " + std::to_string(static_cast<int>(nextVolume * 100.0f)) + "%");
            }
        );

        stage.hontoWhenClicked(
            masterUpButton,
            [stage, statusLabel]()
            {
                const float nextVolume = Clamp01(stage.hontoMasterVolume() + 0.1f);
                stage.hontoSetMasterVolume(nextVolume);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                statusLabel.hontoTextValue("STATUS: MASTER " + std::to_string(static_cast<int>(nextVolume * 100.0f)) + "%");
            }
        );

        stage.hontoWhenClicked(
            saveJsonButton,
            [stage, level, statusLabel]()
            {
                if (honto::hontoSaveLevel("sandbox/levels/platform_exported.json", *level))
                {
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    statusLabel.hontoTextValue("STATUS: JSON SAVED");
                }
                else
                {
                    statusLabel.hontoTextValue("STATUS: JSON SAVE FAILED");
                }
            }
        );

        stage.hontoWhenClicked(
            loadJsonButton,
            [stage, level, levelLabel, statusLabel]()
            {
                honto::hontoLevel loaded = honto::hontoLoadLevel("sandbox/levels/platform.json");
                if (!loaded.IsValid())
                {
                    statusLabel.hontoTextValue("STATUS: JSON LOAD FAILED");
                    return;
                }

                *level = loaded;
                levelLabel.hontoTextValue(level->title);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                statusLabel.hontoTextValue("STATUS: JSON LOADED");
            }
        );

        stage.hontoWhenClicked(
            loadTiledButton,
            [stage, level, levelLabel, statusLabel]()
            {
                honto::hontoLevel loaded = honto::hontoLoadLevel("sandbox/levels/platform_tiled.json");
                if (!loaded.IsValid())
                {
                    statusLabel.hontoTextValue("STATUS: TILED LOAD FAILED");
                    return;
                }

                *level = loaded;
                levelLabel.hontoTextValue(level->title);
                stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                statusLabel.hontoTextValue("STATUS: TILED LOADED");
            }
        );

        stage.hontoWhenClicked(
            saveTextButton,
            [stage, level, statusLabel]()
            {
                if (honto::hontoSaveLevel("sandbox/levels/platform_exported.honto", *level))
                {
                    stage.hontoPlayOnBus("effect", "sandbox/assets/honto_click.wav");
                    statusLabel.hontoTextValue("STATUS: HONTO SAVED");
                }
                else
                {
                    statusLabel.hontoTextValue("STATUS: HONTO SAVE FAILED");
                }
            }
        );

        stage.hontoEveryFrame(
            [stage, masterBar, musicBar, effectBar, mouseLabel, t = 0.0f](float deltaTime) mutable
            {
                t += deltaTime;
                const float wave = (std::sin(t * 2.4f) * 0.5f) + 0.5f;
                masterBar.hontoBarValue(stage.hontoMasterVolume());
                musicBar.hontoBarValue(stage.hontoBusVolume("music"));
                effectBar.hontoBarValue(std::max(stage.hontoBusVolume("effect") * 0.65f, wave * 0.35f));
                mouseLabel.hontoTextValue(MakeMouseText(stage));
            }
        );
    }
}

int main()
{
    return honto::hontoGame("honto Engine Feature Sandbox")
        .hontoWindowId("main_world")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoClear(honto::hontoRGBA(16, 18, 26))
        .hontoOpenWindow(
            "honto Multiverse Window",
            900,
            560,
            320,
            180,
            BuildMultiverseScene,
            honto::hontoRGBA(12, 18, 34)
        )
        .hontoOpenWindow(
            "honto Tool Window",
            960,
            540,
            320,
            180,
            BuildToolsWindow,
            honto::hontoRGBA(20, 22, 30)
        )
        .hontoPlay(BuildPlatformScene)
        .hontoRun();
}
