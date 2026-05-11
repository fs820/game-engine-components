//--------------------------------------------
//
// 入力 [input.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "input.h"
#include <SDL3/SDL.h>
#include "log.h"
#include "gui.h"
#include "json_loader.h"
#include "window.h"

namespace
{
    // キーコードとSDLのスキャンコードのマッピング
    const std::unordered_map<KeyCode, SDL_Scancode> keyCodeToScancode =
    {
        { KeyCode::A, SDL_SCANCODE_A },
        { KeyCode::B, SDL_SCANCODE_B },
        { KeyCode::C, SDL_SCANCODE_C },
        { KeyCode::D, SDL_SCANCODE_D },
        { KeyCode::E, SDL_SCANCODE_E },
        { KeyCode::F, SDL_SCANCODE_F },
        { KeyCode::G, SDL_SCANCODE_G },
        { KeyCode::H, SDL_SCANCODE_H },
        { KeyCode::I, SDL_SCANCODE_I },
        { KeyCode::J, SDL_SCANCODE_J },
        { KeyCode::K, SDL_SCANCODE_K },
        { KeyCode::L, SDL_SCANCODE_L },
        { KeyCode::M, SDL_SCANCODE_M },
        { KeyCode::N, SDL_SCANCODE_N },
        { KeyCode::O, SDL_SCANCODE_O },
        { KeyCode::P, SDL_SCANCODE_P },
        { KeyCode::Q, SDL_SCANCODE_Q },
        { KeyCode::R, SDL_SCANCODE_R },
        { KeyCode::S, SDL_SCANCODE_S },
        { KeyCode::T, SDL_SCANCODE_T },
        { KeyCode::U, SDL_SCANCODE_U },
        { KeyCode::V, SDL_SCANCODE_V },
        { KeyCode::W, SDL_SCANCODE_W },
        { KeyCode::X, SDL_SCANCODE_X },
        { KeyCode::Y, SDL_SCANCODE_Y },
        { KeyCode::Z, SDL_SCANCODE_Z },
        { KeyCode::Num0, SDL_SCANCODE_0 },
        { KeyCode::Num1, SDL_SCANCODE_1 },
        { KeyCode::Num2, SDL_SCANCODE_2 },
        { KeyCode::Num3, SDL_SCANCODE_3 },
        { KeyCode::Num4, SDL_SCANCODE_4 },
        { KeyCode::Num5, SDL_SCANCODE_5 },
        { KeyCode::Num6, SDL_SCANCODE_6 },
        { KeyCode::Num7, SDL_SCANCODE_7 },
        { KeyCode::Num8, SDL_SCANCODE_8 },
        { KeyCode::Num9, SDL_SCANCODE_9 },
        { KeyCode::Space, SDL_SCANCODE_SPACE },
        { KeyCode::Enter, SDL_SCANCODE_RETURN },
        { KeyCode::Escape, SDL_SCANCODE_ESCAPE },
        { KeyCode::LeftShift, SDL_SCANCODE_LSHIFT },
        { KeyCode::RightShift, SDL_SCANCODE_RSHIFT },
        { KeyCode::LeftCtrl, SDL_SCANCODE_LCTRL },
        { KeyCode::RightCtrl, SDL_SCANCODE_RCTRL },
        { KeyCode::LeftAlt, SDL_SCANCODE_LALT },
        { KeyCode::RightAlt, SDL_SCANCODE_RALT },
        { KeyCode::Tab, SDL_SCANCODE_TAB },
        { KeyCode::Backspace, SDL_SCANCODE_BACKSPACE },
        { KeyCode::CapsLock, SDL_SCANCODE_CAPSLOCK },
        { KeyCode::Left, SDL_SCANCODE_LEFT },
        { KeyCode::Right, SDL_SCANCODE_RIGHT },
        { KeyCode::Up, SDL_SCANCODE_UP },
        { KeyCode::Down, SDL_SCANCODE_DOWN },
        { KeyCode::F1, SDL_SCANCODE_F1 },
        { KeyCode::F2, SDL_SCANCODE_F2 },
        { KeyCode::F3, SDL_SCANCODE_F3 },
        { KeyCode::F4, SDL_SCANCODE_F4 },
        { KeyCode::F5, SDL_SCANCODE_F5 },
        { KeyCode::F6, SDL_SCANCODE_F6 },
        { KeyCode::F7, SDL_SCANCODE_F7 },
        { KeyCode::F8, SDL_SCANCODE_F8 },
        { KeyCode::F9, SDL_SCANCODE_F9 },
        { KeyCode::F10, SDL_SCANCODE_F10 },
        { KeyCode::F11, SDL_SCANCODE_F11 },
        { KeyCode::F12, SDL_SCANCODE_F12 },
        { KeyCode::Count, SDL_SCANCODE_UNKNOWN }
    };

