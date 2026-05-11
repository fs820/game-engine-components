//--------------------------------------------
//
// テキストローダー [text_loader.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "text_loader.h"

namespace file
{
    //---------------
    // 読み込み
    //---------------
    std::string loadText(const std::filesystem::path& path)
    {
        // サイズを取得
        size_t fileSize = NativeFile::GetSize(path);
        if (fileSize == 0) return "";

        NativeFile file(path, FileMode::ReadText);
        if (!file.isOpen()) return "";

        // 領域を確保
        std::string content;
        content.resize(fileSize);

        // 書き込む
        if (file.read(content.data(), fileSize) <= fileSize)
        {
            return content;
        }

        return "";
    }

    //----------------
    // 書き込み
    //----------------
    bool saveText(const std::filesystem::path& path, std::string_view content)
    {
        NativeFile file(path, FileMode::WriteText);
        return file.write(content.data(), content.size());
    }
}
