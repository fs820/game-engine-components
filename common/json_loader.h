//--------------------------------------------
//
// Jsonローダー [json_loader.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "native_file.h"
#include <json/json.h>

namespace file
{
    // JsonをMapに変換
    std::unordered_map<std::string, std::string> JsonToMap(const Json::Value& root);
    // MapをJsonに変換
    Json::Value MapToJson(const std::unordered_map<std::string, std::string>& map);

    // 読み込み
    Json::Value loadJson(const std::filesystem::path& path);
    // 書き込み
    bool saveJson(const std::filesystem::path& path, const Json::Value& json);
}
