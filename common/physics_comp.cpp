//--------------------------------------------
//
// 物理コンポーネント [physics_comp.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "physics_comp.h"
#include "physics.h"
#include "object.h"
#include "trans_comp.h"
#include "log.h"

//--------------------------------------------
//
// 物理コンポーネントクラス
//
//--------------------------------------------

//---------------------------
// 初期化
//---------------------------
bool PhysicsComponent::start()
{
    /// 物理に登録する
    auto& owner = getOwner();
        auto trans = owner.getTransform();
        auto transform = trans->get();
        m_physicsManager.addRigidBody(getID(), m_collisionShapeType, transform, m_isTrigger, m_rigidBodyType, m_mass, m_collisionGroup, m_collisionMask);
}

//---------------------------
// 更新
//---------------------------
void PhysicsComponent::physicsSync()
{
    // 物理をもとに位置を更新する
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    trans->set(m_physicsManager.getTransform(getID()));
}

//---------------------------
// 描画
//---------------------------
void PhysicsComponent::render(Renderer& renderer)
{

}

//---------------------------
// 破棄
//---------------------------
void PhysicsComponent::destroy()
{
    // 物理を破棄
    m_physicsManager.removeRigidBody(getID());
}
