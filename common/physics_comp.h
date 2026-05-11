//--------------------------------------------
//
// 物理コンポーネント [physics_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"
#include "physics_types.h"

class PhysicsManager;

//-------------------------------------
// 物理コンポーネントクラス
//-------------------------------------
class PhysicsComponent : public Component
{
public:
    PhysicsComponent(PhysicsManager& physicsManager, CollisionShapeType collisionShapeType, RigidBodyType rigidBodyType, float mass, bool isTrigger, CollisionGroup collisionGroup, CollisionGroup collisionMask)
        : m_physicsManager(physicsManager), m_collisionShapeType(collisionShapeType), m_rigidBodyType(rigidBodyType), m_mass(mass), m_isTrigger(isTrigger), m_collisionGroup(collisionGroup), m_collisionMask(collisionMask) {}
    virtual ~PhysicsComponent() = default;

    bool start() override;
    void physicsSync() override;
    void render(Renderer& renderer) override;
    void destroy() override;

private:
    PhysicsManager& m_physicsManager; // 物理マネージャー参照

    CollisionShapeType m_collisionShapeType; // 形状
    RigidBodyType m_rigidBodyType;           // 物理タイプ
    float m_mass;                            // 重さ
    bool m_isTrigger;                        // 押し出されるかどうか
    CollisionGroup m_collisionGroup;         // 自分は誰か
    CollisionGroup m_collisionMask;          // 誰とぶつかるか
};
