#pragma once

#include "Color.h"
#include "Math.h"
#include "Renderer2D.h"
#include "Scene.h"
#include "Texture.h"

#include <memory>
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
        void ReplaceScene(std::unique_ptr<Scene> scene);
        void ReplaceScene(std::unique_ptr<Scene> scene, const SceneTransition& transition);

    private:
        friend class Application;

        void AttachApplication(Application* application);
        void DetachApplication(Application* application);

        Application* m_Application = nullptr;
    };
}
