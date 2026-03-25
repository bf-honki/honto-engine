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
            auto cleared = std::make_shared<bool>(false);

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
            stage.hontoCameraFollowSmooth(player, 1.0f, 8.0f);

            auto beacon = stage.hontoBox("beacon", 18.0f, 26.0f, honto::hontoRGBA(248, 190, 88))
                .hontoAt(274.0f, 38.0f)
                .hontoLayer(3);

            auto beaconTrigger = stage.hontoTrigger("beacon_trigger", 26.0f, 34.0f)
                .hontoAt(270.0f, 34.0f)
                .hontoLayer(4);

            auto beaconFx = stage.hontoParticles("beacon_fx", 24.0f, 32.0f);
            beaconFx.hontoAt(270.0f, 34.0f).hontoLayer(2);
            beaconFx.hontoEmissionRate(16.0f);
            beaconFx.hontoSpawnArea(24.0f, 32.0f);
            beaconFx.hontoVelocityRange({ -10.0f, -26.0f }, { 10.0f, -64.0f });
            beaconFx.hontoLifetimeRange(0.35f, 0.9f);
            beaconFx.hontoSizeRange(2.0f, 4.0f);
            beaconFx.hontoColorRange(honto::hontoRGBA(255, 236, 162, 220), honto::hontoRGBA(255, 132, 92, 0));

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

            stage.hontoWhenTouching(
                player,
                beaconTrigger,
                [stage, text, beaconFx, cleared]()
                {
                    if (*cleared)
                    {
                        return;
                    }

                    *cleared = true;
                    text.hontoTextValue("SCENE CLEAR");
                    beaconFx.hontoBurst(20);
                    stage.hontoPlayTone(960, 90);
                    stage.hontoCameraShake(2.4f, 0.2f, 18.0f);
                },
                true
            );
        })
        .hontoRun();
}
