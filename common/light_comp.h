//--------------------------------------------
//
// Lightコンポーネント [light_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"
#include "graphics_types.h"

//-------------------------------------
// Lightコンポーネントクラス
//-------------------------------------
class LightComponent : public Component
{
public:
    LightComponent(const LightData& renderQueue, bool isShadowCast = false, float radius = {}, float theta = {}, float phi = {}, const Vector3& shadowTarget = {}, const Vector3& shadowUp = {}) : m_data(renderQueue), m_isShadowCast(isShadowCast), m_radius(radius), m_theta(theta), m_phi(phi), m_shadowTarget(shadowTarget), m_shadowUp(shadowUp) {}
    virtual ~LightComponent() = default;

    void set(const LightData& light) { m_data = light; }
    LightData get() const { return m_data; }

    void setShadowInfo(bool isShadowCast, float radius, float theta, float phi, const Vector3& shadowTarget, const Vector3& shadowUp)
    {
        m_isShadowCast = isShadowCast; m_radius = radius; m_theta = theta; m_phi = phi; m_shadowTarget = shadowTarget; m_shadowUp = shadowUp;
    }
    bool getShadowInfo(float* radius = nullptr, float* theta = nullptr, float* phi = nullptr, Vector3* shadowTarget = nullptr, Vector3* shadowUp = nullptr)
    {
        if (radius != nullptr) *radius = m_radius; if (theta != nullptr) *theta = m_theta; if (phi != nullptr) *phi = m_phi; if (shadowTarget != nullptr) *shadowTarget = m_shadowTarget; if (shadowUp != nullptr) *shadowUp = m_shadowUp;
        return m_isShadowCast;
    }

private:
    LightData m_data;       // ライト
    bool m_isShadowCast;    // 影を落とすかどうか

    // 影を落とす場合
    float m_radius; // 距離
    float m_theta;  // 方位角
    float m_phi;    // 天頂角

    Vector3 m_shadowTarget; // 注視点
    Vector3 m_shadowUp;     // 上
};
