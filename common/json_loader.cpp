//--------------------------------------------
//
// Jsonローダー [json_loader.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "json_loader.h"
#include "text_loader.h"

namespace file
{
    //-----------------------
    // JsonをMapに変換
    //-----------------------
    std::unordered_map<std::string, std::string> JsonToMap(const Json::Value& root)
    {
        std::unordered_map<std::string, std::string> map;

        for (const auto& key : root.getMemberNames())
        {
            map[key] = root[key].asString();
        }

        return map;
    }

    //-----------------------
    // MapをJsonに変換
    //-----------------------
    Json::Value MapToJson(const std::unordered_map<std::string, std::string>& map)
    {
        Json::Value root;

        for (const auto& [key, value] : map)
        {
            root[key] = value;
        }

        return root;
    }

    //---------------
    // 読み込み
    //---------------
    Json::Value loadJson(const std::filesystem::path& path)
    {
        // ReadText関数でファイル内容を読み込む
        std::string jsonContent = loadText(path);
        if (jsonContent.empty())
        {
            return {};
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;

        // 文字列からパースする
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(jsonContent.c_str(), jsonContent.c_str() + jsonContent.length(), &root, &errs))
        {
            // 失敗
            return {};
        }

        return root;
    }

    //----------------
    // 書き込み
    //----------------
    bool saveJson(const std::filesystem::path& path, const Json::Value& json)
    {
        // アウトプットファイル作成（バイナリ）
        std::ofstream file(path, std::ofstream::binary);
        if (!NativeFile::Exists(path)) NativeFile::CreateDir(path.parent_path());

        if (!file.is_open())
        {// 開けない
            return {};
        }

        Json::StreamWriterBuilder writer; // 設定
        writer["indentation"] = "  ";     // インデント設定

        Json::StreamWriter* jsonWriter(writer.newStreamWriter()); // ライターの生成

        // 書き込み
        if (jsonWriter->write(json, &file) != 0)
        {// 失敗
            delete jsonWriter; // ライター破棄
            return false;
        }

        delete jsonWriter; // ライター破棄
        return true;
    }
}
