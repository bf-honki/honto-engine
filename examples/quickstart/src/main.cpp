#include "honto/HonTo.h"

namespace
{
    void BuildHud(honto::hontoStage& stage)
    {
        stage.hontoBackground(22, 24, 34);
        stage.hontoText("hud_title", "HONTO QUICKSTART", honto::hontoRGBA(240, 245, 255), 2)
            .hontoAt(10.0f, 10.0f)
            .hontoLayer(2);
        stage.hontoText("hud_hint", "A D MOVE  SPACE JUMP", honto::hontoRGBA(184, 198, 220), 1)
            .hontoAt(10.0f, 28.0f)
            .hontoLayer(2);
    }
}

int main()
{
    return honto::hontoGame("HonTo QuickStart")
        .hontoWindow(1280, 720)
        .hontoRender(320, 180)
        .hontoOpenWindow("HonTo HUD", 720, 420, 240, 135, BuildHud)
        .hontoPlay([](honto::hontoStage& stage)
        {
            stage.hontoBackground(18, 22, 34);
            stage.hontoGravity(0.0f, 760.0f);

            auto world = stage.hontoTileMap(
                "world",
                {
                    "....................",
                    "..........##........",
                    "......##............",
                    "....................",
                    "####################"
                },
                16.0f,
                16.0f
            );
            world.hontoTile('#', honto::hontoRGBA(84, 128, 94), true, true);

            auto player = stage.hontoBox("player", 16.0f, 16.0f, honto::hontoRGBA(96, 228, 136))
                .hontoAt(24.0f, 24.0f)
                .hontoLayer(3)
                .hontoUseGravity()
                .hontoCollideWithMap(world)
                .hontoMoveLeftRight(124.0f)
                .hontoJumpWhenPressed(honto::hontoKey::Space, 260.0f);

            auto beacon = stage.hontoBox("beacon", 18.0f, 26.0f, honto::hontoRGBA(248, 190, 88))
                .hontoAt(274.0f, 38.0f)
                .hontoLayer(3);

            beacon.hontoAnimate()
                .hontoScaleTo(1.08f)
                .hontoPaintTo(honto::hontoRGBA(255, 236, 162))
                .hontoIn(0.85f)
                .hontoPingPong()
                .hontoLoop()
                .hontoPlay();

            auto text = stage.hontoText("state", "REACH THE BEACON", honto::hontoRGBA(255, 238, 162), 1)
                .hontoAt(8.0f, 8.0f)
                .hontoLayer(10);

            stage.hontoEveryFrame(
                [player, beacon, text, stage](float)
                {
                    if (player.hontoTouching(beacon))
                    {
                        text.hontoTextValue("SCENE CLEAR");
                        stage.hontoPlayTone(960, 90);
                    }
                }
            );
        })
        .hontoRun();
}
