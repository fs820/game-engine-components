//--------------------------------------------
//
// テキストローダー [text_loader.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "native_file.h"
#include <string>

namespace file
{
    // 読み込み
    std::string loadText(const std::filesystem::path& path);

    // 書き込み
    bool saveText(const std::filesystem::path& path, std::string_view content);
}
