#include "honto/SceneGraph.h"

#include "honto/Application.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace honto
{
    void Node::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    void Node::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        (void)renderer;
        (void)worldPosition;
        (void)worldScale;
    }

    void Node::AddChild(const std::shared_ptr<Node>& child, int zOrder)
    {
        if (child == nullptr)
        {
            return;
        }

        child->m_Parent = this;
        child->m_LocalZOrder = zOrder;
        m_Children.push_back(child);
        SortChildren();
    }

    void Node::RemoveAllChildren()
    {
        for (const std::shared_ptr<Node>& child : m_Children)
        {
            if (child != nullptr)
            {
                child->m_Parent = nullptr;
            }
        }

        m_Children.clear();
    }

    void Node::SetPosition(const Vec2& position)
    {
        m_Position = position;
    }

    void Node::SetPosition(float x, float y)
    {
        m_Position = { x, y };
    }

    const Vec2& Node::GetPosition() const
    {
        return m_Position;
    }

    void Node::SetLocalZOrder(int zOrder)
    {
        m_LocalZOrder = zOrder;
        if (m_Parent != nullptr)
        {
            m_Parent->SortChildren();
        }
    }

    void Node::SetScale(const Vec2& scale)
    {
        m_Scale = scale;
    }

    void Node::SetScale(float uniformScale)
    {
        m_Scale = { uniformScale, uniformScale };
    }

    const Vec2& Node::GetScale() const
    {
        return m_Scale;
    }

    void Node::SetContentSize(const Vec2& size)
    {
        m_ContentSize = size;
    }

    void Node::SetContentSize(float width, float height)
    {
        m_ContentSize = { width, height };
    }

    const Vec2& Node::GetContentSize() const
    {
        return m_ContentSize;
    }

    void Node::SetVisible(bool visible)
    {
        m_Visible = visible;
    }

    bool Node::IsVisible() const
    {
        return m_Visible;
    }

    void Node::ScheduleUpdate()
    {
        m_IsUpdateScheduled = true;
    }

    void Node::UnscheduleUpdate()
    {
        m_IsUpdateScheduled = false;
    }

    bool Node::IsUpdateScheduled() const
    {
        return m_IsUpdateScheduled;
    }

    int Node::GetLocalZOrder() const
    {
        return m_LocalZOrder;
    }

    Node* Node::GetParent() const
    {
        return m_Parent;
    }

    void Node::VisitUpdate(float deltaTime)
    {
        if (m_IsUpdateScheduled)
        {
            Update(deltaTime);
        }

        for (const std::shared_ptr<Node>& child : m_Children)
        {
            if (child != nullptr)
            {
                child->VisitUpdate(deltaTime);
            }
        }
    }

    void Node::VisitRender(Renderer2D& renderer, const Vec2& parentPosition, const Vec2& parentScale)
    {
        if (!m_Visible)
        {
            return;
        }

        const Vec2 worldPosition = parentPosition + m_Position;
        const Vec2 worldScale = parentScale * m_Scale;
        Draw(renderer, worldPosition, worldScale);

        for (const std::shared_ptr<Node>& child : m_Children)
        {
            if (child != nullptr)
            {
                child->VisitRender(renderer, worldPosition, worldScale);
            }
        }
    }

    void Node::SortChildren()
    {
        std::stable_sort(
            m_Children.begin(),
            m_Children.end(),
            [](const std::shared_ptr<Node>& left, const std::shared_ptr<Node>& right)
            {
                if (left == nullptr || right == nullptr)
                {
                    return left != nullptr;
                }

                return left->m_LocalZOrder < right->m_LocalZOrder;
            }
        );
    }

    bool LayerColor::Init()
    {
        return true;
    }

    std::shared_ptr<LayerColor> LayerColor::Create(Color color, const Vec2& size)
    {
        auto layer = std::make_shared<LayerColor>();
        if (layer != nullptr && layer->Init())
        {
            layer->SetColor(color);
            layer->SetContentSize(size);
            return layer;
        }

        return nullptr;
    }

    std::shared_ptr<LayerColor> LayerColor::Create(Color color, float width, float height)
    {
        return Create(color, { width, height });
    }

    void LayerColor::SetColor(Color color)
    {
        m_Color = color;
    }

    Color LayerColor::GetColor() const
    {
        return m_Color;
    }

    void LayerColor::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        renderer.DrawFilledRect(worldPosition, GetContentSize() * worldScale, m_Color);
    }

    bool Sprite::Init()
    {
        return true;
    }

    std::shared_ptr<Sprite> Sprite::CreateSolid(const Vec2& size, Color color)
    {
        auto sprite = std::make_shared<Sprite>();
        if (sprite != nullptr && sprite->Init())
        {
            sprite->SetContentSize(size);
            sprite->SetColor(color);
            return sprite;
        }

        return nullptr;
    }

    std::shared_ptr<Sprite> Sprite::CreateTextured(const Vec2& size, const std::shared_ptr<Texture>& texture, Color tint)
    {
        auto sprite = std::make_shared<Sprite>();
        if (sprite != nullptr && sprite->Init())
        {
            sprite->SetContentSize(size);
            sprite->SetColor(tint);
            sprite->SetTexture(texture);
            return sprite;
        }

        return nullptr;
    }

    void Sprite::SetColor(Color color)
    {
        m_Color = color;
    }

    Color Sprite::GetColor() const
    {
        return m_Color;
    }

    void Sprite::SetTexture(const std::shared_ptr<Texture>& texture)
    {
        m_Texture = texture;
    }

    const std::shared_ptr<Texture>& Sprite::GetTexture() const
    {
        return m_Texture;
    }

    void Sprite::SetTextureRegion(const TextureRegion& region)
    {
        m_TextureRegion = region;
        m_UsesTextureRegion = region.IsValid();
    }

    TextureRegion Sprite::GetTextureRegion() const
    {
        return m_TextureRegion;
    }

    void Sprite::ClearTextureRegion()
    {
        m_TextureRegion = {};
        m_UsesTextureRegion = false;
    }

    bool Sprite::HasTextureRegion() const
    {
        return m_UsesTextureRegion;
    }

    bool Sprite::SetTextureFrame(int frameIndex, int frameWidth, int frameHeight, int columns)
    {
        if (m_Texture == nullptr || !m_Texture->IsValid() || frameIndex < 0 || frameWidth <= 0 || frameHeight <= 0)
        {
            return false;
        }

        const int safeColumns = std::max(1, columns <= 0 ? (m_Texture->Width() / frameWidth) : columns);
        const int frameX = (frameIndex % safeColumns) * frameWidth;
        const int frameY = (frameIndex / safeColumns) * frameHeight;
        if (frameX < 0 || frameY < 0 || frameX + frameWidth > m_Texture->Width() || frameY + frameHeight > m_Texture->Height())
        {
            return false;
        }

        SetTextureRegion({ frameX, frameY, frameWidth, frameHeight });
        return true;
    }

    void Sprite::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        if (m_Texture != nullptr && m_Texture->IsValid())
        {
            if (m_UsesTextureRegion)
            {
                renderer.DrawTexturedRectRegion(worldPosition, GetContentSize() * worldScale, *m_Texture, m_TextureRegion, m_Color);
            }
            else
            {
                renderer.DrawTexturedRect(worldPosition, GetContentSize() * worldScale, *m_Texture, m_Color);
            }
            return;
        }

        renderer.DrawFilledRect(worldPosition, GetContentSize() * worldScale, m_Color);
    }

    void CodeScene::OnCreate()
    {
        Init();
    }

    void CodeScene::OnUpdate(float deltaTime)
    {
        Scene::OnUpdate(deltaTime);
        VisitUpdate(deltaTime);
    }

    void CodeScene::OnRender(Renderer2D& renderer)
    {
        Scene::OnRender(renderer);
        VisitRender(renderer, {}, { 1.0f, 1.0f });
    }

    void CodeScene::OnDestroy()
    {
        RemoveAllChildren();
    }

    Director& Director::Get()
    {
        static Director instance;
        return instance;
    }

    Vec2 Director::GetVisibleSize() const
    {
        if (m_Application == nullptr)
        {
            return {};
        }

        const AppConfig& config = m_Application->Config();
        return {
            static_cast<float>(config.renderWidth),
            static_cast<float>(config.renderHeight)
        };
    }

    Renderer2D* Director::GetRenderer() const
    {
        if (m_Application == nullptr)
        {
            return nullptr;
        }

        return &m_Application->GetRenderer();
    }

    void Director::ReplaceScene(std::unique_ptr<Scene> scene)
    {
        if (m_Application != nullptr)
        {
            m_Application->SetScene(std::move(scene));
        }
    }

    void Director::ReplaceScene(std::unique_ptr<Scene> scene, const SceneTransition& transition)
    {
        if (m_Application != nullptr)
        {
            m_Application->SetScene(std::move(scene), transition);
        }
    }

    void Director::AttachApplication(Application* application)
    {
        m_Application = application;
    }

    void Director::DetachApplication(Application* application)
    {
        if (m_Application == application)
        {
            m_Application = nullptr;
        }
    }
}
