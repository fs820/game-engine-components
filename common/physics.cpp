//----------------------------------
//
// 物理 [physics.cpp]
// Author: Fuma Sato
//
//----------------------------------
#include "physics.h"
#include <bullet/btBulletDynamicsCommon.h> // Bullet Physics SDK

//--------------------------------
// 内部実装クラス
//--------------------------------
class PhysicsManagerImpl
{
public:
    PhysicsManagerImpl() : m_collisionShapes{}, m_collisionConfig{}, m_dispatcher{}, m_broadphase{}, m_solver{}, m_dynamicsWorld{} {}
    ~PhysicsManagerImpl() = default;

    void init();
    void uninit();
    void simulate(float deltaTime);

    void setGravity(const Vector3& gravity);

    void addRigidBody(uint64_t id, CollisionShapeType shapeType, Transform offset, bool isTrigger, RigidBodyType bodyType, float mass, CollisionGroup collisionGroup, CollisionGroup collisionMask);
    void removeRigidBody(uint64_t id);

    void addForce(uint64_t id, const Vector3& force, bool isImpulse = false);
    void addTorque(uint64_t id, const Vector3& torque, bool isImpulse = false);
    void setLinearVelocity(uint64_t id, const Vector3& velocity);
    void setAngularVelocity(uint64_t id, const Vector3& angularVelocity);
    void setTransform(uint64_t id, const Transform& transform, bool isResetForces = false, bool isUpdateMass = false);
    void setMaterial(uint64_t id, float friction, float restitution);

    Transform getTransform(uint64_t id);
    const std::vector<CollisionData>& getCollisionEvents() const { return m_collisionEvents; }
    RayHitInfo rayCast(const Vector3& start, const Vector3& end);

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig; // 衝突設定
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;                // 衝突ディスパッチャ
    std::unique_ptr<btBroadphaseInterface> m_broadphase;                // ブロードフェーズ
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;      // 制約ソルバー
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;           // 物理ワールド

    std::unordered_map<uint64_t, btCollisionShape*> m_collisionShapes; // 衝突形状マップ
    std::unordered_map<uint64_t, btRigidBody*> m_rigidBodies;          // 剛体マップ
    std::vector<CollisionData> m_collisionEvents;                      // 衝突リスト
};

//--------------------------------
// シミュレーションのセットアップ
//--------------------------------
void PhysicsManagerImpl::init()
{
    // 衝突設定の初期化
    m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    // 物理ワールドの初期化
    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broadphase.get(),
        m_solver.get(),
        m_collisionConfig.get()
    );

    // 重力の設定
    setGravity(Vector3(0, EARTH_GRAVITY, 0));
}

//--------------------------------
// シミュレーションの終了
//--------------------------------
void PhysicsManagerImpl::uninit()
{
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_broadphase.reset();
    m_dispatcher.reset();
    m_collisionConfig.reset();
}

//--------------------------------
// シミュレーションの実行
//--------------------------------
void PhysicsManagerImpl::simulate(float deltaTime)
{
    if (m_dynamicsWorld != nullptr)
    {
        m_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);

        // 衝突リストをクリア
        m_collisionEvents.clear();

        // 衝突ペア（Manifold）走査
        int numManifolds = m_dispatcher->getNumManifolds();
        for (int cnt = 0; cnt < numManifolds; ++cnt)
        {
            btPersistentManifold* contactManifold = m_dispatcher->getManifoldByIndexInternal(cnt);

            // 接触点があるか確認
            int numContacts = contactManifold->getNumContacts();
            if (numContacts > 0)
            {
                // 剛体を取得
                const btCollisionObject* obA = contactManifold->getBody0();
                const btCollisionObject* obB = contactManifold->getBody1();

                // IDを取り出す
                uint64_t idA = reinterpret_cast<uint64_t>(obA->getUserPointer());
                uint64_t idB = reinterpret_cast<uint64_t>(obB->getUserPointer());

                // リストに追加
                m_collisionEvents.push_back({ idA, idB });
            }
        }
    }
}

