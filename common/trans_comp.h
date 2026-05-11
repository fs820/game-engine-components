//--------------------------------------------
//
// Transformコンポーネント [trans_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"
#include "math_types.h"

//-------------------------------------
// Transformコンポーネントクラス
//-------------------------------------
class TransformComponent : public Component
{
public:
    TransformComponent(const Transform& transform) : m_transform(transform) {}
    virtual ~TransformComponent() = default;

    void set(const Transform& transform) { m_transform = transform; }
    Transform get() const { return m_transform; }

private:
    Transform m_transform; // オブジェクトの位置、回転、スケールを表すTransform
};
