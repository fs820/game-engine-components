#include "camera.h"
#include "renderer.h"
#include "easing.h"

void Camera::Set()
{
    // ビュー行列の計算
    m_viewMatrix = Matrix::LookAtLH(m_position, m_target, m_up);

    // プロジェクション行列の計算
    m_projectionMatrix = Matrix::PerspectiveFovLH(m_fovY, m_aspectRatio, m_nearClip, m_farClip);
}

void Camera::Move(float deltaTime, const Vector2& direction, float speed, float inertiaFactor)
{
    // 移動量の計算
    Vector2 deltaMove = direction * speed * deltaTime;

    // 移動の適用
    switch (m_pivot)
    {
    case Camera::Pivot::None:
        m_targetPosition += Vector3{deltaMove.x, 0.0f, deltaMove.y};
        m_position = Vector3::Lerp(m_position, m_targetPosition, inertiaFactor); // 平行移動
        break;
    case Camera::Pivot::Position:
    case Camera::Pivot::Target:
        m_targetTheta += deltaMove.x;
        m_targetPhi += deltaMove.y;

        m_theta = math::lerpTheta(m_theta, m_targetTheta, inertiaFactor);  // 回転移動
        m_phi = math::lerp(m_phi, m_targetPhi, inertiaFactor);             // 回転移動
        break;
    }

    // 角度の正規化
    m_targetTheta = math::normalizeTheta(m_targetTheta);
    m_targetPhi = math::normalizePhi(m_targetPhi);
    m_theta = math::normalizeTheta(m_theta);
    m_phi = math::normalizePhi(m_phi);

    m_radius = std::clamp(m_radius, m_minRadius, m_maxRadius); // 半径の制限

    switch (m_pivot)
    {
    case Camera::Pivot::None:
    case Camera::Pivot::Position:
        m_target = m_position + Vector3::FromSpherical(m_radius, m_theta, m_phi); // 注視点の更新
        break;
    case Camera::Pivot::Target:
        m_target = Vector3::Lerp(m_target, m_destinationTarget, inertiaFactor);   // 注視点の更新
        m_position = m_target + Vector3::FromSpherical(m_radius, m_theta, m_phi); // カメラ位置の更新
        break;
    }
}

void Camera::SetPivotPosition(const Vector3& position, bool isImmediate)
{
    // ピボット位置の設定
    switch (m_pivot)
    {
    case Camera::Pivot::None:
    case Camera::Pivot::Position:
        m_position = position;
        break;
    case Camera::Pivot::Target:
        isImmediate ? m_target = position : m_destinationTarget = position;
        break;
    }
}
