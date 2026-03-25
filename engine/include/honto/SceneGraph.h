#pragma once

#include "Color.h"
#include "Math.h"
#include "Renderer2D.h"
#include "Scene.h"
#include "Texture.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

#define HONTO_CREATE_FUNC(type)                                                                 \
    static std::shared_ptr<type> Create()                                                      \
    {                                                                                          \
        auto instance = std::make_shared<type>();                                              \
        if (instance != nullptr && instance->Init())                                           \
        {                                                                                      \
            return instance;                                                                   \
        }                                                                                      \
        return nullptr;                                                                        \
    }

#define HONTO_CREATE_SCENE_FUNC(type)                                                          \
    static std::unique_ptr<type> CreateScene()                                                 \
    {                                                                                          \
        return std::make_unique<type>();                                                       \
    }

namespace honto
{
    class Application;
    class Window;
    struct SceneTransition;

    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        virtual ~Node() = default;

        virtual bool Init()
        {
            return true;
        }

        virtual void Update(float deltaTime);
        virtual void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale);

        void AddChild(const std::shared_ptr<Node>& child, int zOrder = 0);

        template <typename T>
        std::shared_ptr<T> AddChild(const std::shared_ptr<T>& child, int zOrder = 0)
        {
            AddChild(std::static_pointer_cast<Node>(child), zOrder);
            return child;
        }

        void RemoveAllChildren();

        void SetPosition(const Vec2& position);
        void SetPosition(float x, float y);
        const Vec2& GetPosition() const;

        void SetLocalZOrder(int zOrder);

        void SetScale(const Vec2& scale);
        void SetScale(float uniformScale);
        const Vec2& GetScale() const;

        void SetContentSize(const Vec2& size);
        void SetContentSize(float width, float height);
        const Vec2& GetContentSize() const;

        void SetVisible(bool visible);
        bool IsVisible() const;

        void ScheduleUpdate();
        void UnscheduleUpdate();
        bool IsUpdateScheduled() const;

        int GetLocalZOrder() const;
        Node* GetParent() const;

    protected:
        void VisitUpdate(float deltaTime);
        void VisitRender(Renderer2D& renderer, const Vec2& parentPosition, const Vec2& parentScale);

    private:
        void SortChildren();

        Vec2 m_Position {};
        Vec2 m_Scale { 1.0f, 1.0f };
        Vec2 m_ContentSize {};
        Node* m_Parent = nullptr;
        std::vector<std::shared_ptr<Node>> m_Children;
        int m_LocalZOrder = 0;
        bool m_Visible = true;
        bool m_IsUpdateScheduled = false;
    };

    class Layer : public Node
    {
    public:
        HONTO_CREATE_FUNC(Layer)
    };

    class LayerColor : public Layer
    {
    public:
        bool Init() override;
        static std::shared_ptr<LayerColor> Create(Color color, const Vec2& size);
        static std::shared_ptr<LayerColor> Create(Color color, float width, float height);

        void SetColor(Color color);
        Color GetColor() const;

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        Color m_Color { 255, 255, 255, 255 };
    };

    class Sprite : public Node
    {
    public:
        bool Init() override;
        HONTO_CREATE_FUNC(Sprite)

        static std::shared_ptr<Sprite> CreateSolid(const Vec2& size, Color color);
        static std::shared_ptr<Sprite> CreateTextured(const Vec2& size, const std::shared_ptr<Texture>& texture, Color tint = { 255, 255, 255, 255 });

        void SetColor(Color color);
        Color GetColor() const;
        void SetTexture(const std::shared_ptr<Texture>& texture);
        const std::shared_ptr<Texture>& GetTexture() const;
        void SetTextureRegion(const TextureRegion& region);
        TextureRegion GetTextureRegion() const;
        void ClearTextureRegion();
        bool HasTextureRegion() const;
        bool SetTextureFrame(int frameIndex, int frameWidth, int frameHeight, int columns = 0);

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        Color m_Color { 255, 255, 255, 255 };
        std::shared_ptr<Texture> m_Texture;
        TextureRegion m_TextureRegion {};
        bool m_UsesTextureRegion = false;
    };

    class Label : public Node
    {
    public:
        bool Init() override;
        static std::shared_ptr<Label> Create(std::string text, Color color = { 255, 255, 255, 255 }, int glyphScale = 1, bool useCamera = false);

        void SetText(std::string text);
        const std::string& GetText() const;
        void SetColor(Color color);
        Color GetColor() const;
        void SetGlyphScale(int glyphScale);
        int GetGlyphScale() const;
        void SetUseCamera(bool useCamera);
        bool UsesCamera() const;

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        void RefreshContentSize();

        std::string m_Text;
        Color m_Color { 255, 255, 255, 255 };
        int m_GlyphScale = 1;
        bool m_UseCamera = false;
    };

    class ProgressBar : public Node
    {
    public:
        bool Init() override;
        static std::shared_ptr<ProgressBar> Create(float width, float height, float value = 1.0f, bool useCamera = false);

        void SetValue(float value);
        float GetValue() const;
        void SetFillColor(Color color);
        Color GetFillColor() const;
        void SetBackgroundColor(Color color);
        Color GetBackgroundColor() const;
        void SetBorderColor(Color color);
        Color GetBorderColor() const;
        void SetUseCamera(bool useCamera);
        bool UsesCamera() const;

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        float m_Value = 1.0f;
        Color m_FillColor { 92, 220, 128, 255 };
        Color m_BackgroundColor { 18, 24, 36, 180 };
        Color m_BorderColor { 255, 255, 255, 255 };
        bool m_UseCamera = false;
    };

    class Button : public Node
    {
    public:
        bool Init() override;
        static std::shared_ptr<Button> Create(std::string text, float width, float height, bool useCamera = false);

        void SetText(std::string text);
        const std::string& GetText() const;
        void SetGlyphScale(int glyphScale);
        int GetGlyphScale() const;
        void SetTextColor(Color color);
        Color GetTextColor() const;
        void SetNormalColor(Color color);
        Color GetNormalColor() const;
        void SetHoverColor(Color color);
        Color GetHoverColor() const;
        void SetPressedColor(Color color);
        Color GetPressedColor() const;
        void SetBorderColor(Color color);
        Color GetBorderColor() const;
        void SetHovered(bool hovered);
        bool IsHovered() const;
        void SetPressed(bool pressed);
        bool IsPressed() const;
        void SetUseCamera(bool useCamera);
        bool UsesCamera() const;

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        std::string m_Text;
        int m_GlyphScale = 1;
        Color m_TextColor { 255, 255, 255, 255 };
        Color m_NormalColor { 44, 62, 96, 255 };
        Color m_HoverColor { 68, 94, 144, 255 };
        Color m_PressedColor { 92, 128, 188, 255 };
        Color m_BorderColor { 236, 245, 255, 255 };
        bool m_Hovered = false;
        bool m_Pressed = false;
        bool m_UseCamera = false;
    };

    class ParticleEmitter : public Node
    {
    public:
        bool Init() override;
        static std::shared_ptr<ParticleEmitter> Create(float width, float height);

        void SetEmissionRate(float particlesPerSecond);
        float GetEmissionRate() const;
        void SetSpawnArea(const Vec2& size);
        Vec2 GetSpawnArea() const;
        void SetVelocityRange(const Vec2& minimum, const Vec2& maximum);
        Vec2 GetVelocityMin() const;
        Vec2 GetVelocityMax() const;
        void SetLifetimeRange(float minimumSeconds, float maximumSeconds);
        float GetLifetimeMin() const;
        float GetLifetimeMax() const;
        void SetSizeRange(float minimumSize, float maximumSize);
        float GetSizeMin() const;
        float GetSizeMax() const;
        void SetColorRange(Color start, Color finish);
        Color GetStartColor() const;
        Color GetFinishColor() const;
        void SetUseCamera(bool useCamera);
        bool UsesCamera() const;
        void Burst(int count);
        void ClearParticles();
        int GetParticleCount() const;

        void Update(float deltaTime) override;
        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        struct Particle
        {
            Vec2 position {};
            Vec2 velocity {};
            float life = 0.0f;
            float maxLife = 1.0f;
            float size = 2.0f;
        };

        float RandomFloat(float minimum, float maximum);
        void SpawnParticle();

        std::vector<Particle> m_Particles;
        std::mt19937 m_Rng;
        float m_EmissionRate = 0.0f;
        float m_EmissionCarry = 0.0f;
        Vec2 m_SpawnArea {};
        Vec2 m_VelocityMin { -30.0f, -30.0f };
        Vec2 m_VelocityMax { 30.0f, -80.0f };
        float m_LifetimeMin = 0.35f;
        float m_LifetimeMax = 0.9f;
        float m_SizeMin = 2.0f;
        float m_SizeMax = 5.0f;
        Color m_StartColor { 255, 236, 162, 255 };
        Color m_FinishColor { 255, 128, 64, 0 };
        bool m_UseCamera = true;
    };

    class CodeScene : public Scene, public Node
    {
    public:
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;
        void OnRender(Renderer2D& renderer) override;
        void OnDestroy() override;
    };

    class Director
    {
    public:
        static Director& Get();

        Vec2 GetVisibleSize() const;
        Renderer2D* GetRenderer() const;
        Window* GetWindow() const;
        void ReplaceScene(std::unique_ptr<Scene> scene);
        void ReplaceScene(std::unique_ptr<Scene> scene, const SceneTransition& transition);
        bool ReplaceSceneInWindow(const std::string& windowIdOrTitle, std::unique_ptr<Scene> scene, const SceneTransition& transition, bool focusWindow = false);
        bool FocusWindow(const std::string& windowIdOrTitle) const;

    private:
        friend class Application;

        void AttachApplication(Application* application);
        void DetachApplication(Application* application);

        Application* m_Application = nullptr;
    };
}