    // マウスボタンコードとSDLのマウスボタンフラグのマッピング
    const std::unordered_map<MouseButtonCode, Uint32> mouseButtonCodeToFlag
    {
        { MouseButtonCode::Left, SDL_BUTTON_LEFT },
        { MouseButtonCode::Right, SDL_BUTTON_RIGHT },
        { MouseButtonCode::Middle, SDL_BUTTON_MIDDLE },
        { MouseButtonCode::Count, -1 }
    };

    // ゲームパッドボタンコードとSDLのゲームパッドボタンのマッピング
    const std::unordered_map<GamepadButtonCode, SDL_GamepadButton> gamepadButtonCodeToButton
    {
        { GamepadButtonCode::South, SDL_GAMEPAD_BUTTON_SOUTH },
        { GamepadButtonCode::East, SDL_GAMEPAD_BUTTON_EAST },
        { GamepadButtonCode::West, SDL_GAMEPAD_BUTTON_WEST },
        { GamepadButtonCode::North, SDL_GAMEPAD_BUTTON_NORTH },
        { GamepadButtonCode::LeftShoulder, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER },
        { GamepadButtonCode::RightShoulder, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
        { GamepadButtonCode::LeftTrigger, SDL_GAMEPAD_BUTTON_INVALID },
        { GamepadButtonCode::RightTrigger, SDL_GAMEPAD_BUTTON_INVALID },
        { GamepadButtonCode::Select, SDL_GAMEPAD_BUTTON_BACK },
        { GamepadButtonCode::Start, SDL_GAMEPAD_BUTTON_START },
        { GamepadButtonCode::LeftStick, SDL_GAMEPAD_BUTTON_LEFT_STICK },
        { GamepadButtonCode::RightStick, SDL_GAMEPAD_BUTTON_RIGHT_STICK },
        { GamepadButtonCode::Up, SDL_GAMEPAD_BUTTON_DPAD_UP },
        { GamepadButtonCode::Down, SDL_GAMEPAD_BUTTON_DPAD_DOWN },
        { GamepadButtonCode::Left, SDL_GAMEPAD_BUTTON_DPAD_LEFT },
        { GamepadButtonCode::Right, SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
        { GamepadButtonCode::Count, SDL_GAMEPAD_BUTTON_INVALID }
    };

    // 文字列からキーコードへのマッピング
    const std::unordered_map<std::string_view, KeyCode> stringToKeyCode
    {
        { "A", KeyCode::A },
        { "B", KeyCode::B },
        { "C", KeyCode::C },
        { "D", KeyCode::D },
        { "E", KeyCode::E },
        { "F", KeyCode::F },
        { "G", KeyCode::G },
        { "H", KeyCode::H },
        { "I", KeyCode::I },
        { "J", KeyCode::J },
        { "K", KeyCode::K },
        { "L", KeyCode::L },
        { "M", KeyCode::M },
        { "N", KeyCode::N },
        { "O", KeyCode::O },
        { "P", KeyCode::P },
        { "Q", KeyCode::Q },
        { "R", KeyCode::R },
        { "S", KeyCode::S },
        { "T", KeyCode::T },
        { "U", KeyCode::U },
        { "V", KeyCode::V },
        { "W", KeyCode::W },
        { "X", KeyCode::X },
        { "Y", KeyCode::Y },
        { "Z", KeyCode::Z },
        { "Num0", KeyCode::Num0 },
        { "Num1", KeyCode::Num1 },
        { "Num2", KeyCode::Num2 },
        { "Num3", KeyCode::Num3 },
        { "Num4", KeyCode::Num4 },
        { "Num5", KeyCode::Num5 },
        { "Num6", KeyCode::Num6 },
        { "Num7", KeyCode::Num7 },
        { "Num8", KeyCode::Num8 },
        { "Num9", KeyCode::Num9 },
        { "Space", KeyCode::Space },
        { "Enter", KeyCode::Enter },
        { "Escape", KeyCode::Escape },
        { "LeftShift", KeyCode::LeftShift },
        { "RightShift", KeyCode::RightShift },
        { "LeftCtrl", KeyCode::LeftCtrl },
        { "RightCtrl", KeyCode::RightCtrl },
        { "LeftAlt", KeyCode::LeftAlt },
        { "RightAlt", KeyCode::RightAlt },
        { "Tab", KeyCode::Tab },
        { "Backspace", KeyCode::Backspace },
        { "CapsLock", KeyCode::CapsLock },
        { "Left", KeyCode::Left },
        { "Right", KeyCode::Right },
        { "Up", KeyCode::Up },
        { "Down", KeyCode::Down },
        { "F1", KeyCode::F1 },
        { "F2", KeyCode::F2 },
        { "F3", KeyCode::F3 },
        { "F4", KeyCode::F4 },
        { "F5", KeyCode::F5 },
        { "F6", KeyCode::F6 },
        { "F7", KeyCode::F7 },
        { "F8", KeyCode::F8 },
        { "F9", KeyCode::F9 },
        { "F10", KeyCode::F10 },
        { "F11", KeyCode::F11 },
        { "F12", KeyCode::F12 }
    };

    // 文字列からマウスボタンコードへのマッピング
    const std::unordered_map<std::string_view, MouseButtonCode> stringToMouseButtonCode
    {
        { "Left", MouseButtonCode::Left },
        { "Right", MouseButtonCode::Right },
        { "Middle", MouseButtonCode::Middle }
    };

    // 文字列からゲームパッドボタンコードへのマッピング
    const std::unordered_map<std::string_view, GamepadButtonCode> stringToGamepadButtonCode
    {
        { "South", GamepadButtonCode::South },
        { "East", GamepadButtonCode::East },
        { "West", GamepadButtonCode::West },
        { "North", GamepadButtonCode::North },
        { "LeftShoulder", GamepadButtonCode::LeftShoulder },
        { "RightShoulder", GamepadButtonCode::RightShoulder },
        { "LeftTrigger", GamepadButtonCode::LeftTrigger },
        { "RightTrigger", GamepadButtonCode::RightTrigger },
        { "Select", GamepadButtonCode::Select },
        { "Start", GamepadButtonCode::Start },
        { "LeftStick", GamepadButtonCode::LeftStick },
        { "RightStick", GamepadButtonCode::RightStick },
        { "Up", GamepadButtonCode::Up },
        { "Down", GamepadButtonCode::Down },
        { "Left", GamepadButtonCode::Left },
        { "Right", GamepadButtonCode::Right }
    };

    // 文字列からアクションコードへのマッピング
    const std::unordered_map<std::string_view, ActionCode> stringToActionCode
    {
        { "MoveForward", ActionCode::MoveForward },
        { "MoveBackward", ActionCode::MoveBackward },
        { "MoveLeft", ActionCode::MoveLeft },
        { "MoveRight", ActionCode::MoveRight },
        { "LookUp", ActionCode::LookUp },
        { "LookDown", ActionCode::LookDown },
        { "LookLeft", ActionCode::LookLeft },
        { "LookRight", ActionCode::LookRight },
        { "Jump", ActionCode::Jump },
        { "Attack", ActionCode::Attack }
    };
}

//--------------------------------------
//
// 入力クラス
//
//--------------------------------------

Input::Input(Window& window) : m_pWindow{}, m_deadZone(DEFAULT_DEAD_ZONE), m_triggerThreshold(DEFAULT_TRIGGER_THRESHOLD)
{
    m_pWindow = window.getWindow();
}

//-------------------------
// コンフィグの読み込み
//-------------------------
void Input::loadConfig(std::filesystem::path configFile)
{
    // JSONファイルを読み込む
    auto config = file::loadJson(configFile);

    // JSONの構造を解析して、アクションとキー/マウスボタン/ゲームパッドボタンのバインディングを設定する
    for (const auto& type : config.getMemberNames())
    {
        if (type =="Keyboard")
        {
            auto keyboardConfig = config[type];
            for (const auto& key : keyboardConfig.getMemberNames())
            {
                if (!stringToActionCode.contains(key))
                {
                    spdlog::warn("Unknown action: {}", key);
                    continue;
                }
                else if (!stringToKeyCode.contains(keyboardConfig[key].asString()))
                {
                    spdlog::warn("Unknown key: {}", keyboardConfig[key].asString());
                    continue;
                }

                ActionCode action = stringToActionCode.at(key);
                KeyCode keyCode = stringToKeyCode.at(keyboardConfig[key].asString());
                m_actionKeyConfig.try_emplace(action, KeyCode::Count);
                m_actionKeyConfig[action] = keyCode;
            }
        }

        if (type == "Mouse")
        {
            auto mouseConfig = config[type];
            for (const auto& key : mouseConfig.getMemberNames())
            {
                if (!stringToActionCode.contains(key))
                {
                    spdlog::warn("Unknown action: {}", key);
                    continue;
                }
                else if (!stringToMouseButtonCode.contains(mouseConfig[key].asString()))
                {
                    spdlog::warn("Unknown mouse button: {}", mouseConfig[key].asString());
                    continue;
                }

                ActionCode action = stringToActionCode.at(key);
                MouseButtonCode buttonCode = stringToMouseButtonCode.at(mouseConfig[key].asString());
                m_actionMouseConfig.try_emplace(action, MouseButtonCode::Count);
                m_actionMouseConfig[action] = buttonCode;
            }
        }

        if (type == "Gamepad")
        {
            auto id = config[type];
            for (const auto& number : id.getMemberNames())
            {
                for (size_t cnt = 0; cnt < MAX_GAMEPADS; ++cnt)
                {
                    if (number == cnt + "0")
                    {
                        auto gamepadConfig = id[number];
                        for (const auto& key : gamepadConfig.getMemberNames())
                        {
                            if (!stringToActionCode.contains(key))
                            {
                                spdlog::warn("Unknown action: {}", key);
                                continue;
                            }
                            else if (!stringToGamepadButtonCode.contains(gamepadConfig[key].asString()))
                            {
                                spdlog::warn("Unknown gamepad button: {}", gamepadConfig[key].asString());
                                continue;
                            }

                            ActionCode action = stringToActionCode.at(key);
                            GamepadButtonCode buttonCode = stringToGamepadButtonCode.at(gamepadConfig[key].asString());
                            m_actionGamepadConfig.try_emplace(action, GamepadButtonCode::Count, -1);
                            m_actionGamepadConfig[action] = { buttonCode, cnt };
                        }
                    }
                }
            }
        }
    }
}

//-------------------------
// 更新
//-------------------------
void Input::update()
{
    ImGuiIO& io = ImGui::GetIO(); // ImGuiのIOを取得

    // キーボード
    m_keyboard.previousKeyState = m_keyboard.currentKeyState; // 前回の状態を保存
    if (!io.WantCaptureKeyboard)
    {
        // キーボードの状態を取得
        const bool* keyboardState = SDL_GetKeyboardState(nullptr);
        for (size_t count = 0; count < size_t(KeyCode::Count); ++count)
        {
            KeyCode key = KeyCode(count);
            m_keyboard.currentKeyState[static_cast<size_t>(key)] = keyboardState[keyCodeToScancode.at(key)];
            spdlog::trace("Key: {}, State: {}", static_cast<int>(key), m_keyboard.currentKeyState[static_cast<size_t>(key)]);
        }
    }

    // マウス
    m_mouse.relPos.zero(); // 相対座標をリセット
    m_mouse.wheel.zero();  // ホイールをリセット
    m_mouse.previousButtonState = m_mouse.currentButtonState; // 前回の状態を保存
    if (!io.WantCaptureMouse)
    {
        // マウスの位置と状態を取得
        Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
        for (size_t count = 0; count < size_t(MouseButtonCode::Count); ++count)
        {
            MouseButtonCode button = MouseButtonCode(count);
            m_mouse.currentButtonState[static_cast<size_t>(button)] = mouseState & SDL_BUTTON_MASK(mouseButtonCodeToFlag.at(button));
            spdlog::trace("Mouse Button: {}, State: {}", static_cast<int>(button), m_mouse.currentButtonState[static_cast<size_t>(button)]);
        }
    }
    spdlog::trace("Mouse Position: ({}, {}), Relative Movement: ({}, {})", m_mouse.pos.x, m_mouse.pos.y, m_mouse.relPos.x, m_mouse.relPos.y);

    for (Gamepad& gamepad : m_gamepads)
    {
        gamepad.previousButtonState = gamepad.currentButtonState; // 前回の状態を保存

        // ボタン
        for (size_t count = 0; count < size_t(GamepadButtonCode::Count); ++count)
        {
            GamepadButtonCode button = GamepadButtonCode(count);
            if (gamepadButtonCodeToButton.at(button) == SDL_GAMEPAD_BUTTON_INVALID) continue;

            gamepad.currentButtonState[static_cast<size_t>(button)] = SDL_GetGamepadButton(gamepad.device, gamepadButtonCodeToButton.at(button));
            spdlog::trace("Gamepad Button: {}, State: {}", static_cast<int>(button), gamepad.currentButtonState[static_cast<size_t>(button)]);
        }

        // スティック
        Sint16 rawX = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_LEFTX);
        Sint16 rawY = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_LEFTY);
        if (rawX < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.leftStick.x = static_cast<float>(rawX) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.leftStick.x = static_cast<float>(rawX) / 32767.0f;
        }
        if (rawY < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.leftStick.y = static_cast<float>(rawY) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.leftStick.y = static_cast<float>(rawY) / 32767.0f;
        }
        if (gamepad.leftStick.length() < m_deadZone) gamepad.leftStick.zero(); // デッドゾーンを適用
        rawX = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_RIGHTX);
        rawY = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_RIGHTY);
        if (rawX < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.rightStick.x = static_cast<float>(rawX) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.rightStick.x = static_cast<float>(rawX) / 32767.0f;
        }
        if (rawY < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.rightStick.y = static_cast<float>(rawY) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.rightStick.y = static_cast<float>(rawY) / 32767.0f;
        }
        if (gamepad.rightStick.length() < m_deadZone) gamepad.rightStick.zero(); // デッドゾーンを適用
        spdlog::trace("Gamepad Left Stick: ({}, {}), Right Stick: ({}, {})", gamepad.leftStick.x, gamepad.leftStick.y, gamepad.rightStick.x, gamepad.rightStick.y);

        // トリガー
        rawX = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        if (rawX < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.leftTrigger = static_cast<float>(rawX) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.leftTrigger = static_cast<float>(rawX) / 32767.0f;
        }
        rawX = SDL_GetGamepadAxis(gamepad.device, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        if (rawX < 0)
        {
            // 負の値は 32768.0f で割る
            gamepad.rightTrigger = static_cast<float>(rawX) / 32768.0f;
        }
        else
        {
            // 正の値は 32767.0f で割る
            gamepad.rightTrigger = static_cast<float>(rawX) / 32767.0f;
        }

        // トリガーの状態を更新
        gamepad.currentButtonState[static_cast<size_t>(GamepadButtonCode::LeftTrigger)] = (gamepad.leftTrigger >= m_triggerThreshold);
        gamepad.currentButtonState[static_cast<size_t>(GamepadButtonCode::RightTrigger)] = (gamepad.rightTrigger >= m_triggerThreshold);
        spdlog::trace("Gamepad Left Trigger: {}, Right Trigger: {}", gamepad.leftTrigger, gamepad.rightTrigger);
    }
}

