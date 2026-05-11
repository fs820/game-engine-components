//--------------------------------------------
//
// 入力 [input.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "math_types.h"

class Window;
struct SDL_Window;
union SDL_Event;
struct SDL_Gamepad;

constexpr float DEFAULT_DEAD_ZONE = 0.2f;         // デッドゾーンのデフォルト値
constexpr float DEFAULT_TRIGGER_THRESHOLD = 0.5f; // トリガーのデフォルト閾値
constexpr int MAX_GAMEPADS = 4;                   // 最大ゲームパッド数

//-------------------------
// キーコードの列挙型
//-------------------------
enum class KeyCode: unsigned char
{
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, LeftShift, RightShift, LeftCtrl, RightCtrl,
    LeftAlt, RightAlt, Tab, Backspace, CapsLock,
    Left, Right, Up, Down,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count
};

//-------------------------
// マウスボタンの列挙型
//-------------------------
enum class MouseButtonCode : unsigned char
{
    Left, Right, Middle, Count
};

//-------------------------
// ゲームパッドボタンの列挙型
//-------------------------
enum class GamepadButtonCode : unsigned char
{
    South, East, West, North, LeftShoulder, RightShoulder,
    LeftTrigger, RightTrigger, Select, Start, LeftStick, RightStick,
    Up, Down, Left, Right,
    Count
};

//-------------------------
// アクションコードの列挙型
//-------------------------
enum class ActionCode : unsigned char
{
    MoveForward, MoveBackward, MoveLeft, MoveRight, LookUp, LookDown, LookLeft, LookRight, Jump, Attack, Move, Look, Zoom, Count
};

//-------------------------
// キーボードの状態を表す構造体
//-------------------------
struct Keyboard
{
    std::array<bool, static_cast<size_t>(KeyCode::Count)> currentKeyState;  // 現在のキー状態
    std::array<bool, static_cast<size_t>(KeyCode::Count)> previousKeyState; // 前回のキー状態

    Keyboard() = default;
    ~Keyboard() = default;
};

//-------------------------
// マウスの状態を表す構造体
//-------------------------
struct Mouse
{
    std::array<bool, static_cast<size_t>(MouseButtonCode::Count)> currentButtonState;  // 現在のボタン状態
    std::array<bool, static_cast<size_t>(MouseButtonCode::Count)> previousButtonState; // 前回のボタン状態
    Vector2 pos;                                                   // 座標
    Vector2 relPos;                                                // 相対座標
    Vector2 wheel;                                                 // ホイールのスクロール量

    Mouse() : currentButtonState{}, previousButtonState{}, pos{}, relPos{}, wheel{} {}
    ~Mouse() = default;
};

//-------------------------
// ゲームパッドの状態を表す構造体
//-------------------------
struct Gamepad
{
    SDL_Gamepad* device; // デバイス

    std::array<bool, static_cast<size_t>(GamepadButtonCode::Count)> currentButtonState;  // 現在のボタン状態
    std::array<bool, static_cast<size_t>(GamepadButtonCode::Count)> previousButtonState; // 前回のボタン状態
    Vector2 leftStick;                                               // 左スティックのX軸
    Vector2 rightStick;                                              // 右スティックのX軸
    float leftTrigger;                                               // 左トリガーの値
    float rightTrigger;                                              // 右トリガーの値

    Gamepad(SDL_Gamepad* device) : device(device), currentButtonState{}, previousButtonState{}, leftStick{}, rightStick{}, leftTrigger(0.0f), rightTrigger(0.0f) {}
    ~Gamepad() = default;
};

//--------------------------------------
//
// 入力クラス
//
//--------------------------------------
class Input
{
public:
    Input(Window& window);
    ~Input() = default;

    void loadConfig(std::filesystem::path configFile);

    void update();
    bool handleEvent(SDL_Event* event);

    void setRelativeMouseMode(bool enable);
    void setDeadZone(float deadZone) { m_deadZone = deadZone; }
    void setTriggerThreshold(float triggerThreshold) { m_triggerThreshold = triggerThreshold; }
    void swapGamepad(size_t srcId, size_t destId);

    bool isKeyPressed(KeyCode key) const;
    bool isKeyReleased(KeyCode key) const;
    bool isKeyDown(KeyCode key) const;

    bool isMouseButtonPressed(MouseButtonCode button) const;
    bool isMouseButtonReleased(MouseButtonCode button) const;
    bool isMouseButtonDown(MouseButtonCode button) const;

    bool isGamepadButtonPressed(GamepadButtonCode button, size_t id = 0u) const;
    bool isGamepadButtonReleased(GamepadButtonCode button, size_t id = 0u) const;
    bool isGamepadButtonDown(GamepadButtonCode button, size_t id = 0u) const;

    bool isActionPressed(ActionCode action, size_t id = 0u) const;
    bool isActionReleased(ActionCode action, size_t id = 0u) const;
    bool isActionDown(ActionCode action, size_t id = 0u) const;

    Vector2 getAxis2D(ActionCode action) const;
    float getAxis1D(ActionCode action) const;

    Vector2 getGamePadLeftStickValue(size_t id = 0u) const;
    Vector2 getGamePadRightStickValue(size_t id = 0u) const;
    float getGamePadLeftTriggerValue(size_t id = 0u) const;
    float getGamePadRightTriggerValue(size_t id = 0u) const;
    Vector2 getMousePosition() const;
    Vector2 getMouseRelative() const;
    Vector2 getMouseWheel() const;

    bool getRelativeMouseMode() const;

private:
    SDL_Window* m_pWindow; // ウインドウへのポインタ

    std::unordered_map<ActionCode, KeyCode> m_actionKeyConfig;                                  // アクションとキーのバインディング
    std::unordered_map<ActionCode, MouseButtonCode> m_actionMouseConfig;                        // アクションとマウスボタンのバインディング
    std::unordered_map<ActionCode, std::pair<GamepadButtonCode, size_t>> m_actionGamepadConfig; // アクションとゲームパッドボタンのバインディング

    Keyboard m_keyboard;             // キーボードの状態
    Mouse m_mouse;                   // マウスの状態
    std::vector<Gamepad> m_gamepads; // ゲームパッドの状態

    float m_deadZone;         // スティックのデッドゾーン
    float m_triggerThreshold; // トリガーの閾値
};