//--------------------------------
// 重力の設定
//--------------------------------
void PhysicsManagerImpl::setGravity(const Vector3& gravity)
{
    m_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

//--------------------------------
// 剛体の追加
//--------------------------------
void PhysicsManagerImpl::addRigidBody(uint64_t id, CollisionShapeType shapeType, Transform offset, bool isTrigger, RigidBodyType bodyType, float mass, CollisionGroup collisionGroup, CollisionGroup collisionMask)
{
    // 形状の作成
    btCollisionShape* shape{};
    switch (shapeType)
    {
    case CollisionShapeType::Plane:
        shape = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
        break;
    case CollisionShapeType::Box:
        shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        break;
    case CollisionShapeType::Sphere:
        shape = new btSphereShape(0.5f);
        break;
    case CollisionShapeType::Capsule:
        shape = new btCapsuleShape(0.5f, 1.0f);
        break;
    case CollisionShapeType::Mesh:
        // 未実装
        return;
    }

    // 衝突形状マップに追加
    m_collisionShapes.try_emplace(id, shape);

    // 姿勢（位置と回転）の設定
    btTransform initialTransform;
    initialTransform.setIdentity();
    initialTransform.setOrigin(btVector3(offset.position.x, offset.position.y, offset.position.z));
    initialTransform.setRotation(btQuaternion(offset.rotation.x, offset.rotation.y, offset.rotation.z, offset.rotation.w));
    btDefaultMotionState* motionState = new btDefaultMotionState(initialTransform);

    // 動的剛体の場合、慣性を計算
    btVector3 localInertia(0, 0, 0);
    if (bodyType == RigidBodyType::Dynamic && mass > 0.0f)
    {
        m_collisionShapes[id]->calculateLocalInertia(mass, localInertia);
    }
    else
    {
        mass = 0.0f; // Dyanamic以外は質量0 (強制)
    }

    // 剛体 (RigidBody) の作成
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, m_collisionShapes[id], localInertia);
    btRigidBody* body = new btRigidBody(rigidBodyCI);

    // キネマティックな物体の設定
    if (bodyType == RigidBodyType::Kinematic)
    {
        // Kinematicな物体として設定
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

        // 計算省略モード無効化
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    // トリガー設定
    if (isTrigger)
    {
        // 衝突応答（反発）なし
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }

    // ユーザーポインタにIDを設定
    body->setUserPointer(reinterpret_cast<void*>(id));

    // マップ追加
    m_rigidBodies.try_emplace(id, body);

    // ワールドに追加
    m_dynamicsWorld->addRigidBody(body, static_cast<int>(collisionGroup), static_cast<int>(collisionMask));
}

//--------------------------------
// 剛体の削除
//--------------------------------
void PhysicsManagerImpl::removeRigidBody(uint64_t id)
{
    // 剛体と形状の削除
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        m_dynamicsWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body;
        m_rigidBodies.erase(id);
    }
    if (m_collisionShapes.contains(id))
    {
        btCollisionShape* shape = m_collisionShapes[id];
        delete shape;
        m_collisionShapes.erase(id);
    }
}

//--------------------------------
// 力の追加
//--------------------------------
void PhysicsManagerImpl::addForce(uint64_t id, const Vector3& force, bool isImpulse)
{
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        body->activate(true);

        btVector3 btForce(force.x, force.y, force.z);
        if (isImpulse)
        {
            // インパルス（瞬間的な力）
            body->applyCentralImpulse(btForce);
        }
        else
        {
            // フォース（継続的な力）
            body->applyCentralForce(btForce);
        }
    }
}

//-------------------------------------
// トルクの追加
//-------------------------------------
void PhysicsManagerImpl::addTorque(uint64_t id, const Vector3& torque, bool isImpulse)
{
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        body->activate(true);

        btVector3 btTorque(torque.x, torque.y, torque.z);
        if (isImpulse)
        {
            // インパルス（瞬間的な力）
            body->applyTorqueImpulse(btTorque);
        }
        else
        {
            // フォース（継続的な力）
            body->applyTorque(btTorque);
        }
    }
}

//-------------------------------------
// 直線速度の設定
//-------------------------------------
void PhysicsManagerImpl::setLinearVelocity(uint64_t id, const Vector3& velocity)
{
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        body->activate(true);

        body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
}

//-------------------------------------
// 角速度の設定
//-------------------------------------
void PhysicsManagerImpl::setAngularVelocity(uint64_t id, const Vector3& velocity)
{
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        body->activate(true);

        body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
}

//-------------------------------------
// 指定IDの剛体のTransformを設定
//-------------------------------------
void PhysicsManagerImpl::setTransform(uint64_t id, const Transform& transform, bool isResetForces, bool isUpdateMass)
{
    if (m_rigidBodies.contains(id))
    {
        // スケールの設定
        btRigidBody* body = m_rigidBodies[id];
        btCollisionShape* shape = body->getCollisionShape();
        shape->setLocalScaling(btVector3(transform.scale.x, transform.scale.y, transform.scale.z));
        m_dynamicsWorld->updateSingleAabb(body);

        // 質量の更新
        if (isUpdateMass)
        {
            btScalar mass = body->getMass();
            if (mass > 0.0f)
            {
                btVector3 localInertia(0, 0, 0);
                shape->calculateLocalInertia(mass, localInertia);
                body->setMassProps(mass, localInertia);
            }
        }

        // 位置と回転の設定
        btTransform btTransform;
        btTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
        btTransform.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));
        body->setWorldTransform(btTransform);
        body->getMotionState()->setWorldTransform(btTransform);

        // 力のリセット
        if (isResetForces)
        {
            body->setLinearVelocity(btVector3(0, 0, 0));
            body->setAngularVelocity(btVector3(0, 0, 0));
            body->clearForces();
        }
        body->activate(true); // 物理演算を有効化
    }
}