//-------------------------
// イベント処理
//-------------------------
bool Input::handleEvent(SDL_Event* event)
{
    // マウス移動
    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        // 絶対座標を更新 (UIなどで使う用)
        m_mouse.pos.x = event->motion.x;
        m_mouse.pos.y = event->motion.y;

        // 移動量を蓄積 (カメラ操作などで使う用)
        m_mouse.relPos.x += event->motion.xrel;
        m_mouse.relPos.y += event->motion.yrel;
    }

    // マウスホイール
    if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        m_mouse.wheel.x += event->wheel.x;
        m_mouse.wheel.y += event->wheel.y;
    }

    // コントローラーの接続
    if (event->type == SDL_EVENT_GAMEPAD_ADDED)
    {
        // リストに追加
        SDL_Gamepad* gamepad = SDL_OpenGamepad(event->gdevice.which);
        if (gamepad != nullptr)
        {
            spdlog::info("コントローラー({})が接続されました。", SDL_GetGamepadName(gamepad));
            m_gamepads.emplace_back(Gamepad(gamepad));
        }
    }
    // コントローラーの切断
    if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
    {
        // リストから削除する
        SDL_Gamepad* gamepad = SDL_GetGamepadFromID(event->gdevice.which);
        for (auto it = m_gamepads.begin(); it != m_gamepads.end(); ++it)
        {
            if (it->device == gamepad)
            {
                m_gamepads.erase(it);
                break;
            }
        }
        SDL_CloseGamepad(gamepad); // デバイスを閉じる

        spdlog::info("コントローラーが切断されました。");
    }

    return true;
}

