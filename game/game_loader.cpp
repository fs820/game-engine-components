//--------------------------------------------
//
// Gameローダー [game_loader.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "game_loader.h"
#include "yaml_loader.h"

namespace file
{
    //---------------
    // 読み込み
    //---------------
    Config loadConfig(const std::filesystem::path& path)
    {
        YAML::Node node = loadYaml(path); // YAMLファイルの読み込み

        // ノードからConfig構造体に変換
        Config config{};
        YAML::Node window = node["Window"];
        config.title = window["Title"].as<std::string>();
        config.windowWidth = window["Width"].as<int>();
        config.windowHeight = window["Height"].as<int>();

        return config;
    }
}