//-------------------------------------
// 指定IDの剛体の材質を設定
//-------------------------------------
void PhysicsManagerImpl::setMaterial(uint64_t id, float friction, float restitution)
{
    if (m_rigidBodies.contains(id))
    {
        btRigidBody* body = m_rigidBodies[id];
        body->setFriction(friction);
        body->setRestitution(restitution);
    }
}

//-------------------------------------
// 指定IDの剛体のTransformを取得
//-------------------------------------
Transform PhysicsManagerImpl::getTransform(uint64_t id)
{
    Transform result{};
    if (m_rigidBodies.contains(id))
    {
        // 位置と回転はMotionStateから取得
        btRigidBody* body = m_rigidBodies[id];
        btTransform transform;
        body->getMotionState()->getWorldTransform(transform);
        btVector3 origin = transform.getOrigin();
        btQuaternion rotation = transform.getRotation();
        result.position = Vector3(origin.getX(), origin.getY(), origin.getZ());
        result.rotation = Quaternion(rotation.getX(), rotation.getY(), rotation.getZ(), rotation.getW());

        // スケールはCollisionShapeから取得
        btCollisionShape* shape = body->getCollisionShape();
        btVector3 localScaling = shape->getLocalScaling();
        result.scale = Vector3(localScaling.getX(), localScaling.getY(), localScaling.getZ());
    }
    return result;
}

//-------------------------------------
// レイキャスト
//-------------------------------------
RayHitInfo PhysicsManagerImpl::rayCast(const Vector3& start, const Vector3& end)
{
    btVector3 s(start.x, start.y, start.z);
    btVector3 e(end.x, end.y, end.z);

    // 一番手前の物体だけを検知する設定
    btCollisionWorld::ClosestRayResultCallback rayCallback(s, e);

    m_dynamicsWorld->rayTest(s, e, rayCallback);

    RayHitInfo info{};
    info.hasHit = rayCallback.hasHit();

    if (info.hasHit)
    {
        info.hitPoint = Vector3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
        info.hitNormal = Vector3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());

        if (rayCallback.m_collisionObject)
        {
            info.hitBodyID = reinterpret_cast<uint64_t>(rayCallback.m_collisionObject->getUserPointer());
        }
    }
    return info;
}

//--------------------------------
// 外部インターフェース
//--------------------------------
PhysicsManager::PhysicsManager() : m_impl{ std::make_unique<PhysicsManagerImpl>() } {}
PhysicsManager::~PhysicsManager() = default;

void PhysicsManager::init()
{
    m_impl = std::make_unique<PhysicsManagerImpl>();
    m_impl->init();
}

void PhysicsManager::uninit()
{
    m_impl->uninit();
    m_impl.reset();
}

void PhysicsManager::simulate(float deltaTime)
{
    m_impl->simulate(deltaTime);
}

void PhysicsManager::setGravity(const Vector3& gravity)
{
    m_impl->setGravity(gravity);
}

void PhysicsManager::addRigidBody(uint64_t id, CollisionShapeType shapeType, Transform offset, bool isTrigger, RigidBodyType bodyType, float mass, CollisionGroup collisionGroup, CollisionGroup collisionMask)
{
    m_impl->addRigidBody(id, shapeType, offset, isTrigger, bodyType, mass, collisionGroup, collisionMask);
}

void PhysicsManager::removeRigidBody(uint64_t id)
{
    m_impl->removeRigidBody(id);
}

void PhysicsManager::addForce(size_t id, const Vector3& force, bool isImpulse)
{
    m_impl->addForce(id, force, isImpulse);
}

void PhysicsManager::addTorque(uint64_t id, const Vector3& torque, bool isImpulse)
{
    m_impl->addTorque(id, torque, isImpulse);
}

void PhysicsManager::setLinearVelocity(uint64_t id, const Vector3& velocity)
{
    m_impl->setLinearVelocity(id, velocity);
}

void PhysicsManager::setAngularVelocity(uint64_t id, const Vector3& velocity)
{
    m_impl->setAngularVelocity(id, velocity);
}

void PhysicsManager::setTransform(uint64_t id, const Transform& transform, bool isResetForces, bool isUpdateMass)
{
    m_impl->setTransform(id, transform, isResetForces, isUpdateMass);
}

void PhysicsManager::setMaterial(uint64_t id, float friction, float restitution)
{
    m_impl->setMaterial(id, friction, restitution);
}

Transform PhysicsManager::getTransform(size_t id)
{
    return m_impl->getTransform(id);
}

const std::vector<CollisionData>& PhysicsManager::getCollisionEvents() const
{
    return m_impl->getCollisionEvents();
}

RayHitInfo PhysicsManager::rayCast(const Vector3& start, const Vector3& end)
{
    return m_impl->rayCast(start, end);
}
