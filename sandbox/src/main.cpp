#include "honto/HonTo.h"

#include <memory>
#include <string>
#include <vector>

namespace
{
    const std::vector<std::string> kPlatformMap {
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

    void BuildPlatformScene(honto::hontoStage& stage);
    void BuildDoomScene(honto::hontoStage& stage);
    void BuildToolsWindow(honto::hontoStage& stage);

    void BuildPlatformScene(honto::hontoStage& stage)
    {
        auto portalUnlocked = std::make_shared<bool>(false);
        auto jumpGate = std::make_shared<bool>(false);

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

        honto::hontoPrint("Platform scene: A/D move, Space jump, Enter opens the 2.5D dungeon after reaching the portal.");

        stage.hontoBackground(18, 22, 34);
        stage.hontoGravity(0.0f, 760.0f);

        stage.hontoFill("sky", 960.0f, 88.0f, honto::hontoRGBA(28, 40, 72)).hontoAt(0.0f, 0.0f).hontoLayer(0);
        stage.hontoFill("haze", 960.0f, 104.0f, honto::hontoRGBA(44, 66, 112, 120)).hontoAt(0.0f, 52.0f).hontoLayer(0);

        auto world = stage.hontoTileMap("world", kPlatformMap, 16.0f, 16.0f);
        world.hontoAt(0.0f, 0.0f);
        world.hontoLayer(1);
        world.hontoTileTextureRegion('#', tileSheet, 0, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        world.hontoTileTextureRegion('B', tileSheet, 16, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        world.hontoTileTextureRegion('C', tileSheet, 32, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);

        auto player = stage.hontoImage("player", playerSheet, 16.0f, 16.0f)
            .hontoAt(36.0f, 40.0f)
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
            20.0f,
            36.0f,
            honto::hontoRGBA(250, 184, 80),
            honto::hontoRGBA(112, 88, 255),
            3
        );
        portal.hontoAt(594.0f, 140.0f);
        portal.hontoLayer(5);
        portal.hontoAnimate()
            .hontoScaleTo(1.08f)
            .hontoPaintTo(honto::hontoRGBA(255, 236, 162))
            .hontoIn(0.85f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

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

        stage.hontoEveryFrame(
            [player, portal, portalUnlocked, jumpGate, stage](float)
            {
                player.hontoPaint(player.hontoIsOnGround() ? honto::hontoRGBA(255, 255, 255) : honto::hontoRGBA(196, 220, 255));

                if (player.hontoIsOnGround())
                {
                    *jumpGate = false;
                }

                if (player.hontoTouching(portal) && !*portalUnlocked)
                {
                    *portalUnlocked = true;
                    stage.hontoPlayTone(980, 120);
                    honto::hontoPrint("Portal unlocked. Press Enter.");
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
                    honto::hontoPrint("Reach the portal first.");
                    return;
                }

                stage.hontoPlayTone(920, 110);
                stage.hontoGoWithFade(BuildDoomScene, 0.7f);
            }
        );
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

        stage.hontoFill("hud", 320.0f, 18.0f, honto::hontoRGBA(8, 10, 14, 180))
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(2);
        stage.hontoOutline("hud_border", 320.0f, 18.0f, honto::hontoRGBA(78, 88, 114), 1)
            .hontoAt(0.0f, 0.0f)
            .hontoLayer(3);

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage]()
            {
                stage.hontoPlayTone(620, 100);
                stage.hontoGoWithFade(BuildPlatformScene, 0.7f);
            }
        );
    }

    void BuildToolsWindow(honto::hontoStage& stage)
    {
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

        auto tileSheet = honto::hontoFrameSheetTexture(
            16,
            16,
            {
                honto::hontoRGBA(82, 126, 86),
                honto::hontoRGBA(142, 102, 76),
                honto::hontoRGBA(74, 110, 164)
            },
            3
        );

        stage.hontoBackground(20, 22, 30);
        stage.hontoOutline(228.0f, 122.0f, honto::hontoRGBA(74, 90, 114), 1).hontoAt(6.0f, 6.0f).hontoLayer(1);

        auto previewMap = stage.hontoTileMap(
            "preview_map",
            {
                "................",
                "..BBB...........",
                "......CCC.......",
                "....####........",
                "################"
            },
            12.0f,
            12.0f
        );
        previewMap.hontoAt(12.0f, 64.0f);
        previewMap.hontoLayer(2);
        previewMap.hontoTileTextureRegion('#', tileSheet, 0, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        previewMap.hontoTileTextureRegion('B', tileSheet, 16, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);
        previewMap.hontoTileTextureRegion('C', tileSheet, 32, 0, 16, 16, honto::hontoRGBA(255, 255, 255), true, true);

        auto scout = stage.hontoImage("scout", previewSheet, 26.0f, 26.0f)
            .hontoAt(26.0f, 22.0f)
            .hontoLayer(3);
        scout.hontoAnimateFrames()
            .hontoTexture(previewSheet)
            .hontoFrameSize(16, 16)
            .hontoFrames({ 0, 1, 2, 3, 2, 1 })
            .hontoFPS(9.0f)
            .hontoLoop()
            .hontoPlay();
        scout.hontoAnimate()
            .hontoMoveTo(184.0f, 22.0f)
            .hontoIn(1.2f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto pulse = stage.hontoBox("pulse", 18.0f, 18.0f, honto::hontoRGBA(250, 198, 98))
            .hontoAt(108.0f, 38.0f)
            .hontoLayer(4);
        pulse.hontoAnimate()
            .hontoScaleTo(1.8f)
            .hontoPaintTo(honto::hontoRGBA(255, 240, 170))
            .hontoIn(0.7f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();
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
