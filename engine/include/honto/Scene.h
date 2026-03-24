#pragma once

#include "Entity.h"
#include "Renderer2D.h"

#include <deque>
#include <string>

namespace honto
{
    class Scene
    {
    public:
        virtual ~Scene() = default;

        virtual void OnCreate() {}
        virtual void OnUpdate(float deltaTime);
        virtual void OnRender(Renderer2D& renderer);
        virtual void OnDestroy() {}

        Entity& CreateEntity(const std::string& name);
        std::deque<Entity>& Entities();
        const std::deque<Entity>& Entities() const;

    protected:
        Entity* FindEntity(const std::string& name);
        const Entity* FindEntity(const std::string& name) const;

    private:
        std::deque<Entity> m_Entities;
    };
}
