//--------------------------------------------
//
// 頂点 [mesh.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <unordered_map>

class Renderer;
struct MeshHandle;

namespace mesh
{
    constexpr unsigned int QUAD_VERTEX = 4u;
    constexpr unsigned int POLYGON_VERTEX = 3u;
    constexpr unsigned int DEFAULT_SPLITS = 64u;
}

// メッシュタイプ
enum class MeshType : unsigned char
{
    Sprite,
    Quad,
    Box,
    Cylinder,
    Sphere,
    Max
};

// メッシュ
struct MeshDesc
{
    MeshType type;            // タイプ
    float texUMax;            // テクスチャU
    float texVMax;            // テクスチャV
    unsigned int splitsTheta; // 分割数 (方位角)
    unsigned int splitsPhi;   // 分割数 (天頂角)
    bool isInward;            // 内側ポリゴン
    bool isCover;             // cylinderやsphereの天面
    bool ishalfDome;          // sphereを半分にする

    bool operator==(const MeshDesc& rhs) const noexcept
    {
        return type == rhs.type &&
            texUMax == rhs.texUMax &&
            texVMax == rhs.texVMax &&
            splitsTheta == rhs.splitsTheta &&
            splitsPhi == rhs.splitsPhi &&
            isInward == rhs.isInward &&
            isCover == rhs.isCover &&
            ishalfDome == rhs.ishalfDome;
    }

    MeshDesc() : type{}, texUMax{ 1.0f }, texVMax{ 1.0f }, splitsTheta{ mesh::DEFAULT_SPLITS }, splitsPhi{ mesh::DEFAULT_SPLITS }, isInward{}, isCover{}, ishalfDome{} {}
    ~MeshDesc() = default;
};

// 構造体をmapするための定型文
namespace std
{
    template<>
    struct hash<MeshDesc>
    {
        size_t operator()(const MeshDesc& k) const noexcept
        {
            size_t h = 0;

            auto hash_combine = [&h](size_t v)
                {
                    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                };

            hash_combine(std::hash<MeshType>{}(k.type));
            hash_combine(std::hash<float>{}(k.texUMax));
            hash_combine(std::hash<float>{}(k.texVMax));
            hash_combine(std::hash<unsigned int>{}(k.splitsTheta));
            hash_combine(std::hash<unsigned int>{}(k.splitsPhi));
            hash_combine(std::hash<bool>{}(k.isInward));
            hash_combine(std::hash<bool>{}(k.isCover));
            hash_combine(std::hash<bool>{}(k.ishalfDome));

            return h;
        }
    };
}

//----------------
// メッシュクラス
//----------------
class MeshManager
{
public:
    MeshManager(Renderer& renderer) : m_renderer(renderer), m_cache{} {}
    ~MeshManager() = default;

    MeshHandle sprite();
    MeshHandle quad(float texUMax = 1.0f,float texVMax = 1.0f);
    MeshHandle box(float texUMax = 1.0f, float texVMax = 1.0f);
    MeshHandle cylinder(float texUMax = 1.0f, float texVMax = 1.0f, unsigned int splits = mesh::DEFAULT_SPLITS, bool isInward = false, bool isCover = false);
    MeshHandle sphere(float texUMax = 1.0f, float texVMax = 1.0f, unsigned int splitsTheta = mesh::DEFAULT_SPLITS, unsigned int splitsPhi = mesh::DEFAULT_SPLITS, bool isInward = false, bool ishalfDome = false);

private:
    Renderer& m_renderer;                             // レンダラー参照
    std::unordered_map<MeshDesc, MeshHandle> m_cache; // メッシュのキャッシュ
};
