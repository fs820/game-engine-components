#pragma once

#include "imgui.h" // ImGui名前空間の呼び出しを公開

class Window;
class Renderer;

namespace gui
{
    void init(const Window& window, const Renderer& renderer);
    void uninit();
    void beginFrame();
    void endFrame();
    void render();
}
