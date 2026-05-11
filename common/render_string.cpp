//--------------------------------------------
//
// String描画用コンポーネント [render_string.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "render_string.h"
#include "renderer.h"
#include "object.h"
#include "log.h"

//-------------------------------------
// 
// Mesh描画用コンポーネントクラス
// 
//-------------------------------------

//----------------------------
// 描画
//----------------------------
void StringRenderComponent::render(Renderer& renderer)
{
    // 描画
    renderer.drawString(m_string, m_pos, m_color, m_angle, m_scale);
}
