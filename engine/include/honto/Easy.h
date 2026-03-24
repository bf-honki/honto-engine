#pragma once

#include "Application.h"
#include "Audio.h"
#include "Input.h"
#include "Level.h"
#include "Raycast.h"
#include "SceneGraph.h"
#include "TileMap.h"
#include "Texture.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace HonTo
{
    using Vec2 = honto::Vec2;
    using Color = honto::Color;
    using Key = honto::KeyCode;
    using Level = honto::LevelDocument;
    using LevelEntity = honto::LevelEntity;
    using Texture = honto::Texture;

    namespace detail
    {
        class FrameNode final : public honto::Node
        {
        public:
            bool Init() override
            {
                return true;
            }

            void SetColor(Color color)
            {
                m_Color = color;
            }

            Color GetColor() const
            {
                return m_Color;
            }

            void SetThickness(int thickness)
            {
                m_Thickness = thickness;
            }

            void Draw(honto::Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override
            {
                renderer.DrawRectOutline(worldPosition, GetContentSize() * worldScale, m_Color, m_Thickness);
            }

        private:
            Color m_Color { 255, 255, 255, 255 };
            int m_Thickness = 1;
        };

        class BuiltGame final : public honto::Game
        {
        public:
            struct BuiltWindow
            {
                honto::AppConfig config {};
                bool closeStopsGame = false;
                std::function<std::unique_ptr<honto::Scene>()> sceneFactory;
            };

            BuiltGame(
                honto::AppConfig config,
                std::function<std::unique_ptr<honto::Scene>()> sceneFactory,
                std::vector<BuiltWindow> additionalWindows
            )
                : m_Config(std::move(config)),
                  m_SceneFactory(std::move(sceneFactory)),
                  m_AdditionalWindows(std::move(additionalWindows))
            {
            }

            honto::AppConfig GetConfig() const override
            {
                return m_Config;
            }

            std::unique_ptr<honto::Scene> CreateInitialScene() override
            {
                if (m_SceneFactory)
                {
                    return m_SceneFactory();
                }

                return nullptr;
            }

            std::vector<honto::WindowStartup> CreateAdditionalWindows() override
            {
                std::vector<honto::WindowStartup> windows;
                windows.reserve(m_AdditionalWindows.size());

                for (const BuiltWindow& builtWindow : m_AdditionalWindows)
                {
                    honto::WindowStartup startup;
                    startup.config = builtWindow.config;
                    startup.closeStopsGame = builtWindow.closeStopsGame;
                    startup.createScene = builtWindow.sceneFactory;
                    windows.push_back(std::move(startup));
                }

                return windows;
            }

        private:
            honto::AppConfig m_Config {};
            std::function<std::unique_ptr<honto::Scene>()> m_SceneFactory;
            std::vector<BuiltWindow> m_AdditionalWindows;
        };

        inline Vec2 Lerp(const Vec2& from, const Vec2& to, float t)
        {
            return {
                from.x + ((to.x - from.x) * t),
                from.y + ((to.y - from.y) * t)
            };
        }

        inline Color Lerp(Color from, Color to, float t)
        {
            const auto blend = [t](std::uint8_t start, std::uint8_t end)
            {
                return static_cast<std::uint8_t>(static_cast<float>(start) + ((static_cast<float>(end) - static_cast<float>(start)) * t));
            };

            return {
                blend(from.r, to.r),
                blend(from.g, to.g),
                blend(from.b, to.b),
                blend(from.a, to.a)
            };
        }
    }

    inline Color RGBA(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
    {
        return { r, g, b, a };
    }

    inline void Print(const std::string& text)
    {
        std::cout << text << '\n';
    }

    using Transition = honto::SceneTransition;

    inline Transition Fade(float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 })
    {
        Transition transition;
        transition.type = Transition::Type::Fade;
        transition.duration = std::max(0.05f, seconds);
        transition.color = color;
        return transition;
    }

    class Actor;
    class Animation;
    class FrameAnimation;
    class RaycastActor;
    class Stage;
    class TileMapActor;

    inline std::shared_ptr<Texture> LoadTexture(const std::string& path)
    {
        return Texture::LoadShared(path);
    }

    inline std::shared_ptr<Texture> CheckerTexture(
        int width,
        int height,
        Color a,
        Color b,
        int cellSize = 8
    )
    {
        return Texture::CreateCheckerboard(width, height, a, b, cellSize);
    }

    inline std::shared_ptr<Texture> FrameSheetTexture(
        int frameWidth,
        int frameHeight,
        const std::vector<Color>& frameColors,
        int columns = 0
    )
    {
        return Texture::CreateFrameSheet(frameWidth, frameHeight, frameColors, columns);
    }

    inline bool PlaySound(const std::string& path, bool loop = false)
    {
        return honto::Audio::PlayWav(path, loop);
    }

    inline bool PlayAlias(const std::string& alias, bool loop = false)
    {
        return honto::Audio::PlayAlias(alias, loop);
    }

    inline void StopAudio()
    {
        honto::Audio::Stop();
    }

    inline void PlayTone(int frequency, int durationMs)
    {
        honto::Audio::PlayTone(frequency, durationMs);
    }

    inline Level LoadLevel(const std::string& path)
    {
        Level level;
        honto::LevelFile::Load(path, level);
        return level;
    }

    inline bool SaveLevel(const std::string& path, const Level& level)
    {
        return honto::LevelFile::Save(path, level);
    }

    inline const LevelEntity* FindLevelEntity(const Level& level, const std::string& name)
    {
        return honto::FindLevelEntity(level, name);
    }

    template <typename Derived, typename TNode>
    class Chain
    {
    public:
        explicit Chain(std::shared_ptr<TNode> node = nullptr)
            : m_Node(std::move(node))
        {
        }

        Derived& At(float x, float y)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetPosition(x, y);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& At(const Vec2& position)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetPosition(position);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Size(float width, float height)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetContentSize(width, height);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Size(const Vec2& size)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetContentSize(size);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Scale(float uniformScale)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetScale(uniformScale);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Scale(float x, float y)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetScale({ x, y });
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Show(bool visible = true)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetVisible(visible);
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Hide()
        {
            return Show(false);
        }

        Derived& UpdateEveryFrame()
        {
            if (m_Node != nullptr)
            {
                m_Node->ScheduleUpdate();
            }

            return static_cast<Derived&>(*this);
        }

        Derived& Z(int zOrder)
        {
            m_ZOrder = zOrder;
            return static_cast<Derived&>(*this);
        }

        Derived& hontoAt(float x, float y)
        {
            return At(x, y);
        }

        Derived& hontoAt(const Vec2& position)
        {
            return At(position);
        }

        Derived& hontoSize(float width, float height)
        {
            return Size(width, height);
        }

        Derived& hontoSize(const Vec2& size)
        {
            return Size(size);
        }

        Derived& hontoScale(float uniformScale)
        {
            return Scale(uniformScale);
        }

        Derived& hontoScale(float x, float y)
        {
            return Scale(x, y);
        }

        Derived& hontoShow(bool visible = true)
        {
            return Show(visible);
        }

        Derived& hontoHide()
        {
            return Hide();
        }

        Derived& hontoUpdateEveryFrame()
        {
            return UpdateEveryFrame();
        }

        Derived& hontoLayer(int zOrder)
        {
            return Z(zOrder);
        }

        std::shared_ptr<TNode> Share() const
        {
            return m_Node;
        }

        int GetZOrder() const
        {
            return m_ZOrder;
        }

    protected:
        std::shared_ptr<TNode> m_Node;
        int m_ZOrder = 0;
    };

    class Sprite final : public Chain<Sprite, honto::Sprite>
    {
    public:
        explicit Sprite(std::shared_ptr<honto::Sprite> node = honto::Sprite::CreateSolid({ 16.0f, 16.0f }, RGBA(255, 255, 255)))
            : Chain(std::move(node))
        {
        }

        Sprite& Paint(Color color)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetColor(color);
            }

            return *this;
        }

        Sprite& Paint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(RGBA(r, g, b, a));
        }

        Sprite& UseTexture(const std::shared_ptr<Texture>& texture)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetTexture(texture);
            }

            return *this;
        }

        Sprite& hontoPaint(Color color)
        {
            return Paint(color);
        }

        Sprite& hontoPaint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(r, g, b, a);
        }

        Sprite& hontoUseTexture(const std::shared_ptr<Texture>& texture)
        {
            return UseTexture(texture);
        }
    };

    class Layer final : public Chain<Layer, honto::LayerColor>
    {
    public:
        explicit Layer(std::shared_ptr<honto::LayerColor> node = honto::LayerColor::Create(RGBA(255, 255, 255), 16.0f, 16.0f))
            : Chain(std::move(node))
        {
        }

        Layer& Paint(Color color)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetColor(color);
            }

            return *this;
        }

        Layer& Paint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(RGBA(r, g, b, a));
        }

        Layer& hontoPaint(Color color)
        {
            return Paint(color);
        }

        Layer& hontoPaint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(r, g, b, a);
        }
    };

    class Frame final : public Chain<Frame, detail::FrameNode>
    {
    public:
        Frame()
            : Chain(std::make_shared<detail::FrameNode>())
        {
            if (m_Node != nullptr)
            {
                m_Node->Init();
            }
        }

        Frame& Paint(Color color)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetColor(color);
            }

            return *this;
        }

        Frame& Paint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(RGBA(r, g, b, a));
        }

        Frame& Thickness(int thickness)
        {
            if (m_Node != nullptr)
            {
                m_Node->SetThickness(thickness);
            }

            return *this;
        }

        Frame& hontoPaint(Color color)
        {
            return Paint(color);
        }

        Frame& hontoPaint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Paint(r, g, b, a);
        }

        Frame& hontoThickness(int thickness)
        {
            return Thickness(thickness);
        }
    };

    inline Sprite Box(float width, float height, Color color)
    {
        return Sprite(honto::Sprite::CreateSolid({ width, height }, color)).Size(width, height).Paint(color);
    }

    inline Sprite Image(const std::shared_ptr<Texture>& texture, float width, float height, Color tint = RGBA(255, 255, 255))
    {
        return Sprite(honto::Sprite::CreateTextured({ width, height }, texture, tint)).Size(width, height).Paint(tint);
    }

    inline Sprite Image(const std::string& path, float width, float height, Color tint = RGBA(255, 255, 255))
    {
        return Image(LoadTexture(path), width, height, tint);
    }

    inline Layer Fill(float width, float height, Color color)
    {
        return Layer(honto::LayerColor::Create(color, width, height)).Size(width, height).Paint(color);
    }

    inline Layer Fill(const Vec2& size, Color color)
    {
        return Fill(size.x, size.y, color);
    }

    inline Frame Outline(float width, float height, Color color, int thickness = 1)
    {
        return Frame().Size(width, height).Paint(color).Thickness(thickness);
    }

    inline Vec2 VisibleSize()
    {
        return honto::Director::Get().GetVisibleSize();
    }

    inline bool Pressing(Key key)
    {
        return honto::Input::IsKeyDown(key);
    }

    inline bool Pressed(Key key)
    {
        return honto::Input::IsKeyPressed(key);
    }

    class Scene : public honto::CodeScene
    {
    public:
        virtual bool Setup()
        {
            return true;
        }

        bool Init() override
        {
            return Setup();
        }

        void EveryFrame()
        {
            ScheduleUpdate();
        }

        void hontoEveryFrame()
        {
            EveryFrame();
        }

        template <typename TNode>
        std::shared_ptr<TNode> Add(const std::shared_ptr<TNode>& node, int zOrder = 0)
        {
            AddChild(node, zOrder);
            return node;
        }

        template <typename TChain>
        auto Add(const TChain& chain) -> decltype(chain.Share())
        {
            auto node = chain.Share();
            AddChild(node, chain.GetZOrder());
            return node;
        }

        template <typename TNode>
        std::shared_ptr<TNode> hontoAdd(const std::shared_ptr<TNode>& node, int zOrder = 0)
        {
            return Add(node, zOrder);
        }

        template <typename TChain>
        auto hontoAdd(const TChain& chain) -> decltype(chain.Share())
        {
            return Add(chain);
        }

        template <typename TScene>
        void Go()
        {
            Go(std::make_unique<TScene>());
        }

        template <typename TScene>
        void Go(const Transition& transition)
        {
            Go(std::make_unique<TScene>(), transition);
        }

        void Go(std::unique_ptr<honto::Scene> scene, const Transition& transition = {})
        {
            honto::Director::Get().ReplaceScene(std::move(scene), transition);
        }

        template <typename TScene>
        void hontoGo()
        {
            Go<TScene>();
        }

        template <typename TScene>
        void hontoGo(const Transition& transition)
        {
            Go<TScene>(transition);
        }

        void hontoGo(std::unique_ptr<honto::Scene> scene, const Transition& transition = {})
        {
            Go(std::move(scene), transition);
        }
    };

    namespace detail
    {
        struct StageState;

        struct ActorState
        {
            std::shared_ptr<honto::Node> node;
            std::weak_ptr<StageState> stage;
            std::string name;
            Vec2 velocity {};
            bool physicsAttached = false;
            bool movesByVelocity = false;
            bool usesGravity = false;
            float gravityScale = 1.0f;
            bool hasGround = false;
            float groundY = 0.0f;
            float groundBounce = 0.0f;
            bool onGround = false;
            std::weak_ptr<honto::TileMap> collisionMap;
        };

        struct StageState
        {
            std::unordered_map<std::string, std::shared_ptr<ActorState>> namedActors;
            std::unordered_map<std::string, std::shared_ptr<honto::TileMap>> namedTileMaps;
            std::vector<std::function<void(float)>> updates;
            HonTo::Scene* scene = nullptr;
            std::size_t nextAnonymousId = 1;
            Vec2 gravity { 0.0f, 0.0f };
        };

        class ScriptScene final : public HonTo::Scene
        {
        public:
            explicit ScriptScene(std::function<void(Stage&)> setup)
                : m_Setup(std::move(setup)),
                  m_State(std::make_shared<StageState>())
            {
            }

            bool Setup() override;
            void Update(float deltaTime) override;

        private:
            std::function<void(Stage&)> m_Setup;
            std::shared_ptr<StageState> m_State;
        };
    }

    class Actor
    {
    public:
        Actor() = default;

        explicit Actor(std::shared_ptr<detail::ActorState> state)
            : m_State(std::move(state))
        {
        }

        explicit operator bool() const
        {
            return Node() != nullptr;
        }

        std::string Name() const
        {
            if (m_State == nullptr)
            {
                return {};
            }

            return m_State->name;
        }

        Vec2 Position() const
        {
            if (const auto node = Node())
            {
                return node->GetPosition();
            }

            return {};
        }

        Vec2 Size() const
        {
            if (const auto node = Node())
            {
                return node->GetContentSize();
            }

            return {};
        }

        Vec2 ScaleValue() const
        {
            if (const auto node = Node())
            {
                return node->GetScale();
            }

            return { 1.0f, 1.0f };
        }

        Color Tint() const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                return sprite->GetColor();
            }

            if (const auto layer = std::dynamic_pointer_cast<honto::LayerColor>(Node()))
            {
                return layer->GetColor();
            }

            if (const auto frame = std::dynamic_pointer_cast<detail::FrameNode>(Node()))
            {
                return frame->GetColor();
            }

            if (const auto label = std::dynamic_pointer_cast<honto::Label>(Node()))
            {
                return label->GetColor();
            }

            if (const auto bar = std::dynamic_pointer_cast<honto::ProgressBar>(Node()))
            {
                return bar->GetFillColor();
            }

            return RGBA(255, 255, 255);
        }

        Vec2 Velocity() const
        {
            if (m_State == nullptr)
            {
                return {};
            }

            return m_State->velocity;
        }

        bool IsOnGround() const
        {
            return m_State != nullptr && m_State->onGround;
        }

        const Actor& At(float x, float y) const
        {
            if (const auto node = Node())
            {
                node->SetPosition(x, y);
            }

            return *this;
        }

        const Actor& At(const Vec2& position) const
        {
            if (const auto node = Node())
            {
                node->SetPosition(position);
            }

            return *this;
        }

        const Actor& Move(float x, float y) const
        {
            return Move({ x, y });
        }

        const Actor& Move(const Vec2& delta) const
        {
            return ApplyMovement(delta);
        }

        const Actor& Size(float width, float height) const
        {
            if (const auto node = Node())
            {
                node->SetContentSize(width, height);
            }

            return *this;
        }

        const Actor& Size(const Vec2& size) const
        {
            if (const auto node = Node())
            {
                node->SetContentSize(size);
            }

            return *this;
        }

        const Actor& Scale(float uniformScale) const
        {
            if (const auto node = Node())
            {
                node->SetScale(uniformScale);
            }

            return *this;
        }

        const Actor& Scale(float x, float y) const
        {
            if (const auto node = Node())
            {
                node->SetScale({ x, y });
            }

            return *this;
        }

        const Actor& Scale(const Vec2& scale) const
        {
            if (const auto node = Node())
            {
                node->SetScale(scale);
            }

            return *this;
        }

        const Actor& Layer(int zOrder) const
        {
            if (const auto node = Node())
            {
                node->SetLocalZOrder(zOrder);
            }

            return *this;
        }

        const Actor& Show(bool visible = true) const
        {
            if (const auto node = Node())
            {
                node->SetVisible(visible);
            }

            return *this;
        }

        const Actor& Hide() const
        {
            return Show(false);
        }

        const Actor& Paint(Color color) const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                sprite->SetColor(color);
            }
            else if (const auto layer = std::dynamic_pointer_cast<honto::LayerColor>(Node()))
            {
                layer->SetColor(color);
            }
            else if (const auto frame = std::dynamic_pointer_cast<detail::FrameNode>(Node()))
            {
                frame->SetColor(color);
            }
            else if (const auto label = std::dynamic_pointer_cast<honto::Label>(Node()))
            {
                label->SetColor(color);
            }
            else if (const auto bar = std::dynamic_pointer_cast<honto::ProgressBar>(Node()))
            {
                bar->SetFillColor(color);
            }

            return *this;
        }

        const Actor& Paint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) const
        {
            return Paint(RGBA(r, g, b, a));
        }

        const Actor& UseTexture(const std::shared_ptr<Texture>& texture) const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                sprite->SetTexture(texture);
            }

            return *this;
        }

        const Actor& UseTextureRegion(int x, int y, int width, int height) const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                sprite->SetTextureRegion({ x, y, width, height });
            }

            return *this;
        }

        const Actor& ClearTextureRegion() const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                sprite->ClearTextureRegion();
            }

            return *this;
        }

        const Actor& UseTextureFrame(int frameIndex, int frameWidth, int frameHeight, int columns = 0) const
        {
            if (const auto sprite = std::dynamic_pointer_cast<honto::Sprite>(Node()))
            {
                sprite->SetTextureFrame(frameIndex, frameWidth, frameHeight, columns);
            }

            return *this;
        }

        const Actor& Thickness(int thickness) const
        {
            if (const auto frame = std::dynamic_pointer_cast<detail::FrameNode>(Node()))
            {
                frame->SetThickness(thickness);
            }

            return *this;
        }

        const Actor& TextValue(const std::string& text) const
        {
            if (const auto label = std::dynamic_pointer_cast<honto::Label>(Node()))
            {
                label->SetText(text);
            }

            return *this;
        }

        const Actor& TextScale(int glyphScale) const
        {
            if (const auto label = std::dynamic_pointer_cast<honto::Label>(Node()))
            {
                label->SetGlyphScale(glyphScale);
            }

            return *this;
        }

        const Actor& UseCamera(bool useCamera) const
        {
            if (const auto label = std::dynamic_pointer_cast<honto::Label>(Node()))
            {
                label->SetUseCamera(useCamera);
            }
            else if (const auto bar = std::dynamic_pointer_cast<honto::ProgressBar>(Node()))
            {
                bar->SetUseCamera(useCamera);
            }

            return *this;
        }

        const Actor& BarValue(float value) const
        {
            if (const auto bar = std::dynamic_pointer_cast<honto::ProgressBar>(Node()))
            {
                bar->SetValue(value);
            }

            return *this;
        }

        const Actor& BarColors(Color fill, Color background, Color border) const
        {
            if (const auto bar = std::dynamic_pointer_cast<honto::ProgressBar>(Node()))
            {
                bar->SetFillColor(fill);
                bar->SetBackgroundColor(background);
                bar->SetBorderColor(border);
            }

            return *this;
        }

        const Actor& Velocity(float x, float y) const
        {
            return Velocity(Vec2 { x, y });
        }

        const Actor& Velocity(const Vec2& velocity) const
        {
            if (m_State != nullptr)
            {
                m_State->velocity = velocity;
            }

            return *this;
        }

        const Actor& MoveByVelocity() const
        {
            if (m_State != nullptr)
            {
                m_State->movesByVelocity = true;
            }

            return EnsurePhysics();
        }

        const Actor& UseGravity(float scale = 1.0f) const
        {
            if (m_State != nullptr)
            {
                m_State->usesGravity = true;
                m_State->gravityScale = scale;
            }

            return EnsurePhysics();
        }

        const Actor& GroundAt(float y, float bounce = 0.0f) const
        {
            if (m_State != nullptr)
            {
                m_State->hasGround = true;
                m_State->groundY = y;
                m_State->groundBounce = std::clamp(bounce, 0.0f, 1.0f);
            }

            return EnsurePhysics();
        }

        const Actor& Jump(float power) const
        {
            if (m_State != nullptr)
            {
                m_State->velocity.y = -std::abs(power);
                m_State->onGround = false;
            }

            return *this;
        }

        const Actor& MoveWithArrows(float speed, bool alsoUseWASD = true) const
        {
            return OnUpdate(
                [speed, alsoUseWASD](Actor& self, float deltaTime)
                {
                    Vec2 move {};

                    if (Pressing(Key::Left) || (alsoUseWASD && Pressing(Key::A)))
                    {
                        move.x -= 1.0f;
                    }

                    if (Pressing(Key::Right) || (alsoUseWASD && Pressing(Key::D)))
                    {
                        move.x += 1.0f;
                    }

                    if (Pressing(Key::Up) || (alsoUseWASD && Pressing(Key::W)))
                    {
                        move.y -= 1.0f;
                    }

                    if (Pressing(Key::Down) || (alsoUseWASD && Pressing(Key::S)))
                    {
                        move.y += 1.0f;
                    }

                    self.Move(move * (speed * deltaTime));
                }
            );
        }

        const Actor& MoveLeftRight(float speed, bool alsoUseAD = true) const
        {
            return OnUpdate(
                [speed, alsoUseAD](Actor& self, float deltaTime)
                {
                    float direction = 0.0f;

                    if (Pressing(Key::Left) || (alsoUseAD && Pressing(Key::A)))
                    {
                        direction -= 1.0f;
                    }

                    if (Pressing(Key::Right) || (alsoUseAD && Pressing(Key::D)))
                    {
                        direction += 1.0f;
                    }

                    self.Move(direction * speed * deltaTime, 0.0f);
                }
            );
        }

        const Actor& JumpWhenPressed(Key key, float power) const
        {
            return OnUpdate(
                [key, power](Actor& self, float)
                {
                    if (Pressed(key) && self.IsOnGround())
                    {
                        self.Jump(power);
                    }
                }
            );
        }

        const Actor& KeepInside(float left, float top, float right, float bottom) const
        {
            return OnUpdate(
                [left, top, right, bottom](Actor& self, float)
                {
                    Vec2 position = self.Position();
                    position.x = std::clamp(position.x, left, right);
                    position.y = std::clamp(position.y, top, bottom);
                    self.At(position);
                }
            );
        }

        const Actor& BounceInside(float left, float top, float right, float bottom) const
        {
            return OnUpdate(
                [left, top, right, bottom](Actor& self, float)
                {
                    Vec2 position = self.Position();
                    Vec2 velocity = self.Velocity();

                    if (position.x <= left || position.x >= right)
                    {
                        velocity.x *= -1.0f;
                        position.x = std::clamp(position.x, left, right);
                    }

                    if (position.y <= top || position.y >= bottom)
                    {
                        velocity.y *= -1.0f;
                        position.y = std::clamp(position.y, top, bottom);
                    }

                    self.At(position);
                    self.Velocity(velocity);
                }
            );
        }

        const Actor& OnUpdate(std::function<void(Actor&, float)> fn) const
        {
            if (fn == nullptr)
            {
                return *this;
            }

            if (const auto stage = LockStage())
            {
                if (stage->scene != nullptr)
                {
                    stage->scene->EveryFrame();
                }

                stage->updates.push_back(
                    [state = m_State, fn = std::move(fn)](float deltaTime)
                    {
                        if (state == nullptr || state->node == nullptr)
                        {
                            return;
                        }

                        Actor actor(state);
                        fn(actor, deltaTime);
                    }
                );
            }

            return *this;
        }

        bool Touching(const Actor& other) const
        {
            if (!(*this) || !other)
            {
                return false;
            }

            const Vec2 myPosition = Position();
            const Vec2 mySize = Size();
            const Vec2 otherPosition = other.Position();
            const Vec2 otherSize = other.Size();

            return myPosition.x < otherPosition.x + otherSize.x &&
                   myPosition.x + mySize.x > otherPosition.x &&
                   myPosition.y < otherPosition.y + otherSize.y &&
                   myPosition.y + mySize.y > otherPosition.y;
        }

        bool TouchingMap(const TileMapActor& map) const;

        const Actor& CollideWithMap(const TileMapActor& map) const;

        std::shared_ptr<honto::Node> Share() const
        {
            return Node();
        }

        Animation Animate() const;
        FrameAnimation AnimateFrames() const;

        const Actor& hontoAt(float x, float y) const
        {
            return At(x, y);
        }

        const Actor& hontoAt(const Vec2& position) const
        {
            return At(position);
        }

        const Actor& hontoMove(float x, float y) const
        {
            return Move(x, y);
        }

        const Actor& hontoMove(const Vec2& delta) const
        {
            return Move(delta);
        }

        const Actor& hontoSize(float width, float height) const
        {
            return Size(width, height);
        }

        const Actor& hontoSize(const Vec2& size) const
        {
            return Size(size);
        }

        const Actor& hontoScale(float uniformScale) const
        {
            return Scale(uniformScale);
        }

        const Actor& hontoScale(float x, float y) const
        {
            return Scale(x, y);
        }

        const Actor& hontoScale(const Vec2& scale) const
        {
            return Scale(scale);
        }

        const Actor& hontoLayer(int zOrder) const
        {
            return Layer(zOrder);
        }

        const Actor& hontoShow(bool visible = true) const
        {
            return Show(visible);
        }

        const Actor& hontoHide() const
        {
            return Hide();
        }

        const Actor& hontoPaint(Color color) const
        {
            return Paint(color);
        }

        const Actor& hontoPaint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) const
        {
            return Paint(r, g, b, a);
        }

        const Actor& hontoUseTexture(const std::shared_ptr<Texture>& texture) const
        {
            return UseTexture(texture);
        }

        const Actor& hontoTextValue(const std::string& text) const
        {
            return TextValue(text);
        }

        const Actor& hontoTextScale(int glyphScale) const
        {
            return TextScale(glyphScale);
        }

        const Actor& hontoUseCamera(bool useCamera) const
        {
            return UseCamera(useCamera);
        }

        const Actor& hontoBarValue(float value) const
        {
            return BarValue(value);
        }

        const Actor& hontoBarColors(Color fill, Color background, Color border) const
        {
            return BarColors(fill, background, border);
        }

        const Actor& hontoUseTextureRegion(int x, int y, int width, int height) const
        {
            return UseTextureRegion(x, y, width, height);
        }

        const Actor& hontoClearTextureRegion() const
        {
            return ClearTextureRegion();
        }

        const Actor& hontoUseTextureFrame(int frameIndex, int frameWidth, int frameHeight, int columns = 0) const
        {
            return UseTextureFrame(frameIndex, frameWidth, frameHeight, columns);
        }

        const Actor& hontoThickness(int thickness) const
        {
            return Thickness(thickness);
        }

        const Actor& hontoVelocity(float x, float y) const
        {
            return Velocity(x, y);
        }

        const Actor& hontoVelocity(const Vec2& velocity) const
        {
            return Velocity(velocity);
        }

        const Actor& hontoMoveByVelocity() const
        {
            return MoveByVelocity();
        }

        const Actor& hontoUseGravity(float scale = 1.0f) const
        {
            return UseGravity(scale);
        }

        const Actor& hontoGroundAt(float y, float bounce = 0.0f) const
        {
            return GroundAt(y, bounce);
        }

        const Actor& hontoJump(float power) const
        {
            return Jump(power);
        }

        const Actor& hontoMoveWithArrows(float speed, bool alsoUseWASD = true) const
        {
            return MoveWithArrows(speed, alsoUseWASD);
        }

        const Actor& hontoMoveLeftRight(float speed, bool alsoUseAD = true) const
        {
            return MoveLeftRight(speed, alsoUseAD);
        }

        const Actor& hontoJumpWhenPressed(Key key, float power) const
        {
            return JumpWhenPressed(key, power);
        }

        const Actor& hontoKeepInside(float left, float top, float right, float bottom) const
        {
            return KeepInside(left, top, right, bottom);
        }

        const Actor& hontoBounceInside(float left, float top, float right, float bottom) const
        {
            return BounceInside(left, top, right, bottom);
        }

        const Actor& hontoOnUpdate(std::function<void(Actor&, float)> fn) const
        {
            return OnUpdate(std::move(fn));
        }

        bool hontoTouching(const Actor& other) const
        {
            return Touching(other);
        }

        bool hontoTouchingMap(const TileMapActor& map) const
        {
            return TouchingMap(map);
        }

        const Actor& hontoCollideWithMap(const TileMapActor& map) const
        {
            return CollideWithMap(map);
        }

        bool hontoIsOnGround() const
        {
            return IsOnGround();
        }

        Animation hontoAnimate() const;
        FrameAnimation hontoAnimateFrames() const;

    private:
        std::shared_ptr<honto::Node> Node() const
        {
            if (m_State == nullptr)
            {
                return nullptr;
            }

            return m_State->node;
        }

        std::shared_ptr<detail::StageState> LockStage() const
        {
            if (m_State == nullptr)
            {
                return nullptr;
            }

            return m_State->stage.lock();
        }

        std::shared_ptr<honto::TileMap> LockCollisionMap() const
        {
            if (m_State == nullptr)
            {
                return nullptr;
            }

            return m_State->collisionMap.lock();
        }

        const Actor& ApplyMovement(const Vec2& delta) const
        {
            if (const auto node = Node())
            {
                if (const auto tileMap = LockCollisionMap())
                {
                    Vec2 position = node->GetPosition();
                    Vec2 velocity = m_State != nullptr ? m_State->velocity : Vec2 {};
                    bool collidedX = false;
                    bool collidedY = false;
                    bool onGround = m_State != nullptr ? m_State->onGround : false;

                    tileMap->ResolveMovement(position, node->GetContentSize(), velocity, delta, collidedX, collidedY, onGround);
                    node->SetPosition(position);

                    if (m_State != nullptr)
                    {
                        m_State->velocity = velocity;
                        m_State->onGround = (delta.y == 0.0f) ? m_State->onGround || onGround : onGround;
                    }
                }
                else
                {
                    node->SetPosition(node->GetPosition() + delta);

                    if (m_State != nullptr && delta.y < 0.0f)
                    {
                        m_State->onGround = false;
                    }
                }
            }

            return *this;
        }

        const Actor& EnsurePhysics() const
        {
            if (m_State == nullptr || m_State->physicsAttached)
            {
                return *this;
            }

            m_State->physicsAttached = true;
            return OnUpdate(
                [](Actor& self, float deltaTime)
                {
                    if (self.m_State == nullptr)
                    {
                        return;
                    }

                    if (self.m_State->usesGravity)
                    {
                        if (const auto stage = self.LockStage())
                        {
                            self.m_State->velocity += stage->gravity * (self.m_State->gravityScale * deltaTime);
                        }
                    }

                    if (self.m_State->movesByVelocity || self.m_State->usesGravity)
                    {
                        self.ApplyMovement(self.m_State->velocity * deltaTime);
                    }

                    if (self.m_State->hasGround && self.LockCollisionMap() == nullptr)
                    {
                        Vec2 position = self.Position();
                        if (position.y >= self.m_State->groundY)
                        {
                            position.y = self.m_State->groundY;

                            if (self.m_State->velocity.y > 0.0f)
                            {
                                self.m_State->velocity.y = -self.m_State->velocity.y * self.m_State->groundBounce;
                                if (std::abs(self.m_State->velocity.y) < 8.0f || self.m_State->groundBounce <= 0.0f)
                                {
                                    self.m_State->velocity.y = 0.0f;
                                }
                            }

                            self.m_State->onGround = true;
                            self.At(position);
                        }
                        else
                        {
                            self.m_State->onGround = false;
                        }
                    }
                }
            );
        }

        std::shared_ptr<detail::ActorState> m_State;
    };

    class Animation
    {
    public:
        explicit Animation(Actor actor)
            : m_Actor(std::move(actor))
        {
        }

        Animation& MoveTo(float x, float y)
        {
            m_HasMove = true;
            m_TargetPosition = { x, y };
            return *this;
        }

        Animation& MoveTo(const Vec2& position)
        {
            m_HasMove = true;
            m_TargetPosition = position;
            return *this;
        }

        Animation& ScaleTo(float uniformScale)
        {
            return ScaleTo(uniformScale, uniformScale);
        }

        Animation& ScaleTo(float x, float y)
        {
            m_HasScale = true;
            m_TargetScale = { x, y };
            return *this;
        }

        Animation& ScaleTo(const Vec2& scale)
        {
            m_HasScale = true;
            m_TargetScale = scale;
            return *this;
        }

        Animation& PaintTo(Color color)
        {
            m_HasColor = true;
            m_TargetColor = color;
            return *this;
        }

        Animation& PaintTo(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return PaintTo(RGBA(r, g, b, a));
        }

        Animation& In(float seconds)
        {
            m_Duration = std::max(0.01f, seconds);
            return *this;
        }

        Animation& Delay(float seconds)
        {
            m_Delay = std::max(0.0f, seconds);
            return *this;
        }

        Animation& Loop(bool enabled = true)
        {
            m_Loop = enabled;
            return *this;
        }

        Animation& PingPong(bool enabled = true)
        {
            m_PingPong = enabled;
            return *this;
        }

        Animation& hontoMoveTo(float x, float y)
        {
            return MoveTo(x, y);
        }

        Animation& hontoMoveTo(const Vec2& position)
        {
            return MoveTo(position);
        }

        Animation& hontoScaleTo(float uniformScale)
        {
            return ScaleTo(uniformScale);
        }

        Animation& hontoScaleTo(float x, float y)
        {
            return ScaleTo(x, y);
        }

        Animation& hontoScaleTo(const Vec2& scale)
        {
            return ScaleTo(scale);
        }

        Animation& hontoPaintTo(Color color)
        {
            return PaintTo(color);
        }

        Animation& hontoPaintTo(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return PaintTo(r, g, b, a);
        }

        Animation& hontoIn(float seconds)
        {
            return In(seconds);
        }

        Animation& hontoDelay(float seconds)
        {
            return Delay(seconds);
        }

        Animation& hontoLoop(bool enabled = true)
        {
            return Loop(enabled);
        }

        Animation& hontoPingPong(bool enabled = true)
        {
            return PingPong(enabled);
        }

        const Actor& Play() const
        {
            if (!m_Actor || (!m_HasMove && !m_HasScale && !m_HasColor))
            {
                return m_Actor;
            }

            const Vec2 startPosition = m_Actor.Position();
            const Vec2 startScale = m_Actor.ScaleValue();
            const Color startColor = m_Actor.Tint();
            const float duration = std::max(0.01f, m_Duration);
            const float delay = std::max(0.0f, m_Delay);
            const bool loops = m_Loop;
            const bool pingPong = m_PingPong;

            return m_Actor.OnUpdate(
                [
                    startPosition,
                    targetPosition = m_TargetPosition,
                    startScale,
                    targetScale = m_TargetScale,
                    startColor,
                    targetColor = m_TargetColor,
                    hasMove = m_HasMove,
                    hasScale = m_HasScale,
                    hasColor = m_HasColor,
                    duration,
                    delay,
                    loops,
                    pingPong,
                    elapsed = 0.0f,
                    finished = false
                ](Actor& self, float deltaTime) mutable
                {
                    if (finished && !loops)
                    {
                        return;
                    }

                    elapsed += deltaTime;
                    if (elapsed < delay)
                    {
                        return;
                    }

                    const float cycleDuration = pingPong ? (duration * 2.0f) : duration;
                    float localTime = elapsed - delay;

                    if (loops)
                    {
                        localTime = std::fmod(localTime, cycleDuration);
                        if (localTime < 0.0f)
                        {
                            localTime += cycleDuration;
                        }
                    }
                    else if (localTime >= cycleDuration)
                    {
                        localTime = cycleDuration;
                        finished = true;
                    }

                    float t = localTime / duration;
                    if (pingPong && localTime > duration)
                    {
                        t = 1.0f - ((localTime - duration) / duration);
                    }

                    t = std::clamp(t, 0.0f, 1.0f);

                    if (hasMove)
                    {
                        self.At(detail::Lerp(startPosition, targetPosition, t));
                    }

                    if (hasScale)
                    {
                        self.Scale(detail::Lerp(startScale, targetScale, t));
                    }

                    if (hasColor)
                    {
                        self.Paint(detail::Lerp(startColor, targetColor, t));
                    }
                }
            );
        }

        const Actor& hontoPlay() const
        {
            return Play();
        }

    private:
        Actor m_Actor;
        float m_Duration = 0.35f;
        float m_Delay = 0.0f;
        bool m_Loop = false;
        bool m_PingPong = false;
        bool m_HasMove = false;
        bool m_HasScale = false;
        bool m_HasColor = false;
        Vec2 m_TargetPosition {};
        Vec2 m_TargetScale { 1.0f, 1.0f };
        Color m_TargetColor { 255, 255, 255, 255 };
    };

    inline Animation Actor::Animate() const
    {
        return Animation(*this);
    }

    inline Animation Actor::hontoAnimate() const
    {
        return Animate();
    }

    class FrameAnimation
    {
    public:
        explicit FrameAnimation(Actor actor)
            : m_Actor(std::move(actor))
        {
        }

        FrameAnimation& Texture(const std::shared_ptr<honto::Texture>& texture)
        {
            m_Texture = texture;
            return *this;
        }

        FrameAnimation& Texture(const std::string& path)
        {
            return Texture(LoadTexture(path));
        }

        FrameAnimation& FrameSize(int width, int height)
        {
            m_FrameWidth = std::max(1, width);
            m_FrameHeight = std::max(1, height);
            return *this;
        }

        FrameAnimation& Frames(const std::vector<int>& frames)
        {
            m_Frames = frames;
            return *this;
        }

        FrameAnimation& Frames(std::initializer_list<int> frames)
        {
            m_Frames.assign(frames.begin(), frames.end());
            return *this;
        }

        FrameAnimation& Range(int startFrame, int count)
        {
            m_Frames.clear();

            for (int index = 0; index < count; ++index)
            {
                m_Frames.push_back(startFrame + index);
            }

            return *this;
        }

        FrameAnimation& Columns(int columns)
        {
            m_Columns = std::max(0, columns);
            return *this;
        }

        FrameAnimation& FPS(float framesPerSecond)
        {
            m_Fps = std::max(1.0f, framesPerSecond);
            return *this;
        }

        FrameAnimation& Loop(bool enabled = true)
        {
            m_Loop = enabled;
            return *this;
        }

        FrameAnimation& PingPong(bool enabled = true)
        {
            m_PingPong = enabled;
            return *this;
        }

        FrameAnimation& hontoTexture(const std::shared_ptr<honto::Texture>& texture)
        {
            return Texture(texture);
        }

        FrameAnimation& hontoTexture(const std::string& path)
        {
            return Texture(path);
        }

        FrameAnimation& hontoFrameSize(int width, int height)
        {
            return FrameSize(width, height);
        }

        FrameAnimation& hontoFrames(const std::vector<int>& frames)
        {
            return Frames(frames);
        }

        FrameAnimation& hontoFrames(std::initializer_list<int> frames)
        {
            return Frames(frames);
        }

        FrameAnimation& hontoRange(int startFrame, int count)
        {
            return Range(startFrame, count);
        }

        FrameAnimation& hontoColumns(int columns)
        {
            return Columns(columns);
        }

        FrameAnimation& hontoFPS(float framesPerSecond)
        {
            return FPS(framesPerSecond);
        }

        FrameAnimation& hontoLoop(bool enabled = true)
        {
            return Loop(enabled);
        }

        FrameAnimation& hontoPingPong(bool enabled = true)
        {
            return PingPong(enabled);
        }

        const Actor& Play() const
        {
            if (!m_Actor || m_FrameWidth <= 0 || m_FrameHeight <= 0)
            {
                return m_Actor;
            }

            if (m_Texture != nullptr)
            {
                m_Actor.UseTexture(m_Texture);
            }

            const std::vector<int> sequence = m_Frames;
            if (sequence.empty())
            {
                return m_Actor;
            }

            m_Actor.UseTextureFrame(sequence.front(), m_FrameWidth, m_FrameHeight, m_Columns);

            return m_Actor.OnUpdate(
                [
                    sequence,
                    frameWidth = m_FrameWidth,
                    frameHeight = m_FrameHeight,
                    columns = m_Columns,
                    fps = m_Fps,
                    loops = m_Loop,
                    pingPong = m_PingPong,
                    elapsed = 0.0f,
                    currentIndex = -1,
                    finished = false
                ](Actor& self, float deltaTime) mutable
                {
                    if (finished && !loops)
                    {
                        return;
                    }

                    elapsed += deltaTime;
                    const int sequenceCount = static_cast<int>(sequence.size());
                    const int cycleCount = pingPong && sequenceCount > 1 ? ((sequenceCount * 2) - 2) : sequenceCount;

                    if (cycleCount <= 0)
                    {
                        return;
                    }

                    int cycleIndex = static_cast<int>(std::floor(elapsed * fps));
                    if (loops)
                    {
                        cycleIndex %= cycleCount;
                        if (cycleIndex < 0)
                        {
                            cycleIndex += cycleCount;
                        }
                    }
                    else if (cycleIndex >= cycleCount)
                    {
                        cycleIndex = cycleCount - 1;
                        finished = true;
                    }

                    int sequenceIndex = cycleIndex;
                    if (pingPong && sequenceCount > 1 && cycleIndex >= sequenceCount)
                    {
                        sequenceIndex = (sequenceCount - 2) - (cycleIndex - sequenceCount);
                    }

                    sequenceIndex = std::clamp(sequenceIndex, 0, sequenceCount - 1);
                    if (sequenceIndex == currentIndex)
                    {
                        return;
                    }

                    currentIndex = sequenceIndex;
                    self.UseTextureFrame(sequence[static_cast<std::size_t>(sequenceIndex)], frameWidth, frameHeight, columns);
                }
            );
        }

        const Actor& hontoPlay() const
        {
            return Play();
        }

    private:
        Actor m_Actor;
        std::shared_ptr<honto::Texture> m_Texture;
        std::vector<int> m_Frames;
        int m_FrameWidth = 16;
        int m_FrameHeight = 16;
        int m_Columns = 0;
        float m_Fps = 8.0f;
        bool m_Loop = true;
        bool m_PingPong = false;
    };

    inline FrameAnimation Actor::AnimateFrames() const
    {
        return FrameAnimation(*this);
    }

    inline FrameAnimation Actor::hontoAnimateFrames() const
    {
        return AnimateFrames();
    }

    class TileMapActor : public Actor
    {
    public:
        TileMapActor() = default;
        explicit TileMapActor(const Actor& actor)
            : Actor(actor)
        {
        }

        TileMapActor& Map(const std::vector<std::string>& map)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetMap(map);
            }

            return *this;
        }

        TileMapActor& TileSize(float width, float height)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTileSize(width, height);
            }

            return *this;
        }

        TileMapActor& Tile(char tile, Color color, bool solid = false, bool visible = true)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTile(tile, color, solid, visible);
            }

            return *this;
        }

        TileMapActor& TileTexture(char tile, const std::shared_ptr<Texture>& texture, Color tint = RGBA(255, 255, 255), bool solid = false, bool visible = true)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTileTexture(tile, texture, tint, solid, visible);
            }

            return *this;
        }

        TileMapActor& TileTextureRegion(
            char tile,
            const std::shared_ptr<Texture>& texture,
            int x,
            int y,
            int width,
            int height,
            Color tint = RGBA(255, 255, 255),
            bool solid = false,
            bool visible = true
        )
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTileTextureRegion(tile, texture, { x, y, width, height }, tint, solid, visible);
            }

            return *this;
        }

        TileMapActor& TileSolid(char tile, bool solid)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTileSolid(tile, solid);
            }

            return *this;
        }

        TileMapActor& TileVisible(char tile, bool visible)
        {
            if (const auto tileMap = View())
            {
                tileMap->SetTileVisible(tile, visible);
            }

            return *this;
        }

        bool Collides(const Actor& actor) const
        {
            if (const auto tileMap = View())
            {
                return tileMap->CollidesRect(actor.Position(), actor.Size());
            }

            return false;
        }

        bool IsSolidAt(const Vec2& point) const
        {
            if (const auto tileMap = View())
            {
                return tileMap->IsSolidAtWorldPoint(point);
            }

            return false;
        }

        TileMapActor& hontoMap(const std::vector<std::string>& map)
        {
            return Map(map);
        }

        TileMapActor& hontoTileSize(float width, float height)
        {
            return TileSize(width, height);
        }

        TileMapActor& hontoTile(char tile, Color color, bool solid = false, bool visible = true)
        {
            return Tile(tile, color, solid, visible);
        }

        TileMapActor& hontoTileTexture(char tile, const std::shared_ptr<Texture>& texture, Color tint = RGBA(255, 255, 255), bool solid = false, bool visible = true)
        {
            return TileTexture(tile, texture, tint, solid, visible);
        }

        TileMapActor& hontoTileTextureRegion(
            char tile,
            const std::shared_ptr<Texture>& texture,
            int x,
            int y,
            int width,
            int height,
            Color tint = RGBA(255, 255, 255),
            bool solid = false,
            bool visible = true
        )
        {
            return TileTextureRegion(tile, texture, x, y, width, height, tint, solid, visible);
        }

        TileMapActor& hontoTileSolid(char tile, bool solid)
        {
            return TileSolid(tile, solid);
        }

        TileMapActor& hontoTileVisible(char tile, bool visible)
        {
            return TileVisible(tile, visible);
        }

        bool hontoCollides(const Actor& actor) const
        {
            return Collides(actor);
        }

        bool hontoIsSolidAt(const Vec2& point) const
        {
            return IsSolidAt(point);
        }

    private:
        std::shared_ptr<honto::TileMap> View() const
        {
            return std::dynamic_pointer_cast<honto::TileMap>(Share());
        }
    };

    inline bool Actor::TouchingMap(const TileMapActor& map) const
    {
        const auto tileMap = std::dynamic_pointer_cast<honto::TileMap>(map.Share());
        if (!(*this) || tileMap == nullptr)
        {
            return false;
        }

        return tileMap->CollidesRect(Position(), Size());
    }

    inline const Actor& Actor::CollideWithMap(const TileMapActor& map) const
    {
        if (m_State != nullptr)
        {
            m_State->collisionMap = std::dynamic_pointer_cast<honto::TileMap>(map.Share());
        }

        return EnsurePhysics();
    }

    class RaycastActor : public Actor
    {
    public:
        RaycastActor() = default;
        explicit RaycastActor(const Actor& actor)
            : Actor(actor)
        {
        }

        RaycastActor& hontoMap(const std::vector<std::string>& map)
        {
            if (const auto view = View())
            {
                view->SetMap(map);
            }

            return *this;
        }

        RaycastActor& hontoPlayer(float x, float y, float angleRadians)
        {
            if (const auto view = View())
            {
                view->SetPlayer({ x, y }, angleRadians);
            }

            return *this;
        }

        RaycastActor& hontoPlayer(const Vec2& position, float angleRadians)
        {
            if (const auto view = View())
            {
                view->SetPlayer(position, angleRadians);
            }

            return *this;
        }

        RaycastActor& hontoViewDegrees(float degrees)
        {
            if (const auto view = View())
            {
                view->SetFieldOfView(degrees * 0.0174532925f);
            }

            return *this;
        }

        RaycastActor& hontoMoveSpeed(float speed)
        {
            if (const auto view = View())
            {
                view->SetMoveSpeed(speed);
            }

            return *this;
        }

        RaycastActor& hontoTurnSpeed(float speed)
        {
            if (const auto view = View())
            {
                view->SetTurnSpeed(speed);
            }

            return *this;
        }

        RaycastActor& hontoFloor(Color color)
        {
            if (const auto view = View())
            {
                view->SetFloorColor(color);
            }

            return *this;
        }

        RaycastActor& hontoCeiling(Color color)
        {
            if (const auto view = View())
            {
                view->SetCeilingColor(color);
            }

            return *this;
        }

        RaycastActor& hontoWall(char cell, Color color)
        {
            if (const auto view = View())
            {
                view->SetWallColor(cell, color);
            }

            return *this;
        }

        RaycastActor& hontoWallTexture(char cell, const std::shared_ptr<Texture>& texture)
        {
            if (const auto view = View())
            {
                view->SetWallTexture(cell, texture);
            }

            return *this;
        }

        RaycastActor& hontoDoomControls(float moveSpeed = 3.0f, float turnSpeed = 2.0f)
        {
            if (const auto view = View())
            {
                view->SetMoveSpeed(moveSpeed);
                view->SetTurnSpeed(turnSpeed);
                view->EnableDoomControls(true);
            }

            return *this;
        }

        RaycastActor& hontoMiniMap(bool enabled = true, float scale = 8.0f)
        {
            if (const auto view = View())
            {
                view->SetMiniMapEnabled(enabled, scale);
            }

            return *this;
        }

        RaycastActor& hontoMaxDistance(float distance)
        {
            if (const auto view = View())
            {
                view->SetMaxDistance(distance);
            }

            return *this;
        }

    private:
        std::shared_ptr<honto::RaycastView> View() const
        {
            return std::dynamic_pointer_cast<honto::RaycastView>(Share());
        }
    };

    class Stage
    {
    public:
        Stage(HonTo::Scene* scene, std::shared_ptr<detail::StageState> state)
            : m_Scene(scene),
              m_State(std::move(state))
        {
            if (m_State != nullptr)
            {
                m_State->scene = m_Scene;
            }
        }

        Vec2 VisibleSize() const
        {
            return HonTo::VisibleSize();
        }

        Vec2 hontoVisibleSize() const
        {
            return VisibleSize();
        }

        Stage& CameraAt(const Vec2& position, float zoom = 1.0f)
        {
            if (auto* renderer = honto::Director::Get().GetRenderer())
            {
                renderer->SetCamera(position, zoom);
            }

            return *this;
        }

        Stage& CameraAt(float x, float y, float zoom = 1.0f)
        {
            return CameraAt({ x, y }, zoom);
        }

        Stage& CameraReset()
        {
            if (auto* renderer = honto::Director::Get().GetRenderer())
            {
                renderer->ResetCamera();
            }

            return *this;
        }

        void CameraFollow(const Actor& actor, float zoom = 1.0f)
        {
            EveryFrame(
                [actor, zoom](float)
                {
                    if (!actor)
                    {
                        return;
                    }

                    if (auto* renderer = honto::Director::Get().GetRenderer())
                    {
                        const Vec2 visible = HonTo::VisibleSize();
                        const Vec2 cameraHalf = visible / (2.0f * std::max(0.01f, zoom));
                        const Vec2 target = actor.Position() + (actor.Size() * 0.5f) - cameraHalf;
                        renderer->SetCamera(target, zoom);
                    }
                }
            );
        }

        Stage& hontoCameraAt(const Vec2& position, float zoom = 1.0f)
        {
            return CameraAt(position, zoom);
        }

        Stage& hontoCameraAt(float x, float y, float zoom = 1.0f)
        {
            return CameraAt(x, y, zoom);
        }

        Stage& hontoCameraReset()
        {
            return CameraReset();
        }

        void hontoCameraFollow(const Actor& actor, float zoom = 1.0f)
        {
            CameraFollow(actor, zoom);
        }

        Stage& Gravity(float x, float y)
        {
            return Gravity({ x, y });
        }

        Stage& Gravity(const Vec2& gravity)
        {
            if (m_State != nullptr)
            {
                m_State->gravity = gravity;
            }

            return *this;
        }

        Stage& hontoGravity(float x, float y)
        {
            return Gravity(x, y);
        }

        Stage& hontoGravity(const Vec2& gravity)
        {
            return Gravity(gravity);
        }

        Vec2 GravityValue() const
        {
            if (m_State == nullptr)
            {
                return {};
            }

            return m_State->gravity;
        }

        Actor Background(Color color)
        {
            return Fill(VisibleSize(), color);
        }

        Actor Background(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Background(RGBA(r, g, b, a));
        }

        Actor hontoBackground(Color color)
        {
            return Background(color);
        }

        Actor hontoBackground(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        {
            return Background(r, g, b, a);
        }

        Actor Box(float width, float height, Color color = RGBA(255, 255, 255))
        {
            return Box({}, width, height, color);
        }

        Actor Box(const std::string& name, float width, float height, Color color = RGBA(255, 255, 255))
        {
            auto actor = CreateActor(name, honto::Sprite::CreateSolid({ width, height }, color));
            actor.Size(width, height).Paint(color);
            return actor;
        }

        Actor hontoBox(float width, float height, Color color = RGBA(255, 255, 255))
        {
            return Box(width, height, color);
        }

        Actor hontoBox(const std::string& name, float width, float height, Color color = RGBA(255, 255, 255))
        {
            return Box(name, width, height, color);
        }

        Actor Image(const std::string& name, const std::shared_ptr<Texture>& texture, float width, float height, Color tint = RGBA(255, 255, 255))
        {
            auto actor = CreateActor(name, honto::Sprite::CreateTextured({ width, height }, texture, tint));
            actor.Size(width, height).Paint(tint).UseTexture(texture);
            return actor;
        }

        Actor Image(const std::string& name, const std::string& path, float width, float height, Color tint = RGBA(255, 255, 255))
        {
            return Image(name, LoadTexture(path), width, height, tint);
        }

        Actor Checker(
            const std::string& name,
            float width,
            float height,
            Color a,
            Color b,
            int cellSize = 8,
            Color tint = RGBA(255, 255, 255)
        )
        {
            return Image(name, CheckerTexture(static_cast<int>(width), static_cast<int>(height), a, b, cellSize), width, height, tint);
        }

        Actor hontoImage(const std::string& name, const std::shared_ptr<Texture>& texture, float width, float height, Color tint = RGBA(255, 255, 255))
        {
            return Image(name, texture, width, height, tint);
        }

        Actor hontoImage(const std::string& name, const std::string& path, float width, float height, Color tint = RGBA(255, 255, 255))
        {
            return Image(name, path, width, height, tint);
        }

        Actor hontoChecker(
            const std::string& name,
            float width,
            float height,
            Color a,
            Color b,
            int cellSize = 8,
            Color tint = RGBA(255, 255, 255)
        )
        {
            return Checker(name, width, height, a, b, cellSize, tint);
        }

        Actor Fill(float width, float height, Color color)
        {
            return Fill({}, width, height, color);
        }

        Actor Fill(const std::string& name, float width, float height, Color color)
        {
            auto actor = CreateActor(name, honto::LayerColor::Create(color, width, height));
            actor.Size(width, height).Paint(color);
            return actor;
        }

        Actor Fill(const Vec2& size, Color color)
        {
            return Fill({}, size.x, size.y, color);
        }

        Actor Fill(const std::string& name, const Vec2& size, Color color)
        {
            return Fill(name, size.x, size.y, color);
        }

        Actor hontoFill(float width, float height, Color color)
        {
            return Fill(width, height, color);
        }

        Actor hontoFill(const std::string& name, float width, float height, Color color)
        {
            return Fill(name, width, height, color);
        }

        Actor hontoFill(const Vec2& size, Color color)
        {
            return Fill(size, color);
        }

        Actor hontoFill(const std::string& name, const Vec2& size, Color color)
        {
            return Fill(name, size, color);
        }

        Actor Outline(float width, float height, Color color, int thickness = 1)
        {
            return Outline({}, width, height, color, thickness);
        }

        Actor Outline(const std::string& name, float width, float height, Color color, int thickness = 1)
        {
            auto frame = std::make_shared<detail::FrameNode>();
            if (frame != nullptr)
            {
                frame->Init();
                frame->SetContentSize(width, height);
                frame->SetColor(color);
                frame->SetThickness(thickness);
            }

            auto actor = CreateActor(name, frame);
            actor.Size(width, height).Paint(color).Thickness(thickness);
            return actor;
        }

        Actor hontoOutline(float width, float height, Color color, int thickness = 1)
        {
            return Outline(width, height, color, thickness);
        }

        Actor hontoOutline(const std::string& name, float width, float height, Color color, int thickness = 1)
        {
            return Outline(name, width, height, color, thickness);
        }

        TileMapActor TileMap(const std::string& name, const std::vector<std::string>& map, float tileWidth, float tileHeight)
        {
            auto tileMap = honto::TileMap::Create(map, tileWidth, tileHeight);
            return TileMapActor(CreateActor(name, tileMap));
        }

        TileMapActor TileMap(const std::string& name, const Level& level)
        {
            return TileMap(name, level.map, level.tileSize.x, level.tileSize.y);
        }

        TileMapActor hontoTileMap(const std::string& name, const std::vector<std::string>& map, float tileWidth, float tileHeight)
        {
            return TileMap(name, map, tileWidth, tileHeight);
        }

        TileMapActor hontoTileMap(const std::string& name, const Level& level)
        {
            return TileMap(name, level);
        }

        Actor Text(
            const std::string& name,
            const std::string& text,
            Color color = RGBA(255, 255, 255),
            int glyphScale = 1,
            bool useCamera = false
        )
        {
            auto actor = CreateActor(name, honto::Label::Create(text, color, glyphScale, useCamera));
            actor.TextValue(text).Paint(color).TextScale(glyphScale).UseCamera(useCamera);
            return actor;
        }

        Actor hontoText(
            const std::string& name,
            const std::string& text,
            Color color = RGBA(255, 255, 255),
            int glyphScale = 1,
            bool useCamera = false
        )
        {
            return Text(name, text, color, glyphScale, useCamera);
        }

        Actor Bar(
            const std::string& name,
            float width,
            float height,
            float value = 1.0f,
            Color fill = RGBA(92, 220, 128),
            Color background = RGBA(18, 24, 36, 180),
            Color border = RGBA(255, 255, 255),
            bool useCamera = false
        )
        {
            auto actor = CreateActor(name, honto::ProgressBar::Create(width, height, value, useCamera));
            actor.Size(width, height).BarValue(value).BarColors(fill, background, border).UseCamera(useCamera);
            return actor;
        }

        Actor hontoBar(
            const std::string& name,
            float width,
            float height,
            float value = 1.0f,
            Color fill = RGBA(92, 220, 128),
            Color background = RGBA(18, 24, 36, 180),
            Color border = RGBA(255, 255, 255),
            bool useCamera = false
        )
        {
            return Bar(name, width, height, value, fill, background, border, useCamera);
        }

        RaycastActor Raycast(const std::string& name, float width, float height)
        {
            auto view = std::make_shared<honto::RaycastView>();
            if (view != nullptr)
            {
                view->Init();
                view->SetContentSize(width, height);
            }

            return RaycastActor(CreateActor(name, view));
        }

        RaycastActor hontoRaycast(const std::string& name, float width, float height)
        {
            return Raycast(name, width, height);
        }

        Actor Find(const std::string& name) const
        {
            if (m_State == nullptr)
            {
                return {};
            }

            const auto found = m_State->namedActors.find(name);
            if (found == m_State->namedActors.end())
            {
                return {};
            }

            return Actor(found->second);
        }

        Actor hontoFind(const std::string& name) const
        {
            return Find(name);
        }

        bool PlaySound(const std::string& path, bool loop = false) const
        {
            return HonTo::PlaySound(path, loop);
        }

        bool PlayAlias(const std::string& alias, bool loop = false) const
        {
            return HonTo::PlayAlias(alias, loop);
        }

        void StopAudio() const
        {
            HonTo::StopAudio();
        }

        void PlayTone(int frequency, int durationMs) const
        {
            HonTo::PlayTone(frequency, durationMs);
        }

        bool hontoPlaySound(const std::string& path, bool loop = false) const
        {
            return PlaySound(path, loop);
        }

        bool hontoPlayAlias(const std::string& alias, bool loop = false) const
        {
            return PlayAlias(alias, loop);
        }

        void hontoStopAudio() const
        {
            StopAudio();
        }

        void hontoPlayTone(int frequency, int durationMs) const
        {
            PlayTone(frequency, durationMs);
        }

        void EveryFrame(std::function<void(float)> fn)
        {
            if (m_State == nullptr || fn == nullptr)
            {
                return;
            }

            if (m_Scene != nullptr)
            {
                m_Scene->EveryFrame();
            }

            m_State->updates.push_back(std::move(fn));
        }

        void hontoEveryFrame(std::function<void(float)> fn)
        {
            EveryFrame(std::move(fn));
        }

        void WhenPressed(Key key, std::function<void()> fn)
        {
            EveryFrame(
                [key, fn = std::move(fn)](float)
                {
                    if (Pressed(key) && fn != nullptr)
                    {
                        fn();
                    }
                }
            );
        }

        void hontoWhenPressed(Key key, std::function<void()> fn)
        {
            WhenPressed(key, std::move(fn));
        }

        void WhilePressing(Key key, std::function<void(float)> fn)
        {
            EveryFrame(
                [key, fn = std::move(fn)](float deltaTime)
                {
                    if (Pressing(key) && fn != nullptr)
                    {
                        fn(deltaTime);
                    }
                }
            );
        }

        void hontoWhilePressing(Key key, std::function<void(float)> fn)
        {
            WhilePressing(key, std::move(fn));
        }

        template <typename TScene>
        void Go() const
        {
            if (m_Scene != nullptr)
            {
                m_Scene->Go<TScene>();
            }
        }

        template <typename TScene>
        void Go(const Transition& transition) const
        {
            if (m_Scene != nullptr)
            {
                m_Scene->Go<TScene>(transition);
            }
        }

        void Go(std::function<void(Stage&)> setup, const Transition& transition = {}) const
        {
            if (m_Scene != nullptr)
            {
                m_Scene->Go(std::make_unique<detail::ScriptScene>(std::move(setup)), transition);
            }
        }

        void GoWithFade(std::function<void(Stage&)> setup, float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 }) const
        {
            Go(std::move(setup), Fade(seconds, color));
        }

        template <typename TScene>
        void GoWithFade(float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 }) const
        {
            Go<TScene>(Fade(seconds, color));
        }

        template <typename TScene>
        void hontoGo() const
        {
            Go<TScene>();
        }

        template <typename TScene>
        void hontoGo(const Transition& transition) const
        {
            Go<TScene>(transition);
        }

        void hontoGo(std::function<void(Stage&)> setup, const Transition& transition = {}) const
        {
            Go(std::move(setup), transition);
        }

        void hontoGoWithFade(std::function<void(Stage&)> setup, float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 }) const
        {
            GoWithFade(std::move(setup), seconds, color);
        }

        template <typename TScene>
        void hontoGoWithFade(float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 }) const
        {
            GoWithFade<TScene>(seconds, color);
        }

    private:
        Actor CreateActor(const std::string& name, const std::shared_ptr<honto::Node>& node)
        {
            if (m_State == nullptr || node == nullptr)
            {
                return {};
            }

            auto actorState = std::make_shared<detail::ActorState>();
            actorState->node = node;
            actorState->stage = m_State;
            actorState->name = name.empty() ? ("actor_" + std::to_string(m_State->nextAnonymousId++)) : name;

            if (!name.empty())
            {
                m_State->namedActors[name] = actorState;
            }

            if (const auto tileMap = std::dynamic_pointer_cast<honto::TileMap>(node))
            {
                m_State->namedTileMaps[actorState->name] = tileMap;
            }

            if (m_Scene != nullptr)
            {
                m_Scene->Add(node);
            }

            return Actor(actorState);
        }

        HonTo::Scene* m_Scene = nullptr;
        std::shared_ptr<detail::StageState> m_State;
    };

    inline bool detail::ScriptScene::Setup()
    {
        Stage stage(this, m_State);

        if (m_Setup != nullptr)
        {
            m_Setup(stage);
        }

        if (m_State != nullptr && !m_State->updates.empty())
        {
            EveryFrame();
        }

        return true;
    }

    inline void detail::ScriptScene::Update(float deltaTime)
    {
        if (m_State == nullptr)
        {
            return;
        }

        const std::size_t callbackCount = m_State->updates.size();
        for (std::size_t index = 0; index < callbackCount; ++index)
        {
            const auto& callback = m_State->updates[index];
            if (callback != nullptr)
            {
                callback(deltaTime);
            }
        }
    }

    namespace detail
    {
        inline std::function<std::unique_ptr<honto::Scene>()> MakeScriptSceneFactory(std::function<void(Stage&)> setup)
        {
            return [setup = std::move(setup)]()
            {
                return std::make_unique<detail::ScriptScene>(setup);
            };
        }
    }

    class GameBuilder
    {
    public:
        GameBuilder()
        {
            m_Config.title = "HonTo Game";
        }

        GameBuilder& Title(std::string title)
        {
            m_Config.title = std::move(title);
            return *this;
        }

        GameBuilder& hontoTitle(std::string title)
        {
            return Title(std::move(title));
        }

        GameBuilder& Window(int width, int height)
        {
            m_Config.windowWidth = width;
            m_Config.windowHeight = height;
            return *this;
        }

        GameBuilder& hontoWindow(int width, int height)
        {
            return Window(width, height);
        }

        GameBuilder& Render(int width, int height)
        {
            m_Config.renderWidth = width;
            m_Config.renderHeight = height;
            return *this;
        }

        GameBuilder& hontoRender(int width, int height)
        {
            return Render(width, height);
        }

        GameBuilder& Clear(Color color)
        {
            m_Config.clearColor = color;
            return *this;
        }

        GameBuilder& hontoClear(Color color)
        {
            return Clear(color);
        }

        template <typename TScene>
        GameBuilder& StartWith()
        {
            m_SceneFactory = []()
            {
                return std::make_unique<TScene>();
            };
            return *this;
        }

        template <typename TScene>
        GameBuilder& hontoStartWith()
        {
            return StartWith<TScene>();
        }

        GameBuilder& Play(std::function<void(Stage&)> setup)
        {
            m_SceneFactory = detail::MakeScriptSceneFactory(std::move(setup));
            return *this;
        }

        GameBuilder& hontoPlay(std::function<void(Stage&)> setup)
        {
            return Play(std::move(setup));
        }

        template <typename TScene>
        GameBuilder& OpenWindow(
            std::string title,
            int windowWidth,
            int windowHeight,
            int renderWidth,
            int renderHeight,
            Color clearColor = Color { 16, 18, 28, 255 },
            bool closeStopsGame = false
        )
        {
            detail::BuiltGame::BuiltWindow window;
            window.config.title = std::move(title);
            window.config.windowWidth = windowWidth;
            window.config.windowHeight = windowHeight;
            window.config.renderWidth = renderWidth;
            window.config.renderHeight = renderHeight;
            window.config.clearColor = clearColor;
            window.closeStopsGame = closeStopsGame;
            window.sceneFactory = []()
            {
                return std::make_unique<TScene>();
            };
            m_AdditionalWindows.push_back(std::move(window));
            return *this;
        }

        template <typename TScene>
        GameBuilder& hontoOpenWindow(
            std::string title,
            int windowWidth,
            int windowHeight,
            int renderWidth,
            int renderHeight,
            Color clearColor = Color { 16, 18, 28, 255 },
            bool closeStopsGame = false
        )
        {
            return OpenWindow<TScene>(
                std::move(title),
                windowWidth,
                windowHeight,
                renderWidth,
                renderHeight,
                clearColor,
                closeStopsGame
            );
        }

        GameBuilder& OpenWindow(
            std::string title,
            int windowWidth,
            int windowHeight,
            int renderWidth,
            int renderHeight,
            std::function<void(Stage&)> setup,
            Color clearColor = Color { 16, 18, 28, 255 },
            bool closeStopsGame = false
        )
        {
            detail::BuiltGame::BuiltWindow window;
            window.config.title = std::move(title);
            window.config.windowWidth = windowWidth;
            window.config.windowHeight = windowHeight;
            window.config.renderWidth = renderWidth;
            window.config.renderHeight = renderHeight;
            window.config.clearColor = clearColor;
            window.closeStopsGame = closeStopsGame;
            window.sceneFactory = detail::MakeScriptSceneFactory(std::move(setup));
            m_AdditionalWindows.push_back(std::move(window));
            return *this;
        }

        GameBuilder& hontoOpenWindow(
            std::string title,
            int windowWidth,
            int windowHeight,
            int renderWidth,
            int renderHeight,
            std::function<void(Stage&)> setup,
            Color clearColor = Color { 16, 18, 28, 255 },
            bool closeStopsGame = false
        )
        {
            return OpenWindow(
                std::move(title),
                windowWidth,
                windowHeight,
                renderWidth,
                renderHeight,
                std::move(setup),
                clearColor,
                closeStopsGame
            );
        }

        int Run()
        {
            detail::BuiltGame game(m_Config, m_SceneFactory, m_AdditionalWindows);
            honto::Application application;
            return application.Run(game);
        }

        int hontoRun()
        {
            return Run();
        }

    private:
        honto::AppConfig m_Config {};
        std::function<std::unique_ptr<honto::Scene>()> m_SceneFactory;
        std::vector<detail::BuiltGame::BuiltWindow> m_AdditionalWindows;
    };

    inline GameBuilder Game(const std::string& title = "HonTo Game")
    {
        GameBuilder game;
        game.Title(title);
        return game;
    }
}

