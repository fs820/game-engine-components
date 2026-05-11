//--------------------------------------------
//
// Yamlローダー [yaml_loader.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "yaml_loader.h"

namespace file
{
    //----------------
    // 読み込み
    //----------------
    YAML::Node loadYaml(const std::filesystem::path& path)
    {
        // YAMLファイルの読み込み
        return YAML::LoadFile(path.string()); // ノードを返す
    }
}
