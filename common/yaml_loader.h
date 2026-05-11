//--------------------------------------------
//
// Yamlローダー [yaml_loader.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "native_file.h"
#pragma warning(push)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace file
{
    // 読み込み
    YAML::Node loadYaml(const std::filesystem::path& path);
}