//-------------------------
// キーの状態を取得
//-------------------------
bool Input::isKeyPressed(KeyCode key) const
{
    return m_keyboard.currentKeyState.at(static_cast<size_t>(key)) && !m_keyboard.previousKeyState.at(static_cast<size_t>(key));
}
bool Input::isKeyReleased(KeyCode key) const
{
    return !m_keyboard.currentKeyState.at(static_cast<size_t>(key)) && m_keyboard.previousKeyState.at(static_cast<size_t>(key));
}
bool Input::isKeyDown(KeyCode key) const
{
    return m_keyboard.currentKeyState.at(static_cast<size_t>(key));
}
bool Input::isMouseButtonPressed(MouseButtonCode button) const
{
    return m_mouse.currentButtonState.at(static_cast<size_t>(button)) && !m_mouse.previousButtonState.at(static_cast<size_t>(button));
}
bool Input::isMouseButtonReleased(MouseButtonCode button) const
{
    return !m_mouse.currentButtonState.at(static_cast<size_t>(button)) && m_mouse.previousButtonState.at(static_cast<size_t>(button));
}
bool Input::isMouseButtonDown(MouseButtonCode button) const
{
    return m_mouse.currentButtonState.at(static_cast<size_t>(button));
}
bool Input::isGamepadButtonPressed(GamepadButtonCode button, size_t id) const
{
    if (m_gamepads.empty()) return false;
    if (id >= m_gamepads.size()) return false;

    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.currentButtonState.at(static_cast<size_t>(button)) && !gamepad.previousButtonState.at(static_cast<size_t>(button));

}
bool Input::isGamepadButtonReleased(GamepadButtonCode button, size_t id) const
{
    if (m_gamepads.empty()) return false;
    if (id >= m_gamepads.size()) return false;

    const Gamepad& gamepad = m_gamepads[id];
    return !gamepad.currentButtonState.at(static_cast<size_t>(button)) && gamepad.previousButtonState.at(static_cast<size_t>(button));
}
bool Input::isGamepadButtonDown(GamepadButtonCode button, size_t id) const
{
    if (m_gamepads.empty()) return false;
    if (id >= m_gamepads.size()) return false;

    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.currentButtonState.at(static_cast<size_t>(button));
}
bool Input::isActionPressed(ActionCode action, size_t id) const
{
    // アクションに対応するキー、マウスボタン、ゲームパッドボタンのいずれかが押されたかをチェックする
    if (m_actionKeyConfig.contains(action))
    {
        KeyCode key = m_actionKeyConfig.at(action);
        if (isKeyPressed(key)) return true;
    }

    if (m_actionMouseConfig.contains(action))
    {
        MouseButtonCode mouseButton = m_actionMouseConfig.at(action);
        if (isMouseButtonPressed(mouseButton)) return true;
    }

    if (!m_actionGamepadConfig.contains(action)) return false;

    auto config = m_actionGamepadConfig.at(action);
    if (id != config.second) return false;
    GamepadButtonCode gamepadButton = config.first;

    return isGamepadButtonPressed(gamepadButton, id);
}
bool Input::isActionReleased(ActionCode action, size_t id) const
{
    // アクションに対応するキー、マウスボタン、ゲームパッドボタンのいずれかが離されたかをチェックする
    if (m_actionKeyConfig.contains(action))
    {
        KeyCode key = m_actionKeyConfig.at(action);
        if (isKeyReleased(key)) return true;
    }

    if (m_actionMouseConfig.contains(action))
    {
        MouseButtonCode mouseButton = m_actionMouseConfig.at(action);
        if (isMouseButtonReleased(mouseButton)) return true;
    }

    if (!m_actionGamepadConfig.contains(action)) return false;

    auto config = m_actionGamepadConfig.at(action);
    if (id != config.second) return false;
    GamepadButtonCode gamepadButton = config.first;

    return isGamepadButtonReleased(gamepadButton, id);
}
bool Input::isActionDown(ActionCode action, size_t id) const
{
    // アクションに対応するキー、マウスボタン、ゲームパッドボタンのいずれかが押されているかをチェックする
    if (m_actionKeyConfig.contains(action))
    {
        KeyCode key = m_actionKeyConfig.at(action);
        if (isKeyDown(key)) return true;
    }

    if (m_actionMouseConfig.contains(action))
    {
        MouseButtonCode mouseButton = m_actionMouseConfig.at(action);
        if (isMouseButtonDown(mouseButton)) return true;
    }

    if (!m_actionGamepadConfig.contains(action)) return false;

    auto config = m_actionGamepadConfig.at(action);
    if (id != config.second) return false;
    GamepadButtonCode gamepadButton = config.first;

    return isGamepadButtonDown(gamepadButton, id);
}

