//--------------------------------------------
//
// 描画用コンポーネント [render.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"
#include "graphics_types.h"

//-------------------------------------
// 描画用コンポーネントクラス [抽象]
//-------------------------------------
class RenderComponent : public Component
{
public:
    RenderComponent(const RenderQueueMask& renderQueueMask, const RasMode& rasMode);
    virtual ~RenderComponent();

    virtual void render(Renderer& renderer) override = 0;

    void setRenderQueueMask(const RenderQueueMask& renderQueueMask) { m_renderQueueMask = renderQueueMask; }
    RenderQueueMask getRenderQueueMask() const { return m_renderQueueMask; }
    void setRasMode(RasMode mode) { m_rasMode = mode; }
    RasMode getRasMode() { return m_rasMode; }

private:
    RenderQueueMask m_renderQueueMask;
    RasMode m_rasMode;
};
