//--------------------------------------------
//
// ビルボードコンポーネント [billboard_comp.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "billboard_comp.h"
#include "object.h"
#include "camera.h"
#include "trans_comp.h"
#include "log.h"

//--------------------------------------------
//
// ビルボードコンポーネントクラス
//
//--------------------------------------------

//---------------------------
// 更新
//---------------------------
void BillboardComponent::update(float deltaTime)
{
    // ワールド変換の設定
    Transform transform{};
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    transform = trans->get();

    // 真正面(カメラの見ている方向の逆)を見る
    if (m_isLookForward)
    {
        Matrix viewMat = m_camera.GetViewMatrix();
        viewMat.inverse();
        transform.rotation = viewMat.getQuaternion();
    }
    // カメラ方向を見る
    else
    {
        Vector3 cameraPos = m_camera.GetPosition();
        Vector3 cameraUp = m_camera.GetUp();

        // Y軸のみ回転
        if (m_isYAxisOnly)
        {
            // 自分の高さと同じ
            cameraPos.y = transform.position.y;

            // 真上に固定
            cameraUp = Vector3(0.0f, 1.0f, 0.0f);
        }

        Matrix lookMat = Matrix::LookAtLH(transform.position, cameraPos, cameraUp);
        lookMat.inverse();
        transform.rotation = lookMat.getQuaternion();
    }
    trans->set(transform);
}