//-------------------
// 抽象軸
//-------------------
Vector2 Input::getAxis2D(ActionCode action) const
{
    switch (action)
    {
    case ActionCode::Move:
    {
        // コントローラー
        Vector2 stick = getGamePadLeftStickValue();
        if (stick.length() >= 0.01f)
        {
            stick.y = -stick.y;
            stick.normalize();
            return stick;
        }

        // キーボード
        bool forward = isActionDown(ActionCode::MoveForward),
            backward = isActionDown(ActionCode::MoveBackward),
            left = isActionDown(ActionCode::MoveLeft),
            right = isActionDown(ActionCode::MoveRight);
        Vector2 axis{};
        if (right) axis.x += 1.0f;
        if (left) axis.x += -1.0f;
        if (backward) axis.y += 1.0f;
        if (forward) axis.y += -1.0f;
        axis.normalize();
        return axis;
    }
    case ActionCode::Look:
    {
        // コントローラー
        Vector2 stick = getGamePadRightStickValue();
        if (stick.length() >= 0.01f)
        {
            stick.y = -stick.y;
            stick.normalize();
            return stick;
        }

        // マウス
        Vector2 mouse = getMouseRelative();
        if (mouse.length() >= 0.01f)
        {
            mouse.y = -mouse.y;
            mouse.normalize();
            return mouse;
        }

        // キーボード
        bool up = isActionDown(ActionCode::LookUp),
            down = isActionDown(ActionCode::LookDown),
            left = isActionDown(ActionCode::LookLeft),
            right = isActionDown(ActionCode::LookRight);
        Vector2 axis{};
        if (right) axis.x += 1.0f;
        if (left) axis.x += -1.0f;
        if (down) axis.y += 1.0f;
        if (up) axis.y += -1.0f;
        axis.normalize();
        return axis;
    }
    default:
        spdlog::warn("ActionCode:{} getAxis2Dに対応するActionではありません", static_cast<int>(action));
        return Vector2::Zero();
    }
}
float Input::getAxis1D(ActionCode action) const
{
    switch (action)
    {
    case ActionCode::Zoom:
    {
        Vector2 wheel = getMouseWheel();
        return wheel.y;
    }
    default:
        spdlog::warn("ActionCode:{} getAxis1Dに対応するActionではありません", static_cast<int>(action));
        return 0.0f;
    }
}

