//--------------------------------------------
//
// ビルボードコンポーネント [billboard_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"

class Camera; // カメラ

//-------------------------------------
// ビルボードコンポーネントクラス
//-------------------------------------
class BillboardComponent : public Component
{
public:
    BillboardComponent(const Camera& camera, bool isLookForward = false, bool isYAxisOnly = false)
        : m_camera(camera), m_isLookForward(isLookForward), m_isYAxisOnly(isYAxisOnly) {}
    virtual ~BillboardComponent() = default;

    void update(float deltaTime) override;

private:
    const Camera& m_camera;
    bool m_isLookForward;
    bool m_isYAxisOnly;
};
