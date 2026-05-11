//--------------------------------------------
//
// 頂点 [vertex.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "mesh.h"
#include "renderer.h"
#include "graphics_types.h"
#include "mymath.h"

using namespace mesh;

//-------------
// 2Dスプライト
//-------------
MeshHandle MeshManager::sprite()
{
    // メッシュ情報
    MeshDesc desc{};
    desc.type = MeshType::Sprite;

    // 作ってあるならそれを返す
    if (m_cache.contains(desc)) return m_cache[desc];

    std::vector<Vertex2D> vertices;    // 頂点データ
    std::vector<unsigned int> indices; // インデックスデータ

    // サイズ指定
    vertices.resize(QUAD_VERTEX);
    indices.resize(POLYGON_VERTEX * 2u);

    // 頂点データの初期化
    vertices[0] = Vertex2D{ Vector2{ -0.5f, -0.5f }, Vector2{ 0.0f, 0.0f } };
    vertices[1] = Vertex2D{ Vector2{  0.5f, -0.5f }, Vector2{ 1.0f, 0.0f } };
    vertices[2] = Vertex2D{ Vector2{  0.5f,  0.5f }, Vector2{ 1.0f, 1.0f } };
    vertices[3] = Vertex2D{ Vector2{ -0.5f,  0.5f }, Vector2{ 0.0f, 1.0f } };

    // インデックスデータの初期化
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

    // メッシュの作成
    m_cache.try_emplace(desc, m_renderer.createMesh(VertexShaderType::Vertex2D, vertices.data(), vertices.size(), indices.data(), indices.size()));
    return m_cache[desc];
}

