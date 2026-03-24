#pragma once

#include "Components.h"
#include "Transform2D.h"

#include <cassert>
#include <string>
#include <utility>

namespace honto
{
    class Entity
    {
    public:
        explicit Entity(std::string name = "Entity") : m_Name(std::move(name)) {}

        const std::string& Name() const
        {
            return m_Name;
        }

        Transform2D& Transform()
        {
            return m_Transform;
        }

        const Transform2D& Transform() const
        {
            return m_Transform;
        }

        Entity& AddSprite(const SpriteComponent& sprite = {})
        {
            m_Sprite = sprite;
            m_HasSprite = true;
            return *this;
        }

        Entity& AddRigidBody(const RigidBody2D& rigidBody = {})
        {
            m_RigidBody = rigidBody;
            m_HasRigidBody = true;
            return *this;
        }

        bool HasSprite() const
        {
            return m_HasSprite;
        }

        bool HasRigidBody() const
        {
            return m_HasRigidBody;
        }

        SpriteComponent& Sprite()
        {
            assert(m_HasSprite && "Sprite component was not added to this entity.");
            return m_Sprite;
        }

        const SpriteComponent& Sprite() const
        {
            assert(m_HasSprite && "Sprite component was not added to this entity.");
            return m_Sprite;
        }

        RigidBody2D& RigidBody()
        {
            assert(m_HasRigidBody && "RigidBody2D component was not added to this entity.");
            return m_RigidBody;
        }

        const RigidBody2D& RigidBody() const
        {
            assert(m_HasRigidBody && "RigidBody2D component was not added to this entity.");
            return m_RigidBody;
        }

    private:
        std::string m_Name;
        Transform2D m_Transform {};
        bool m_HasSprite = false;
        bool m_HasRigidBody = false;
        SpriteComponent m_Sprite {};
        RigidBody2D m_RigidBody {};
    };
}
