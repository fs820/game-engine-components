//--------------------------------------------
//
// ウインドウ [window.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once

class Renderer;
class Window;
class  Input;

namespace gui
{
    void init(const Window& window, const Renderer& renderer);
}

struct SDL_Window;
struct SDL_Cursor;
union SDL_Event;

// SDLのウィンドウのデリータ
struct SDLWindowDeleter
{
    void operator()(SDL_Window* window) const;
};

// SDLのカーソルのデリータ
struct SDLCursorDeleter
{
    void operator()(SDL_Cursor* cursor) const;
};

//-------------------------
// ウインドウクラス
//-------------------------
class Window
{
public:
    Window();
    ~Window();

    bool init(const char* title, int width, int height);
    void uninit();
    bool handleEvent(SDL_Event* event);

    void setIcon(const std::filesystem::path& iconPath);
    void setCursor(const std::filesystem::path& cursorPath, int hotspotX = 0, int hotspotY = 0);

    void setTitle(const char* title);
    void setSize(int width, int height);
    void setFullscreen(bool fullscreen);
    void setCursorVisible(bool visible);

    void* getNativeWindow() const { return m_pNativeWindow; }

private:
    // ↓ friend
    friend class Input;
    friend void gui::init(const Window& window, const Renderer& renderer);
    SDL_Window* getWindow() const;
    // ↑

    std::unique_ptr<SDL_Window, SDLWindowDeleter> m_pWindow; // SDLウインドウ
    void* m_pNativeWindow;                                   // 生のウィンドウ

    std::unique_ptr<SDL_Cursor, SDLCursorDeleter> m_pCursor; // カーソル
};
