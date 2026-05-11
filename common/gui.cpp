#include "gui.h"

// バックエンドを隠蔽
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

#include "window.h"   // Window
#include "renderer.h" // renderer

namespace gui
{
    void init(const Window& window, const Renderer& renderer)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // キーボード有効化

        // スタイル:ダークモード
        ImGui::StyleColorsDark();

        io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\meiryo.ttc",  // フォント (メイリオ)
            18.0f,                             // フォントサイズ (18)
            nullptr,
            io.Fonts->GetGlyphRangesJapanese() // 日本語
        );

        // バックエンド初期化
        ImGui_ImplSDL3_InitForD3D(window.getWindow());                    // SDL3
        ImGui_ImplDX11_Init(renderer.getDevice(), renderer.getContext()); // DX11
    }

    void uninit()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame()
    {
        // 新しいフレームを開始
        ImGui_ImplDX11_NewFrame(); // レンダラ
        ImGui_ImplSDL3_NewFrame(); // プラットフォーム
        ImGui::NewFrame();
    }

    void endFrame()
    {
        // 内部描画リストを生成
        ImGui::Render();
    }

    void render()
    {
        // 描画
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}