//-------------
// 3D面
//-------------
MeshHandle MeshManager::quad(float texUMax, float texVMax)
{
    // メッシュ情報
    MeshDesc desc{};
    desc.type = MeshType::Quad;
    desc.texUMax = texUMax;
    desc.texVMax = texVMax;

    // 作ってあるならそれを返す
    if (m_cache.contains(desc)) return m_cache[desc];

    std::vector<Vertex3D> vertices;    // 頂点データ
    std::vector<unsigned int> indices; // インデックスデータ

    // サイズ指定
    vertices.resize(QUAD_VERTEX);
    indices.resize(POLYGON_VERTEX * 2u);

    // 頂点データの初期化
    vertices[0] = Vertex3D{ Vector3{ -0.5f, 0.5f,0.0f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[1] = Vertex3D{ Vector3{  0.5f, 0.5f,0.0f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ texUMax, 0.0f } };
    vertices[2] = Vertex3D{ Vector3{  0.5f, -0.5f,0.0f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ texUMax, texVMax } };
    vertices[3] = Vertex3D{ Vector3{ -0.5f, -0.5f,0.0f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ 0.0f, texVMax } };

    // インデックスデータの初期化
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

    // メッシュの作成
    m_cache.try_emplace(desc, m_renderer.createMesh(VertexShaderType::Vertex3D, vertices.data(), vertices.size(), indices.data(), indices.size()));
    return m_cache[desc];
}

//-------------
// 箱
//-------------
MeshHandle MeshManager::box(float texUMax, float texVMax)
{
    // メッシュ情報
    MeshDesc desc{};
    desc.type = MeshType::Box;
    desc.texUMax = texUMax;
    desc.texVMax = texVMax;

    // 作ってあるならそれを返す
    if (m_cache.contains(desc)) return m_cache[desc];

    std::vector<Vertex3D> vertices;    // 頂点データ
    std::vector<unsigned int> indices; // インデックスデータ

    // サイズ指定
    vertices.resize(QUAD_VERTEX * 6u);
    indices.resize(POLYGON_VERTEX * 2u * 6u);

    // 頂点データの初期化

    // 前
    vertices[0] = Vertex3D{ Vector3{ -0.5f, 0.5f,-0.5f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[1] = Vertex3D{ Vector3{  0.5f, 0.5f,-0.5f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ texUMax, 0.0f } };
    vertices[2] = Vertex3D{ Vector3{  0.5f, -0.5f,-0.5f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ texUMax, texVMax } };
    vertices[3] = Vertex3D{ Vector3{ -0.5f, -0.5f,-0.5f },Vector3{ 0.0f, 0.0f,-1.0f }, Vector2{ 0.0f, texVMax } };

    // 左
    vertices[4] = Vertex3D{ Vector3{ -0.5f, 0.5f,0.5f },Vector3{ -1.0f, 0.0f,0.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[5] = Vertex3D{ Vector3{ -0.5f, 0.5f,-0.5f },Vector3{ -1.0f, 0.0f,0.0f}, Vector2{ texUMax, 0.0f } };
    vertices[6] = Vertex3D{ Vector3{ -0.5f, -0.5f,-0.5f },Vector3{ -1.0f, 0.0f,0.0f }, Vector2{ texUMax, texVMax } };
    vertices[7] = Vertex3D{ Vector3{ -0.5f, -0.5f,0.5f },Vector3{ -1.0f, 0.0f,0.0f }, Vector2{ 0.0f, texVMax } };

    // 奥
    vertices[8] = Vertex3D{ Vector3{ 0.5f, 0.5f,0.5f },Vector3{ 0.0f, 0.0f,1.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[9] = Vertex3D{ Vector3{ -0.5f, 0.5f,0.5f },Vector3{ 0.0f, 0.0f,1.0f }, Vector2{ texUMax, 0.0f } };
    vertices[10] = Vertex3D{ Vector3{ -0.5f, -0.5f,0.5f },Vector3{ 0.0f, 0.0f,1.0f }, Vector2{ texUMax, texVMax } };
    vertices[11] = Vertex3D{ Vector3{ 0.5f, -0.5f,0.5f },Vector3{ 0.0f, 0.0f,1.0f }, Vector2{ 0.0f, texVMax } };

    // 右
    vertices[12] = Vertex3D{ Vector3{ 0.5f, 0.5f,-0.5f },Vector3{ 1.0f, 0.0f,0.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[13] = Vertex3D{ Vector3{ 0.5f, 0.5f,0.5f },Vector3{ 1.0f, 0.0f,0.0f}, Vector2{ texUMax, 0.0f } };
    vertices[14] = Vertex3D{ Vector3{ 0.5f, -0.5f,0.5f },Vector3{ 1.0f, 0.0f,0.0f }, Vector2{ texUMax, texVMax } };
    vertices[15] = Vertex3D{ Vector3{ 0.5f, -0.5f,-0.5f },Vector3{ 1.0f, 0.0f,0.0f }, Vector2{ 0.0f, texVMax } };

    // 上
    vertices[16] = Vertex3D{ Vector3{ -0.5f, 0.5f,0.5f },Vector3{ 0.0f, 1.0f,0.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[17] = Vertex3D{ Vector3{  0.5f, 0.5f,0.5f },Vector3{ 0.0f, 1.0f,0.0f }, Vector2{ texUMax, 0.0f } };
    vertices[18] = Vertex3D{ Vector3{  0.5f, 0.5f,-0.5f },Vector3{ 0.0f, 1.0f,0.0f }, Vector2{ texUMax, texVMax } };
    vertices[19] = Vertex3D{ Vector3{ -0.5f, 0.5f,-0.5f },Vector3{ 0.0f, 1.0f,0.0f }, Vector2{ 0.0f, texVMax } };

    // 下
    vertices[20] = Vertex3D{ Vector3{ 0.5f, -0.5f,0.5f },Vector3{ 0.0f, -1.0f,0.0f }, Vector2{ 0.0f, 0.0f } };
    vertices[21] = Vertex3D{ Vector3{ -0.5f, -0.5f,0.5f },Vector3{ 0.0f, -1.0f,0.0f }, Vector2{ texUMax, 0.0f } };
    vertices[22] = Vertex3D{ Vector3{ -0.5f, -0.5f,-0.5f },Vector3{ 0.0f, -1.0f,0.0f }, Vector2{ texUMax, texVMax } };
    vertices[23] = Vertex3D{ Vector3{ 0.5f, -0.5f,-0.5f },Vector3{ 0.0f, -1.0f,0.0f }, Vector2{ 0.0f, texVMax } };

    // インデックスデータの初期化
    unsigned int vertexOff = 0;
    size_t indexOff = 0;
    for (size_t cnt = 0; cnt < 6u; ++cnt, vertexOff += 4u, indexOff += 6u)
    {
        indices[0 + indexOff] = 0 + vertexOff;
        indices[1 + indexOff] = 1 + vertexOff;
        indices[2 + indexOff] = 2 + vertexOff;
        indices[3 + indexOff] = 2 + vertexOff;
        indices[4 + indexOff] = 3 + vertexOff;
        indices[5 + indexOff] = 0 + vertexOff;
    }

    // メッシュの作成
    m_cache.try_emplace(desc, m_renderer.createMesh(VertexShaderType::Vertex3D, vertices.data(), vertices.size(), indices.data(), indices.size()));
    return m_cache[desc];
}

//-------------
// 円柱
//-------------
MeshHandle MeshManager::cylinder(float texUMax, float texVMax, unsigned int splits, bool isInward, bool isCover)
{
    if (splits < 1u) return MeshHandle();

    // メッシュ情報
    MeshDesc desc{};
    desc.type = MeshType::Cylinder;
    desc.texUMax = texUMax;
    desc.texVMax = texVMax;
    desc.splitsTheta = splits;
    desc.isInward = isInward;
    desc.isCover = isCover;

    // 作ってあるならそれを返す
    if (m_cache.contains(desc)) return m_cache[desc];

    std::vector<Vertex3D> vertices;    // 頂点データ
    std::vector<unsigned int> indices; // インデックスデータ

    // サイズ指定
    vertices.resize(2u * (splits + 1u) + isCover * (2u * (splits + 1u + 1u)));
    indices.resize(POLYGON_VERTEX * splits * 2u + isCover * POLYGON_VERTEX * splits * 2u);

    // 分割ごとの角度
    float splitsAngle = math::degreesToRadians(360.0f) / splits;
    float splitsU = texUMax / splits;

    // 頂点データの初期化
    size_t offSet = 0;
    if (isCover)
    {
        vertices[offSet] = Vertex3D{ Vector3{ 0.0f, 0.5f,0.0f },Vector3{ 0.0f, 1.0f + isInward * -2.0f,0.0f }, Vector2{ 0.5f, 0.5f } };
        ++offSet;

        for (size_t cnt = 0; cnt < splits + 1u; ++cnt, ++offSet)
        {
            float angle = 0;    // 頂点方向
            if (isInward)
            {// 内側向き
                angle = -splitsAngle * (cnt % (splits + 1));
            }
            else
            {// 外側向き
                angle = splitsAngle * (cnt % (splits + 1));
            }
            angle = math::normalizeTheta(angle);

            Vector2 angleVec{ std::sinf(angle) ,std::cosf(angle) };

            vertices[offSet] = Vertex3D{ Vector3{ angleVec.x * 0.5f, 0.5f,angleVec.y * 0.5f},Vector3{0.0f, 1.0f + isInward * -2.0f,0.0f}, Vector2{0.5f + angleVec.x * 0.5f, 0.5f + angleVec.y * 0.5f } };
        }
    }
    for (size_t cnt = 0; cnt < splits * 2u + 2u; ++cnt, ++offSet)
    {
        float angle = 0;    // 頂点方向
        float norAngle = 0; // 法線方向
        if (isInward)
        {// 内側向き
            angle = splitsAngle * (cnt % (splits + 1));
            angle = math::normalizeTheta(angle);
            norAngle = math::normalizeTheta(angle + math::degreesToRadians(180.0f)); // 法線は頂点方向の反対側
        }
        else
        {// 外側向き
            angle = -splitsAngle * (cnt % (splits + 1));
            angle = math::normalizeTheta(angle);
            norAngle = angle; // 法線は頂点方向
        }
        vertices[offSet] = Vertex3D{ Vector3{ std::sinf(angle) * 0.5f, 0.5f - 1.0f * (cnt / (splits + 1)),std::cosf(angle) * 0.5f},Vector3{std::sinf(norAngle), 0.0f,std::cosf(norAngle)}, Vector2{splitsU * (cnt % (splits + 1)), texVMax * (cnt / (splits + 1))} };
    }
    if (isCover)
    {
        for (size_t cnt = 0; cnt < splits + 1u; ++cnt, ++offSet)
        {
            float angle = 0;    // 頂点方向
            if (isInward)
            {// 内側向き
                angle = splitsAngle * (cnt % (splits + 1));
            }
            else
            {// 外側向き
                angle = -splitsAngle * (cnt % (splits + 1));
            }
            angle = math::normalizeTheta(angle);

            Vector2 angleVec{ std::sinf(angle) ,std::cosf(angle) };

            vertices[offSet] = Vertex3D{ Vector3{ std::sinf(angle) * 0.5f, -0.5f,std::cosf(angle) * 0.5f},Vector3{0.0f, -1.0f + isInward * 2.0f,0.0f}, Vector2{0.5f + angleVec.x * 0.5f, 0.5f + angleVec.y * 0.5f } };
        }

        vertices[offSet] = Vertex3D{ Vector3{ 0.0f, -0.5f,0.0f },Vector3{ 0.0f, -1.0f + isInward * 2.0f,0.0f }, Vector2{ 0.5f, 0.5f } };
    }

    // インデックスデータの初期化
    unsigned int vertexOff = 0;
    size_t indexOff = 0;
    if (isCover)
    {
        for (size_t cnt = 0; cnt < splits; ++cnt)
        {
            indices[cnt * POLYGON_VERTEX] = 0;
            indices[cnt * POLYGON_VERTEX + 1] = 1 + int(cnt);
            indices[cnt * POLYGON_VERTEX + 2] = 2 + int(cnt);
        }
        vertexOff+= splits + 1u + 1u;
        indexOff += splits * POLYGON_VERTEX;
    }
    for (size_t cnt = 0; cnt < splits; ++cnt, ++vertexOff, indexOff += POLYGON_VERTEX * 2u)
    {
        indices[0 + indexOff] = vertexOff;
        indices[1 + indexOff] = 1 + vertexOff;
        indices[2 + indexOff] = 1 + vertexOff + (splits + 1);
        indices[3 + indexOff] = 1 + vertexOff + (splits + 1);
        indices[4 + indexOff] = vertexOff + (splits + 1);
        indices[5 + indexOff] = vertexOff;
    }
    if (isCover)
    {
        for (size_t cnt = 0; cnt < splits; ++cnt)
        {
            indices[indexOff + cnt * POLYGON_VERTEX] = 2u * splits + 2u + 2u * splits + 2u + 1u;
            indices[indexOff + cnt * POLYGON_VERTEX + 1] = 2u * splits + 2u + splits + 1u + 1u + int(cnt);
            indices[indexOff + cnt * POLYGON_VERTEX + 2] = 2u * splits + 2u + splits + 1u + 1u + 1 + int(cnt);
        }
    }

    // メッシュの作成
    m_cache.try_emplace(desc, m_renderer.createMesh(VertexShaderType::Vertex3D, vertices.data(), vertices.size(), indices.data(), indices.size()));
    return m_cache[desc];
}

//-------------
// 球
//-------------
MeshHandle MeshManager::sphere(float texUMax, float texVMax, unsigned int splitsTheta, unsigned int splitsPhi, bool isInward, bool ishalfDome)
{
    if (splitsTheta < 1u || splitsPhi < 1u) return MeshHandle();

    // メッシュ情報
    MeshDesc desc{};
    desc.type = MeshType::Sphere;
    desc.texUMax = texUMax;
    desc.texVMax = texVMax;
    desc.splitsTheta = splitsTheta;
    desc.splitsPhi = splitsPhi;
    desc.isInward = isInward;
    desc.ishalfDome = ishalfDome;

    // 作ってあるならそれを返す
    if (m_cache.contains(desc)) return m_cache[desc];

    std::vector<Vertex3D> vertices;    // 頂点データ
    std::vector<unsigned int> indices; // インデックスデータ

    // 分割ごとの角度
    float splitsThetaAngle, splitsPhiAngle, splitsU, splitsV;
    if (ishalfDome)
    {// 半球
        splitsPhi /= 2u;

        splitsThetaAngle = math::degreesToRadians(360.0f) / splitsTheta;
        splitsPhiAngle = math::degreesToRadians(90.0f) / splitsPhi;
    }
    else
    {
        splitsThetaAngle = math::degreesToRadians(360.0f) / splitsTheta;
        splitsPhiAngle = math::degreesToRadians(180.0f) / splitsPhi;
    }

    splitsU = texUMax / splitsTheta;
    splitsV = texVMax / splitsPhi;

    // サイズ指定
    vertices.resize((splitsTheta + 1u) * (splitsPhi + 1u));
    indices.resize((POLYGON_VERTEX * 2u) * splitsTheta * splitsPhi);

    // 頂点データの初期化
    size_t offSet = 0;
    float thetaAngle = 0.0f; // 方位角
    float phiAngle = 0.0f;   // 天頂角
    float U = 0.0f;          // U
    float V = 0.0f;          // V

    // 北極
    for (size_t cnt = 0; cnt < splitsTheta + 1u; ++cnt, ++offSet)
    {
        // UV計算
        U = splitsU * cnt;

        Vector3 pos = Vector3::FromSpherical(0.5f, thetaAngle, phiAngle);
        vertices[offSet] = Vertex3D{ pos, pos - pos * 2.0f * float(isInward), Vector2{ U, 0.0f } };

        if (cnt != splitsTheta)
        {// 方位角を進める
            if (isInward)
            {// 内側向き
                thetaAngle += splitsThetaAngle;
            }
            else
            {// 外側向き
                thetaAngle -= splitsThetaAngle;
            }
            thetaAngle = math::normalizeTheta(thetaAngle);
            U += splitsU;
        }
    }

    // 天頂角を進める
    phiAngle += splitsPhiAngle;
    phiAngle = math::normalizePhi(phiAngle, 0.0f);

    for (size_t cntPhi = 0; cntPhi < (splitsPhi - !ishalfDome); ++cntPhi)
    {
        // UV計算
        V = splitsV * (cntPhi + 1u);
        U = 0.0f; // リセット

        for (size_t cntTheta = 0; cntTheta < (splitsTheta + 1u); ++cntTheta, ++offSet)
        {
            Vector3 pos = Vector3::FromSpherical(0.5f, thetaAngle, phiAngle);
            vertices[offSet] = Vertex3D{ pos, pos - pos * 2.0f * float(isInward), Vector2{ U, V } };

            if (cntTheta != splitsTheta)
            {// 方位角を進める
                if (isInward)
                {// 内側向き
                    thetaAngle -= splitsThetaAngle;
                }
                else
                {// 外側向き
                    thetaAngle += splitsThetaAngle;
                }
                thetaAngle = math::normalizeTheta(thetaAngle);
                U += splitsU;
            }
        }

        // 天頂角を進める
        phiAngle += splitsPhiAngle;
        phiAngle = math::normalizePhi(phiAngle, 0.0f);
    }
    if (!ishalfDome)
    {
        U = 0.0f; // リセット

        for (size_t cnt = 0; cnt < splitsTheta + 1u; ++cnt, ++offSet)
        {
            Vector3 pos = Vector3::FromSpherical(0.5f, thetaAngle, phiAngle);
            vertices[offSet] = Vertex3D{ pos, pos - pos * 2.0f * float(isInward), Vector2{ U, texVMax } };

            if (cnt != splitsTheta)
            {// 方位角を進める
                if (isInward)
                {// 内側向き
                    thetaAngle -= splitsThetaAngle;
                }
                else
                {// 外側向き
                    thetaAngle += splitsThetaAngle;
                }
                thetaAngle = math::normalizeTheta(thetaAngle);
                U += splitsU;
            }
        }
    }

    // インデックスデータの初期化
    unsigned int vertexOff = 0;
    size_t indexOff = 0;
    for (size_t ring = 0; ring < splitsPhi; ++ring)
    {
        for (size_t thetaCnt = 0; thetaCnt < splitsTheta; ++thetaCnt, indexOff += POLYGON_VERTEX * 2u)
        {
            unsigned int v0 = vertexOff + unsigned int(thetaCnt);
            unsigned int v1 = v0 + 1;
            unsigned int v2 = v0 + (splitsTheta + 1);
            unsigned int v3 = v1 + (splitsTheta + 1);

            indices[0 + indexOff] = v0;
            indices[1 + indexOff] = v1;
            indices[2 + indexOff] = v3;
            indices[3 + indexOff] = v3;
            indices[4 + indexOff] = v2;
            indices[5 + indexOff] = v0;
        }
        vertexOff += (splitsTheta + 1u);
    }

    // メッシュの作成
    m_cache.try_emplace(desc, m_renderer.createMesh(VertexShaderType::Vertex3D, vertices.data(), vertices.size(), indices.data(), indices.size()));
    return m_cache[desc];
}
