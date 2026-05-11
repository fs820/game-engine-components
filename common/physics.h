//--------------------------------------------
//
// 物理 [physics.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "physics_types.h"

class PhysicsManagerImpl;

//--------------------------------------------
// 物理管理クラス
//--------------------------------------------
class PhysicsManager
{
public:
    PhysicsManager();
    ~PhysicsManager();

    void init();
    void uninit();
    void simulate(float deltaTime);

    void addRigidBody(uint64_t id, CollisionShapeType shapeType, Transform offset, bool isTrigger, RigidBodyType bodyType, float mass, CollisionGroup collisionGroup, CollisionGroup collisionMask);
    void removeRigidBody(uint64_t id);

    void setGravity(const Vector3& gravity);

    void addForce(uint64_t id, const Vector3& force, bool isImpulse = false);
    void addTorque(uint64_t id, const Vector3& torque, bool isImpulse = false);
    void setLinearVelocity(uint64_t id, const Vector3& velocity);
    void setAngularVelocity(uint64_t id, const Vector3& velocity);
    void setTransform(uint64_t id, const Transform& transform, bool isResetForces = false, bool isUpdateMass = false);
    void setMaterial(uint64_t id, float friction, float restitution);

    Transform getTransform(uint64_t id);
    const std::vector<CollisionData>& getCollisionEvents() const;

    RayHitInfo rayCast(const Vector3& start, const Vector3& end);

private:
    std::unique_ptr<PhysicsManagerImpl> m_impl;
};
