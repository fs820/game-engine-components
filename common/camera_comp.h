//--------------------------------------------
//
// Cameraコンポーネント [camera_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"
#include "camera.h"

//-------------------------------------
// Cameraコンポーネントクラス
//-------------------------------------
class CameraComponent : public Component
{
public:
    CameraComponent(const Camera& camera) : m_camera(camera) {}
    virtual ~CameraComponent() = default;

    void set(const Camera& camera) { m_camera = camera; }
    Camera& get() { return m_camera; }

private:
    Camera m_camera; // オブジェクトの位置、回転、スケールを表すTransform
};
