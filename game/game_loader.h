//--------------------------------------------
//
// Gameローダー [game_loader.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <string>
#include <filesystem>

struct Config
{
    std::string title; // タイトル
    int windowWidth;   // ウィンドウ幅
    int windowHeight;  // ウィンドウ高さ
};

namespace file
{
    // 読み込み
    Config loadConfig(const std::filesystem::path& path);
}