namespace honto
{
    using hontoVec2 = Vec2;
    using hontoColor = Color;
    using hontoKey = KeyCode;
    using hontoTransition = HonTo::Transition;
    using hontoScene = HonTo::Scene;
    using hontoActor = HonTo::Actor;
    using hontoAnimation = HonTo::Animation;
    using hontoFrameAnimation = HonTo::FrameAnimation;
    using hontoLevel = LevelDocument;
    using hontoLevelEntity = LevelEntity;
    using hontoStage = HonTo::Stage;
    using hontoSpriteBuilder = HonTo::Sprite;
    using hontoLayerBuilder = HonTo::Layer;
    using hontoFrameBuilder = HonTo::Frame;
    using hontoGameBuilder = HonTo::GameBuilder;
    using hontoTexture = Texture;
    using hontoTextureRegion = TextureRegion;
    using hontoTileMapActor = HonTo::TileMapActor;
    using hontoRaycastActor = HonTo::RaycastActor;

    inline Color hontoRGBA(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
    {
        return HonTo::RGBA(r, g, b, a);
    }

    inline void hontoPrint(const std::string& text)
    {
        HonTo::Print(text);
    }

    inline HonTo::Transition hontoFade(float seconds = 0.5f, Color color = Color { 0, 0, 0, 255 })
    {
        return HonTo::Fade(seconds, color);
    }

    inline HonTo::Sprite hontoBox(float width, float height, Color color)
    {
        return HonTo::Box(width, height, color);
    }

    inline std::shared_ptr<Texture> hontoLoadTexture(const std::string& path)
    {
        return HonTo::LoadTexture(path);
    }

    inline LevelDocument hontoLoadLevel(const std::string& path)
    {
        return HonTo::LoadLevel(path);
    }

    inline bool hontoSaveLevel(const std::string& path, const LevelDocument& level)
    {
        return HonTo::SaveLevel(path, level);
    }

    inline const LevelEntity* hontoFindLevelEntity(const LevelDocument& level, const std::string& name)
    {
        return HonTo::FindLevelEntity(level, name);
    }

    inline std::shared_ptr<Texture> hontoCheckerTexture(
        int width,
        int height,
        Color a,
        Color b,
        int cellSize = 8
    )
    {
        return HonTo::CheckerTexture(width, height, a, b, cellSize);
    }

    inline std::shared_ptr<Texture> hontoFrameSheetTexture(
        int frameWidth,
        int frameHeight,
        const std::vector<Color>& frameColors,
        int columns = 0
    )
    {
        return HonTo::FrameSheetTexture(frameWidth, frameHeight, frameColors, columns);
    }

    inline bool hontoPlaySound(const std::string& path, bool loop = false)
    {
        return HonTo::PlaySound(path, loop);
    }

    inline bool hontoPlayAlias(const std::string& alias, bool loop = false)
    {
        return HonTo::PlayAlias(alias, loop);
    }

    inline void hontoStopAudio()
    {
        HonTo::StopAudio();
    }

    inline void hontoPlayTone(int frequency, int durationMs)
    {
        HonTo::PlayTone(frequency, durationMs);
    }

    inline HonTo::Layer hontoFill(float width, float height, Color color)
    {
        return HonTo::Fill(width, height, color);
    }

    inline HonTo::Layer hontoFill(const Vec2& size, Color color)
    {
        return HonTo::Fill(size, color);
    }

    inline HonTo::Frame hontoOutline(float width, float height, Color color, int thickness = 1)
    {
        return HonTo::Outline(width, height, color, thickness);
    }

    inline Vec2 hontoVisibleSize()
    {
        return HonTo::VisibleSize();
    }

    inline bool hontoPressing(KeyCode key)
    {
        return HonTo::Pressing(key);
    }

    inline bool hontoPressed(KeyCode key)
    {
        return HonTo::Pressed(key);
    }

    inline HonTo::GameBuilder hontoGame(const std::string& title = "honto Game")
    {
        return HonTo::Game(title);
    }
}
