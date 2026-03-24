#include "honto/HonTo.h"

#include <memory>
#include <vector>

namespace
{
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
        auto portalOpen = std::make_shared<bool>(false);
        auto checkerTexture = honto::hontoCheckerTexture(
            32,
            32,
            honto::hontoRGBA(40, 58, 98),
            honto::hontoRGBA(86, 132, 212),
            4
        );

        honto::hontoPrint("Platform scene: A/D move, Space jump, Enter enters the 2.5D dungeon.");

        stage.hontoGravity(0.0f, 560.0f);
        stage.hontoBackground(18, 22, 34);
        stage.hontoFill("sky", 960.0f, 84.0f, honto::hontoRGBA(28, 40, 72)).hontoAt(0.0f, 0.0f).hontoLayer(1);
        stage.hontoFill("ground", 960.0f, 26.0f, honto::hontoRGBA(58, 116, 70)).hontoAt(0.0f, 154.0f).hontoLayer(1);

        auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(98, 232, 132))
            .hontoAt(36.0f, 42.0f)
            .hontoLayer(4)
            .hontoUseGravity()
            .hontoGroundAt(138.0f)
            .hontoMoveLeftRight(128.0f)
            .hontoJumpWhenPressed(honto::hontoKey::Space, 255.0f);

        stage.hontoCameraFollow(player, 1.0f);

        auto crate = stage.hontoImage("crate", checkerTexture, 32.0f, 32.0f)
            .hontoAt(220.0f, 122.0f)
            .hontoLayer(3);
        crate.hontoAnimate()
            .hontoMoveTo(252.0f, 118.0f)
            .hontoIn(1.4f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto portal = stage.hontoChecker(
            "portal",
            24.0f,
            48.0f,
            honto::hontoRGBA(250, 182, 84),
            honto::hontoRGBA(122, 84, 255),
            3
        )
            .hontoAt(760.0f, 92.0f)
            .hontoLayer(3);
        portal.hontoAnimate()
            .hontoScaleTo(1.12f)
            .hontoPaintTo(honto::hontoRGBA(255, 228, 160))
            .hontoIn(0.9f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        stage.hontoEveryFrame(
            [player, portal, crate, stage, portalOpen](float)
            {
                player.hontoPaint(player.hontoIsOnGround() ? honto::hontoRGBA(98, 232, 132) : honto::hontoRGBA(160, 255, 190));

                if (player.hontoTouching(crate))
                {
                    player.hontoPaint(honto::hontoRGBA(255, 224, 120));
                }

                if (player.hontoTouching(portal))
                {
                    *portalOpen = true;
                }
            }
        );

        stage.hontoWhenPressed(
            honto::hontoKey::Enter,
            [stage, portalOpen]()
            {
                if (!*portalOpen)
                {
                    honto::hontoPrint("Reach the portal first to unlock the 2.5D dungeon.");
                    return;
                }

                honto::hontoPrint("Switching to the raycast dungeon.");
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
                honto::hontoPrint("Returning to the platform scene.");
                stage.hontoGoWithFade(BuildPlatformScene, 0.7f);
            }
        );

        (void)raycast;
    }

    void BuildToolsWindow(honto::hontoStage& stage)
    {
        auto texture = honto::hontoCheckerTexture(
            48,
            48,
            honto::hontoRGBA(86, 236, 154),
            honto::hontoRGBA(34, 88, 78),
            6
        );

        stage.hontoBackground(20, 22, 30);
        stage.hontoOutline(228.0f, 122.0f, honto::hontoRGBA(74, 90, 114), 1).hontoAt(6.0f, 6.0f).hontoLayer(1);

        auto panel = stage.hontoImage("panel", texture, 54.0f, 54.0f)
            .hontoAt(30.0f, 30.0f)
            .hontoLayer(2);
        panel.hontoAnimate()
            .hontoMoveTo(150.0f, 30.0f)
            .hontoScaleTo(1.2f)
            .hontoIn(1.0f)
            .hontoPingPong()
            .hontoLoop()
            .hontoPlay();

        auto pulse = stage.hontoBox("pulse", 18.0f, 18.0f, honto::hontoRGBA(250, 198, 98))
            .hontoAt(108.0f, 88.0f)
            .hontoLayer(3);
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
