#include "honto/HonTo.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
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
        "#...A....#",
        "#.#.#.##A#",
        "#.#...#..#",
        "#.###.#A.#",
        "#.....#..#",
        "#A#####..#",
        "#........#",
        "##########"
    };

    honto::hontoLevel LoadPlatformLevel()
    {
        honto::hontoLevel level = honto::hontoLoadLevel("sandbox/levels/platform.honto");
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
        const std::string hudHint = hintEntity != nullptr && !hintEntity->text.empty() ? hintEntity->text : "A/D MOVE  SPACE JUMP  ENTER PORTAL";

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

        honto::hontoPrint("Platform scene: A/D move, Space jump, F5 save level, Enter opens the 2.5D dungeon.");

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
                    statusLabel.hontoTextValue("STATUS: PORTAL OPEN");
                    stage.hontoPlayTone(980, 120);
                    stage.hontoPlayAlias("SystemAsterisk");
                    honto::hontoPrint("Portal unlocked. Press Enter.");
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

        (void)saveLabel;
    }

    void BuildDoomScene(honto::hontoStage& stage)
    {
        honto::hontoPrint("Raycast scene: W/S move, A/D strafe, Left/Right turn, Enter returns.");

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

        stage.hontoBackground(10, 12, 16);

        auto raycast = stage.hontoRaycast("dungeon", 320.0f, 180.0f);
        raycast.hontoAt(0.0f, 0.0f);
        raycast.hontoLayer(1);
        raycast.hontoMap(kDungeonMap)
            .hontoPlayer(1.5f, 1.5f, 0.0f)
            .hontoViewDegrees(68.0f)
            .hontoMoveSpeed(2.8f)
            .hontoTurnSpeed(2.2f)
            .hontoFloor(honto::hontoRGBA(38, 36, 44))
            .hontoCeiling(honto::hontoRGBA(18, 22, 34))
            .hontoWall('#', honto::hontoRGBA(174, 112, 92))
            .hontoWallTexture('#', wallA)
            .hontoWall('A', honto::hontoRGBA(92, 140, 196))
            .hontoWallTexture('A', wallB)
            .hontoMiniMap(true, 7.0f)
            .hontoDoomControls();

        stage.hontoFill("hud", 320.0f, 24.0f, honto::hontoRGBA(8, 10, 14, 180))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(2);
        stage.hontoOutline("hud_border", 320.0f, 24.0f, honto::hontoRGBA(78, 88, 114), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(3);
        stage.hontoText("doom_hint", "W/S MOVE  A/D STRAFE  ENTER BACK", honto::hontoRGBA(238, 245, 255), 1)
            .hontoAt(8.0f, 8.0f)
            .hontoLayer(4);

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

    void BuildToolsWindow(honto::hontoStage& stage)
    {
        auto level = LoadPlatformLevel();
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

        stage.hontoBackground(20, 22, 30);
        stage.hontoOutline(228.0f, 122.0f, honto::hontoRGBA(74, 90, 114), 1).hontoAt(6.0f, 6.0f).hontoLayer(1);
        stage.hontoText("tool_title", "PNG  UI  LEVEL", honto::hontoRGBA(236, 245, 255), 2)
            .hontoAt(12.0f, 10.0f)
            .hontoLayer(3);
        stage.hontoText("tool_level", level.title, honto::hontoRGBA(166, 188, 216), 1)
            .hontoAt(12.0f, 30.0f)
            .hontoLayer(3);

        stage.hontoImage("tool_badge", "sandbox/assets/honto_badge.png", 24.0f, 24.0f)
            .hontoAt(196.0f, 12.0f)
            .hontoLayer(3);

        auto bar = stage.hontoBar(
            "tool_bar",
            88.0f,
            10.0f,
            0.45f,
            honto::hontoRGBA(110, 236, 158),
            honto::hontoRGBA(10, 14, 22, 220),
            honto::hontoRGBA(236, 245, 255)
        )
            .hontoAt(12.0f, 46.0f)
            .hontoLayer(3);

        auto scout = stage.hontoImage("scout", previewSheet, 26.0f, 26.0f)
            .hontoAt(30.0f, 82.0f)
            .hontoLayer(3);
        scout.hontoAnimateFrames()
            .hontoTexture(previewSheet)
            .hontoFrameSize(16, 16)
            .hontoFrames({ 0, 1, 2, 3, 2, 1 })
            .hontoFPS(9.0f)
            .hontoLoop()
            .hontoPlay();
        scout.hontoAnimate()
            .hontoMoveTo(188.0f, 82.0f)
            .hontoIn(1.2f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoEveryFrame(
            [bar, t = 0.0f](float deltaTime) mutable
            {
                t += deltaTime;
                const float wave = (std::sin(t * 2.4f) * 0.5f) + 0.5f;
                bar.hontoBarValue(wave);
            }
        );
    }
}

int main()
{
    return honto::hontoGame("honto Engine Feature Sandbox")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoClear(honto::hontoRGBA(16, 18, 26))
        .hontoOpenWindow(
            "honto Tool Window",
            720,
            420,
            240,
            135,
            BuildToolsWindow,
            honto::hontoRGBA(20, 22, 30)
        )
        .hontoPlay(BuildPlatformScene)
        .hontoRun();
}
