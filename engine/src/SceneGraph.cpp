#include "honto/SceneGraph.h"

#include "honto/Application.h"

#include <algorithm>
#include <cmath>
#include <string>
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

    bool Label::Init()
    {
        RefreshContentSize();
        return true;
    }

    std::shared_ptr<Label> Label::Create(std::string text, Color color, int glyphScale, bool useCamera)
    {
        auto label = std::make_shared<Label>();
        if (label != nullptr && label->Init())
        {
            label->SetText(std::move(text));
            label->SetColor(color);
            label->SetGlyphScale(glyphScale);
            label->SetUseCamera(useCamera);
            return label;
        }

        return nullptr;
    }

    void Label::SetText(std::string text)
    {
        m_Text = std::move(text);
        RefreshContentSize();
    }

    const std::string& Label::GetText() const
    {
        return m_Text;
    }

    void Label::SetColor(Color color)
    {
        m_Color = color;
    }

    Color Label::GetColor() const
    {
        return m_Color;
    }

    void Label::SetGlyphScale(int glyphScale)
    {
        m_GlyphScale = std::max(1, glyphScale);
        RefreshContentSize();
    }

    int Label::GetGlyphScale() const
    {
        return m_GlyphScale;
    }

    void Label::SetUseCamera(bool useCamera)
    {
        m_UseCamera = useCamera;
    }

    bool Label::UsesCamera() const
    {
        return m_UseCamera;
    }

    void Label::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        const int pixelHeight = std::max(10, static_cast<int>(std::round(static_cast<float>(m_GlyphScale) * worldScale.x * 10.0f)));
        renderer.DrawText(m_Text, worldPosition, pixelHeight, m_Color, m_UseCamera);
    }

    void Label::RefreshContentSize()
    {
        SetContentSize(Renderer2D {}.MeasureText(m_Text, std::max(10, m_GlyphScale * 10)));
    }

    bool ProgressBar::Init()
    {
        return true;
    }

    std::shared_ptr<ProgressBar> ProgressBar::Create(float width, float height, float value, bool useCamera)
    {
        auto bar = std::make_shared<ProgressBar>();
        if (bar != nullptr && bar->Init())
        {
            bar->SetContentSize(width, height);
            bar->SetValue(value);
            bar->SetUseCamera(useCamera);
            return bar;
        }

        return nullptr;
    }

    void ProgressBar::SetValue(float value)
    {
        m_Value = std::clamp(value, 0.0f, 1.0f);
    }

    float ProgressBar::GetValue() const
    {
        return m_Value;
    }

    void ProgressBar::SetFillColor(Color color)
    {
        m_FillColor = color;
    }

    Color ProgressBar::GetFillColor() const
    {
        return m_FillColor;
    }

    void ProgressBar::SetBackgroundColor(Color color)
    {
        m_BackgroundColor = color;
    }

    Color ProgressBar::GetBackgroundColor() const
    {
        return m_BackgroundColor;
    }

    void ProgressBar::SetBorderColor(Color color)
    {
        m_BorderColor = color;
    }

    Color ProgressBar::GetBorderColor() const
    {
        return m_BorderColor;
    }

    void ProgressBar::SetUseCamera(bool useCamera)
    {
        m_UseCamera = useCamera;
    }

    bool ProgressBar::UsesCamera() const
    {
        return m_UseCamera;
    }

    void ProgressBar::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        const Vec2 size = GetContentSize() * worldScale;
        renderer.DrawFilledRect(worldPosition, size, m_BackgroundColor, m_UseCamera);
        renderer.DrawRectOutline(worldPosition, size, m_BorderColor, std::max(1, static_cast<int>(std::round(worldScale.x))), m_UseCamera);

        const float border = std::max(1.0f, worldScale.x);
        const float innerWidth = std::max(0.0f, size.x - (border * 2.0f));
        const float innerHeight = std::max(0.0f, size.y - (border * 2.0f));
        renderer.DrawFilledRect(
            { worldPosition.x + border, worldPosition.y + border },
            { innerWidth * m_Value, innerHeight },
            m_FillColor,
            m_UseCamera
        );
    }

    bool Button::Init()
    {
        return true;
    }

    std::shared_ptr<Button> Button::Create(std::string text, float width, float height, bool useCamera)
    {
        auto button = std::make_shared<Button>();
        if (button != nullptr && button->Init())
        {
            button->SetContentSize(width, height);
            button->SetText(std::move(text));
            button->SetUseCamera(useCamera);
            return button;
        }

        return nullptr;
    }

    void Button::SetText(std::string text)
    {
        m_Text = std::move(text);
    }

    const std::string& Button::GetText() const
    {
        return m_Text;
    }

    void Button::SetGlyphScale(int glyphScale)
    {
        m_GlyphScale = std::max(1, glyphScale);
    }

    int Button::GetGlyphScale() const
    {
        return m_GlyphScale;
    }

    void Button::SetTextColor(Color color)
    {
        m_TextColor = color;
    }

    Color Button::GetTextColor() const
    {
        return m_TextColor;
    }

    void Button::SetNormalColor(Color color)
    {
        m_NormalColor = color;
    }

    Color Button::GetNormalColor() const
    {
        return m_NormalColor;
    }

    void Button::SetHoverColor(Color color)
    {
        m_HoverColor = color;
    }

    Color Button::GetHoverColor() const
    {
        return m_HoverColor;
    }

    void Button::SetPressedColor(Color color)
    {
        m_PressedColor = color;
    }

    Color Button::GetPressedColor() const
    {
        return m_PressedColor;
    }

    void Button::SetBorderColor(Color color)
    {
        m_BorderColor = color;
    }

    Color Button::GetBorderColor() const
    {
        return m_BorderColor;
    }

    void Button::SetHovered(bool hovered)
    {
        m_Hovered = hovered;
    }

    bool Button::IsHovered() const
    {
        return m_Hovered;
    }

    void Button::SetPressed(bool pressed)
    {
        m_Pressed = pressed;
    }

    bool Button::IsPressed() const
    {
        return m_Pressed;
    }

    void Button::SetUseCamera(bool useCamera)
    {
        m_UseCamera = useCamera;
    }

    bool Button::UsesCamera() const
    {
        return m_UseCamera;
    }

    void Button::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        const Vec2 size = GetContentSize() * worldScale;
        Color background = m_NormalColor;
        if (m_Pressed)
        {
            background = m_PressedColor;
        }
        else if (m_Hovered)
        {
            background = m_HoverColor;
        }

        renderer.DrawFilledRect(worldPosition, size, background, m_UseCamera);
        renderer.DrawRectOutline(worldPosition, size, m_BorderColor, std::max(1, static_cast<int>(std::round(worldScale.x))), m_UseCamera);

        const int pixelHeight = std::max(10, static_cast<int>(std::round(static_cast<float>(m_GlyphScale) * worldScale.x * 10.0f)));
        const Vec2 textSize = renderer.MeasureText(m_Text, pixelHeight);
        const Vec2 textPosition {
            worldPosition.x + std::max(0.0f, (size.x - textSize.x) * 0.5f),
            worldPosition.y + std::max(0.0f, (size.y - textSize.y) * 0.5f)
        };
        renderer.DrawText(m_Text, textPosition, pixelHeight, m_TextColor, m_UseCamera);
    }

    bool ParticleEmitter::Init()
    {
        std::random_device seed;
        m_Rng.seed(seed());
        ScheduleUpdate();
        return true;
    }

    std::shared_ptr<ParticleEmitter> ParticleEmitter::Create(float width, float height)
    {
        auto emitter = std::make_shared<ParticleEmitter>();
        if (emitter != nullptr && emitter->Init())
        {
            emitter->SetContentSize(width, height);
            emitter->SetSpawnArea({ width, height });
            return emitter;
        }

        return nullptr;
    }

    void ParticleEmitter::SetEmissionRate(float particlesPerSecond)
    {
        m_EmissionRate = std::max(0.0f, particlesPerSecond);
    }

    float ParticleEmitter::GetEmissionRate() const
    {
        return m_EmissionRate;
    }

    void ParticleEmitter::SetSpawnArea(const Vec2& size)
    {
        m_SpawnArea = { std::max(0.0f, size.x), std::max(0.0f, size.y) };
    }

    Vec2 ParticleEmitter::GetSpawnArea() const
    {
        return m_SpawnArea;
    }

    void ParticleEmitter::SetVelocityRange(const Vec2& minimum, const Vec2& maximum)
    {
        m_VelocityMin = {
            std::min(minimum.x, maximum.x),
            std::min(minimum.y, maximum.y)
        };
        m_VelocityMax = {
            std::max(minimum.x, maximum.x),
            std::max(minimum.y, maximum.y)
        };
    }

    Vec2 ParticleEmitter::GetVelocityMin() const
    {
        return m_VelocityMin;
    }

    Vec2 ParticleEmitter::GetVelocityMax() const
    {
        return m_VelocityMax;
    }

    void ParticleEmitter::SetLifetimeRange(float minimumSeconds, float maximumSeconds)
    {
        m_LifetimeMin = std::max(0.01f, std::min(minimumSeconds, maximumSeconds));
        m_LifetimeMax = std::max(m_LifetimeMin, std::max(minimumSeconds, maximumSeconds));
    }

    float ParticleEmitter::GetLifetimeMin() const
    {
        return m_LifetimeMin;
    }

    float ParticleEmitter::GetLifetimeMax() const
    {
        return m_LifetimeMax;
    }

    void ParticleEmitter::SetSizeRange(float minimumSize, float maximumSize)
    {
        m_SizeMin = std::max(1.0f, std::min(minimumSize, maximumSize));
        m_SizeMax = std::max(m_SizeMin, std::max(minimumSize, maximumSize));
    }

    float ParticleEmitter::GetSizeMin() const
    {
        return m_SizeMin;
    }

    float ParticleEmitter::GetSizeMax() const
    {
        return m_SizeMax;
    }

    void ParticleEmitter::SetColorRange(Color start, Color finish)
    {
        m_StartColor = start;
        m_FinishColor = finish;
    }

    Color ParticleEmitter::GetStartColor() const
    {
        return m_StartColor;
    }

    Color ParticleEmitter::GetFinishColor() const
    {
        return m_FinishColor;
    }

    void ParticleEmitter::SetUseCamera(bool useCamera)
    {
        m_UseCamera = useCamera;
    }

    bool ParticleEmitter::UsesCamera() const
    {
        return m_UseCamera;
    }

    void ParticleEmitter::Burst(int count)
    {
        const int safeCount = std::max(0, count);
        for (int index = 0; index < safeCount; ++index)
        {
            SpawnParticle();
        }
    }

    void ParticleEmitter::ClearParticles()
    {
        m_Particles.clear();
    }

    int ParticleEmitter::GetParticleCount() const
    {
        return static_cast<int>(m_Particles.size());
    }

    void ParticleEmitter::Update(float deltaTime)
    {
        if (m_EmissionRate > 0.0f)
        {
            m_EmissionCarry += m_EmissionRate * std::max(0.0f, deltaTime);
            const int spawnCount = static_cast<int>(std::floor(m_EmissionCarry));
            if (spawnCount > 0)
            {
                Burst(spawnCount);
                m_EmissionCarry -= static_cast<float>(spawnCount);
            }
        }

        for (Particle& particle : m_Particles)
        {
            particle.life -= deltaTime;
            particle.position += particle.velocity * deltaTime;
        }

        m_Particles.erase(
            std::remove_if(
                m_Particles.begin(),
                m_Particles.end(),
                [](const Particle& particle)
                {
                    return particle.life <= 0.0f;
                }
            ),
            m_Particles.end()
        );
    }

    void ParticleEmitter::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        const auto blend = [](std::uint8_t from, std::uint8_t to, float t)
        {
            return static_cast<std::uint8_t>(static_cast<float>(from) + ((static_cast<float>(to) - static_cast<float>(from)) * t));
        };

        for (const Particle& particle : m_Particles)
        {
            const float lifeT = 1.0f - std::clamp(particle.life / std::max(0.01f, particle.maxLife), 0.0f, 1.0f);
            const Color color {
                blend(m_StartColor.r, m_FinishColor.r, lifeT),
                blend(m_StartColor.g, m_FinishColor.g, lifeT),
                blend(m_StartColor.b, m_FinishColor.b, lifeT),
                blend(m_StartColor.a, m_FinishColor.a, lifeT)
            };
            const float size = particle.size * std::max(worldScale.x, worldScale.y);
            renderer.DrawFilledRect(
                {
                    worldPosition.x + (particle.position.x * worldScale.x),
                    worldPosition.y + (particle.position.y * worldScale.y)
                },
                { size, size },
                color,
                m_UseCamera
            );
        }
    }

    float ParticleEmitter::RandomFloat(float minimum, float maximum)
    {
        const float safeMinimum = std::min(minimum, maximum);
        const float safeMaximum = std::max(minimum, maximum);
        std::uniform_real_distribution<float> distribution(safeMinimum, safeMaximum);
        return distribution(m_Rng);
    }

    void ParticleEmitter::SpawnParticle()
    {
        Particle particle;
        particle.position = {
            RandomFloat(0.0f, m_SpawnArea.x),
            RandomFloat(0.0f, m_SpawnArea.y)
        };
        particle.velocity = {
            RandomFloat(m_VelocityMin.x, m_VelocityMax.x),
            RandomFloat(m_VelocityMin.y, m_VelocityMax.y)
        };
        particle.maxLife = RandomFloat(m_LifetimeMin, m_LifetimeMax);
        particle.life = particle.maxLife;
        particle.size = RandomFloat(m_SizeMin, m_SizeMax);
        m_Particles.push_back(particle);
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

    Window* Director::GetWindow() const
    {
        if (m_Application == nullptr)
        {
            return nullptr;
        }

        return &m_Application->GetWindow();
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

    bool Director::ReplaceSceneInWindow(const std::string& windowIdOrTitle, std::unique_ptr<Scene> scene, const SceneTransition& transition, bool focusWindow)
    {
        return m_Application != nullptr && m_Application->SetSceneForWindow(windowIdOrTitle, std::move(scene), transition, focusWindow);
    }

    bool Director::FocusWindow(const std::string& windowIdOrTitle) const
    {
        return m_Application != nullptr && m_Application->FocusWindow(windowIdOrTitle);
    }

    bool Director::OpenWindow(WindowStartup startup, bool focusWindow)
    {
        return m_Application != nullptr && m_Application->OpenWindow(std::move(startup), focusWindow);
    }

    bool Director::CloseWindow(const std::string& windowIdOrTitle) const
    {
        return m_Application != nullptr && m_Application->CloseWindow(windowIdOrTitle);
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