//----------------------
// 具体的なアナログ入力
//-----------------------
Vector2 Input::getGamePadLeftStickValue(size_t id) const
{
    if (m_gamepads.empty()) return Vector2::Zero();
    if (id >= m_gamepads.size()) return Vector2::Zero();
    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.leftStick;
}
Vector2 Input::getGamePadRightStickValue(size_t id) const
{
    if (m_gamepads.empty()) return Vector2::Zero();
    if (id >= m_gamepads.size()) return Vector2::Zero();
    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.rightStick;
}
float Input::getGamePadLeftTriggerValue(size_t id) const
{
    if (m_gamepads.empty()) return 0.0f;
    if (id >= m_gamepads.size()) return 0.0f;
    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.leftTrigger;
}
float Input::getGamePadRightTriggerValue(size_t id) const
{
    if (m_gamepads.empty()) return 0.0f;
    if (id >= m_gamepads.size()) return 0.0f;
    const Gamepad& gamepad = m_gamepads[id];
    return gamepad.rightTrigger;
}
Vector2 Input::getMousePosition() const
{
    return m_mouse.pos;
}
Vector2 Input::getMouseRelative() const
{
    return m_mouse.relPos;
}
Vector2 Input::getMouseWheel() const
{
    return m_mouse.wheel;
}

//----------------------------------------------------------------------------------------------------
// マウスを移動量モードにする (マウスが非表示になりウィンドウに固定される) (視点の回転などに使う)
//----------------------------------------------------------------------------------------------------
void Input::setRelativeMouseMode(bool enable)
{
    if (m_pWindow != nullptr)
    {
        SDL_SetWindowRelativeMouseMode(m_pWindow, enable);
    }
}

//----------------------------------------------------------------------------------------------------
// マウスが移動量モードかを確認する
//----------------------------------------------------------------------------------------------------
bool Input::getRelativeMouseMode() const
{
    if (m_pWindow != nullptr)
    {
        return SDL_GetWindowRelativeMouseMode(m_pWindow);
    }
    return false;
}

//-------------------------
// ゲームパッドの順番を入れ替える
//-------------------------
void Input::swapGamepad(size_t srcId, size_t destId)
{
    if (m_gamepads.size() <= srcId || m_gamepads.size() <= destId) return;
    std::swap(m_gamepads[srcId], m_gamepads[destId]);
}
