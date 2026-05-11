//--------------------------------------------
//
// プレイヤー [player.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <memory>

class GameObject;
class ModelManager;
struct ModelHandle;
class Renderer;
struct Transform;

namespace factory
{
    std::unique_ptr<GameObject> createPlayer(ModelManager& modelManager, Renderer& renderer, ModelHandle texture, Transform transform, float offsetModelScale = 1.0f);
}
