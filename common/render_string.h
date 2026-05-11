//--------------------------------------------
//
// String描画用コンポーネント [render_string.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <string>
#include <string_view>
#include "render.h"

//-------------------------------------
// String描画用コンポーネントクラス
//-------------------------------------
class StringRenderComponent : public RenderComponent
{
public:
    StringRenderComponent(std::string_view string, Vector2 pos, Color color = Color::White(), float angle = {}, Vector2 scale = Vector2::One())
        : RenderComponent(RenderQueueMask::String, RasMode::None), m_string(string), m_pos(pos), m_color(color), m_angle(angle), m_scale(scale) {}
    ~StringRenderComponent() = default;

    void render(Renderer& renderer) override;

    void setMeshHandle(std::string_view string) { m_string = string; }
    void setPosition(const Vector2& pos) { m_pos = pos; }
    void setColor(const Color& color) { m_color = color; }
    void setAngle(float angle) { m_angle = angle; }
    void setScale(const Vector2& scale) { m_scale = scale; }

private:
    std::string m_string; // 描画するstring
    Vector2 m_pos;        // 描画位置
    Color m_color;        // 色
    float m_angle;        // 角度
    Vector2 m_scale;      // スケール
};
