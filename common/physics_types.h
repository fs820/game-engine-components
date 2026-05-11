//--------------------------------------------
//
// 物理 [physics_types.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "math_types.h"

static constexpr size_t INVALID_ID = ~0;

static constexpr float EARTH_GRAVITY = -9.81f;
static constexpr float MOON_GRAVITY = -1.62f;
static constexpr float MARS_GRAVITY = -3.71f;

// 剛体の種類
enum class RigidBodyType
{
    Static,
    Dynamic,
    Kinematic,
    Max
};

// 衝突形状の種類
enum class CollisionShapeType
{
    Plane,
    Box,
    Sphere,
    Capsule,
    Mesh,
    Max
};

// 衝突グループ
enum class CollisionGroup
{
    Default = 1 << 0,
    Player = 1 << 1,
    Enemy = 1 << 2,
    Environment = 1 << 3,
    Trigger = 1 << 4
};

// 衝突情報（誰と誰がぶつかったか）
struct CollisionData
{
    uint64_t idA; // ぶつかった物体AのID
    uint64_t idB; // ぶつかった物体BのID
};

// レイヒット情報
struct RayHitInfo
{
    bool hasHit;
    Vector3 hitPoint;
    Vector3 hitNormal;
    uint64_t hitBodyID;
};

inline CollisionGroup operator|(CollisionGroup lhs, CollisionGroup rhs)
{
    return static_cast<CollisionGroup>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
        );
}

inline CollisionGroup& operator|=(CollisionGroup& lhs, CollisionGroup rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline CollisionGroup operator&(CollisionGroup lhs, CollisionGroup rhs)
{
    return static_cast<CollisionGroup>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
        );
}

inline CollisionGroup& operator&=(CollisionGroup& lhs, CollisionGroup rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline CollisionGroup operator^(CollisionGroup lhs, CollisionGroup rhs)
{
    return static_cast<CollisionGroup>(
        static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs)
        );
}

inline CollisionGroup& operator^=(CollisionGroup& lhs, CollisionGroup rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

inline CollisionGroup operator~(CollisionGroup mask)
{
    return static_cast<CollisionGroup>(
        ~static_cast<uint32_t>(mask)
        );
}
