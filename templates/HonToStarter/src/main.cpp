#include "honto/HonTo.h"

#include <memory>

namespace
{
    void BuildTools(honto::hontoStage& stage)
    {
        stage.hontoBackground(18, 20, 30);
        stage.hontoText("title", "__HONTO_PROJECT_NAME__ TOOLS", honto::hontoRGBA(238, 245, 255), 2)
            .hontoAt(10.0f, 10.0f)
            .hontoLayer(2);
        stage.hontoText("hint", "SPACE TONE  ENTER RESET", honto::hontoRGBA(180, 196, 220), 1)
            .hontoAt(10.0f, 28.0f)
            .hontoLayer(2);
    }
}

int main()
{
    return honto::hontoGame("__HONTO_PROJECT_NAME__")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoClear(honto::hontoRGBA(14, 18, 30))
        .hontoOpenWindow("__HONTO_PROJECT_NAME__ Tools", 720, 420, 240, 135, BuildTools)
        .hontoPlay([](honto::hontoStage& stage)
        {
            auto unlocked = std::make_shared<bool>(false);

            stage.hontoBackground(18, 22, 34);
            stage.hontoGravity(0.0f, 780.0f);

            auto world = stage.hontoTileMap(
                "world",
                {
                    "....................",
                    "..............##....",
                    "..........##........",
                    "......###...........",
                    "####################"
                },
                16.0f,
                16.0f
            );
            world.hontoTile('#', honto::hontoRGBA(78, 120, 86), true, true);

            auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(98, 232, 132))
                .hontoAt(24.0f, 28.0f)
                .hontoLayer(3)
                .hontoUseGravity()
                .hontoCollideWithMap(world)
                .hontoMoveLeftRight(126.0f)
                .hontoJumpWhenPressed(honto::hontoKey::Space, 270.0f);

            auto portal = stage.hontoChecker(
                "portal",
                20.0f,
                32.0f,
                honto::hontoRGBA(250, 184, 80),
                honto::hontoRGBA(112, 88, 255),
                3
            )
                .hontoAt(276.0f, 32.0f)
                .hontoLayer(3);

            portal.hontoAnimate()
                .hontoScaleTo(1.08f)
                .hontoPaintTo(honto::hontoRGBA(255, 236, 162))
                .hontoIn(0.8f)
                .hontoPingPong()
                .hontoLoop()
                .hontoPlay();

            stage.hontoText("title", "__HONTO_PROJECT_NAME__", honto::hontoRGBA(238, 245, 255), 2)
                .hontoAt(8.0f, 8.0f)
                .hontoLayer(10);
            auto status = stage.hontoText("status", "MOVE TO THE PORTAL", honto::hontoRGBA(255, 238, 162), 1)
                .hontoAt(8.0f, 26.0f)
                .hontoLayer(10);

            stage.hontoWhenPressed(
                honto::hontoKey::Space,
                [stage]()
                {
                    stage.hontoPlayTone(740, 70);
                }
            );

            stage.hontoWhenPressed(
                honto::hontoKey::Enter,
                [stage]()
                {
                    stage.hontoGoWithFade([](honto::hontoStage& next)
                    {
                        next.hontoBackground(18, 22, 34);
                    }, 0.6f);
                }
            );

            stage.hontoEveryFrame(
                [stage, player, portal, unlocked, status](float)
                {
                    if (player.hontoTouching(portal))
                    {
                        if (!*unlocked)
                        {
                            *unlocked = true;
                            status.hontoTextValue("PORTAL READY");
                            stage.hontoPlayTone(920, 100);
                        }
                    }
                    else if (!*unlocked)
                    {
                        status.hontoTextValue("MOVE TO THE PORTAL");
                    }
                }
            );
        })
        .hontoRun();
}
