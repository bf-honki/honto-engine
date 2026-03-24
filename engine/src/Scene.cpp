#include "honto/Scene.h"

namespace honto
{
    void Scene::OnUpdate(float deltaTime)
    {
        for (Entity& entity : m_Entities)
        {
            if (!entity.HasRigidBody())
            {
                continue;
            }

            entity.Transform().position += entity.RigidBody().velocity * deltaTime;
        }
    }

    void Scene::OnRender(Renderer2D& renderer)
    {
        for (const Entity& entity : m_Entities)
        {
            if (!entity.HasSprite() || !entity.Sprite().visible)
            {
                continue;
            }

            const Vec2 size {
                entity.Sprite().size.x * entity.Transform().scale.x,
                entity.Sprite().size.y * entity.Transform().scale.y
            };

            renderer.DrawFilledRect(entity.Transform().position, size, entity.Sprite().color);
        }
    }

    Entity& Scene::CreateEntity(const std::string& name)
    {
        m_Entities.emplace_back(name);
        return m_Entities.back();
    }

    std::deque<Entity>& Scene::Entities()
    {
        return m_Entities;
    }

    const std::deque<Entity>& Scene::Entities() const
    {
        return m_Entities;
    }

    Entity* Scene::FindEntity(const std::string& name)
    {
        for (Entity& entity : m_Entities)
        {
            if (entity.Name() == name)
            {
                return &entity;
            }
        }

        return nullptr;
    }

    const Entity* Scene::FindEntity(const std::string& name) const
    {
        for (const Entity& entity : m_Entities)
        {
            if (entity.Name() == name)
            {
                return &entity;
            }
        }

        return nullptr;
    }
}
