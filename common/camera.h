#pragma once
#include "mymath.h"

class Renderer;
class Camera
{
public:
    Camera() : m_minRadius(1.0f),
        m_maxRadius(10.0f),
        m_pivot(Pivot::Target),
        m_radius(5.0f),
        m_theta(0.0f),
        m_phi(90.0f),
        m_position(0.0f, 1.5f, -5.0f),
        m_target(0.0f, 1.5f, 0.0f),
        m_destinationTarget(0.0f, 1.5f, 0.0f),
        m_up(0.0f, 1.0f, 0.0f),
        m_fovY(math::degreesToRadians(55.0f)),
        m_aspectRatio(16.0f / 9.0f),
        m_nearClip(0.1f),
        m_farClip(1200.0f),
        m_viewMatrix(),
        m_projectionMatrix(),
        m_targetTheta(0.0f),
        m_targetPhi(90.0f),
        m_targetPosition(0.0f, 1.5f, -5.0f)
    {}
    ~Camera() = default;

    // 回転の基準点
    enum class Pivot : unsigned char
    {
        None,     // 平行移動
        Position, // カメラ位置を中心に回転
        Target    // 注視点を中心に回転
    };

    void Set();
    void Move(float deltaTime, const Vector2& direction, float speed, float inertiaFactor = 0.1f);
    void AddRadius(float deltaTime, float radius) { m_radius += radius * deltaTime; }

    void SetPivot(Pivot pivot) { m_pivot = pivot; }
    void SetPivotPosition(const Vector3& position, bool isImmediate = false);
    void SetMinRadius(float radius) { m_minRadius = radius; }
    void SetMaxRadius(float radius) { m_maxRadius = radius; }
    void SetRadius(float radius) { m_radius = radius; }
    void SetTheta(float theta) { m_theta = theta; m_targetTheta = theta; }
    void SetPhi(float phi) { m_phi = phi; m_targetPhi = phi; }
    void SetUp(const Vector3& up) { m_up = up; }
    void SetFovY(float fovY) { m_fovY = fovY; }
    void SetAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    void SetNearClip(float nearClip) { m_nearClip = nearClip; }
    void SetFarClip(float farClip) { m_farClip = farClip; }
    const Matrix& GetViewMatrix() const { return m_viewMatrix; }
    const Matrix& GetProjectionMatrix() const { return m_projectionMatrix; }
    const Vector3& GetPosition() const { return m_position; }
    const Vector3& GetTarget() const { return m_target; }
    const Vector3& GetUp() const { return m_up; }
    float GetMinRadius() const { return m_minRadius; }
    float GetMaxRadius() const { return m_maxRadius; }
    float GetRadius() const { return m_radius; }
    float GetTheta() const { return m_theta; }
    float GetPhi() const { return m_phi; }

private:
    float m_minRadius; // 最小の距離
    float m_maxRadius; // 最大の距離

    Pivot m_pivot;             // 回転の基準点
    float m_radius;            // 球座標系の半径
    float m_theta;             // 球座標系の角度
    float m_phi;               // 球座標系の角度
    Vector3 m_position;        // カメラ位置
    Vector3 m_target;          // 注視点
    Vector3 m_up;              // 上方向ベクトル
    float m_fovY;              // 垂直視野角（ラジアン）
    float m_aspectRatio;       // アスペクト比
    float m_nearClip;          // ニアクリップ距離
    float m_farClip;           // ファークリップ距離
    Matrix m_viewMatrix;       // ビュー行列
    Matrix m_projectionMatrix; // プロジェクション行列

    Vector3 m_destinationTarget; // 注視点移動先
    float m_targetTheta;         // 球座標系の角度
    float m_targetPhi;           // 球座標系の角度
    Vector3 m_targetPosition;    // カメラ位置
};                            
