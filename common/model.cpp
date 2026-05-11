//--------------------------------------------
//
// モデル [model.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "model.h"
#include "texture.h"
#include "renderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// DirectX用に変換する (左手座標系,UV反転,カリング対策),ポリゴンをすべて三角形に変換,タンジェントとバイタンジェントを計算,スキニング,法線がない場合に生成
constexpr unsigned int LOAD_FLAGS{ aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_GenNormals };

// 階層構造を持つノード
struct Node
{
    std::string name;        // ノード名
    Matrix defaultTransform; // デフォルトのローカル変換行列

    std::vector<int> meshIndices; // このノード下のメッシュの番号リスト
    std::vector<Node*> children;  // 子ノード
    Node* parent;                 // 親ノード

    Node() : name{}, defaultTransform{}, meshIndices{}, children{}, parent{ nullptr } {}
    ~Node() { for (auto c : children) delete c; }
};

// マテリアルデータ
struct MaterialData
{
    std::string name;            // マテリアル名
    Color diffuseColor;          // ディフューズ色
    Color specularColor;         // スペキュラー色
    Color emissiveColor;         // エミッシブ色
    float shininess;             // スペキュラーの強さ
    int textureIndex;            // m_textures配列へのインデックス (-1ならテクスチャなし)

    MaterialData() : name{ "None" }, diffuseColor{ 1,1,1,1 }, specularColor{ 0,0,0,1 }, emissiveColor{ 0,0,0,1 }, shininess{ 32.0f }, textureIndex(-1) {}
    ~MaterialData() = default;
};

// サブセット（マテリアルごとの描画単位）
struct Subset
{
    unsigned int indexStart;    // インデックスバッファの開始位置
    unsigned int indexCount;    // インデックス数
    unsigned int materialIndex; // 使用するマテリアルの番号

    Subset() : indexStart(0), indexCount(0), materialIndex(0) {}
    ~Subset() = default;
};

// キーフレーム (時間と値のペア)
struct VectorKey { double time; Vector3 value;  VectorKey() : time(0.0), value{} {} VectorKey(double time, Vector3 value) : time(time), value(value) {} ~VectorKey() = default; };
struct QuatKey { double time; Quaternion value;  QuatKey() : time(0.0), value{} {} QuatKey(double time, Quaternion value) : time(time), value(value) {}  ~QuatKey() = default; };

// チャンネル (1つのノードに対応するアニメーションデータ)
struct NodeAnimation
{
    std::string nodeName; // 動かす対象のノード名
    std::vector<VectorKey> positionKeys;
    std::vector<QuatKey>   rotationKeys;
    std::vector<VectorKey> scalingKeys;

    NodeAnimation() : nodeName{}, positionKeys{}, rotationKeys{}, scalingKeys{} {}
    ~NodeAnimation() = default;
};

// アニメーション (1つのモーション全体)
struct Animation
{
    std::string name;
    double duration;        // 全体の長さ(Tick)
    double ticksPerSecond;  // 1秒あたりのTick数
    std::vector<NodeAnimation> channels;

    Animation() : name{}, duration(0.0), ticksPerSecond(0.0), channels{} {}
    ~Animation() = default;
};

// ボーン情報
struct BoneInfo
{
    std::string name;    // ボーン名
    Matrix offsetMatrix; // オフセット行列 (モデル空間 -> ボーン空間)

    BoneInfo() : name{}, offsetMatrix{} {}
    ~BoneInfo() = default;
};

namespace
{
    //--------------
    // Matrix変換
    //--------------
    Matrix convertMatrix(const aiMatrix4x4& src)
    {
        Matrix dest;

        // Row 1
        dest.m[0][0] = src.a1; dest.m[0][1] = src.b1; dest.m[0][2] = src.c1; dest.m[0][3] = src.d1;
        // Row 2
        dest.m[1][0] = src.a2; dest.m[1][1] = src.b2; dest.m[1][2] = src.c2; dest.m[1][3] = src.d2;
        // Row 3
        dest.m[2][0] = src.a3; dest.m[2][1] = src.b3; dest.m[2][2] = src.c3; dest.m[2][3] = src.d3;
        // Row 4
        dest.m[3][0] = src.a4; dest.m[3][1] = src.b4; dest.m[3][2] = src.c4; dest.m[3][3] = src.d4;

        return dest;
    }

    //--------------
    // ベクトル補間
    //--------------
    Vector3 CalcInterpolatedVector(double time, const std::vector<VectorKey>& keys, double duration, bool isLoop, const Vector3& defaultValue)
    {
        if (keys.empty()) return defaultValue;
        if (keys.size() == 1) return keys[0].value;

        // 時間が最初のキーより前なら、最初の値を返す
        if (time < keys[0].time) return keys[0].value;

        for (size_t cnt = 0; cnt < keys.size() - 1; ++cnt)
        {
            if (time < keys[cnt + 1].time)
            {
                double diff = keys[cnt + 1].time - keys[cnt].time;
                if (diff <= 0.0001) return keys[cnt].value;

                float t = (float)((time - keys[cnt].time) / diff);
                return Vector3::Lerp(keys[cnt].value, keys[cnt + 1].value, t);
            }
        }

        if (isLoop)
        {// ループ
            // 「最後のキー」から「(次の周の)最初のキー」への補間を行う
            const auto& lastKey = keys.back();
            const auto& firstKey = keys.front();

            // 補間区間の長さ = (全体の長さ - 最後のキーの時間) + 最初のキーの時間
            double timeToLoop = duration - lastKey.time;
            if (timeToLoop > 0.0001)
            {
                // 現在位置の割合
                float t = (float)((time - lastKey.time) / timeToLoop);
                t = std::clamp(t, 0.0f, 1.0f);

                return Vector3::Lerp(lastKey.value, firstKey.value, t);
            }
        }
        return keys.back().value;
    }

    //--------------
    // クォータニオン補間
    //--------------
    Quaternion CalcInterpolatedRotation(double time, const std::vector<QuatKey>& keys, double duration, bool isLoop, const Quaternion& defaultValue)
    {
        if (keys.empty()) return defaultValue;
        if (keys.size() == 1) return keys[0].value;

        // ★追加: 時間が最初のキーより前なら、最初の値を返す (マイナス回避)
        if (time < keys[0].time) return keys[0].value;

        for (size_t cnt = 0; cnt < keys.size() - 1; ++cnt)
        {
            if (time < keys[cnt + 1].time)
            {
                // ★追加: ゼロ除算対策
                double diff = keys[cnt + 1].time - keys[cnt].time;
                if (diff <= 0.0001) return keys[cnt].value;

                float t = (float)((time - keys[cnt].time) / diff);
                return Quaternion::Slerp(keys[cnt].value, keys[cnt + 1].value, t);
            }
        }

        if (isLoop)
        {// ループ
            const auto& lastKey = keys.back();
            const auto& firstKey = keys.front();
            double timeToLoop = duration - lastKey.time;

            if (timeToLoop > 0.0001)
            {
                float t = (float)((time - lastKey.time) / timeToLoop);
                t = std::clamp(t, 0.0f, 1.0f);

                return Quaternion::Slerp(lastKey.value, firstKey.value, t);
            }
        }
        return keys.back().value;
    }

    //--------------
    // 名前からノードを探す（再帰）
    //--------------
    Node* FindNode(Node* node, const std::string& name)
    {
        if (node->name == name) return node;
        for (auto child : node->children)
        {
            Node* res = FindNode(child, name);
            if (res) return res;
        }
        return nullptr;
    }

    //--------------
    // 名前からプレフィックスを除去して純粋なボーン名を取り出す
    //--------------
    std::string extractBoneName(std::string name)
    {
        // ':' または "___" を探して、その後ろを返す
        size_t pos = name.rfind(':');
        if (pos != std::string::npos) return name.substr(pos + 1);

        pos = name.rfind("___");
        if (pos != std::string::npos) return name.substr(pos + 3);

        return name;
    }

    //--------------
    // 抽出したボーン名からノードを探す（再帰）
    //--------------
    Node* FindNodeofExtract(Node* node, const std::string& name)
    {
        if (extractBoneName(node->name) == extractBoneName(name)) return node;
        for (auto child : node->children)
        {
            Node* res = FindNodeofExtract(child, name);
            if (res) return res;
        }
        return nullptr;
    }
}

//----------------------------
// モデルリソース
//----------------------------
class ModelResource
{
public:
    ModelResource(const std::filesystem::path& path, Renderer& renderer);
    ~ModelResource();

    bool load(TextureManager& textureManager, bool isAnimOnly);
    bool setAnimation(std::span<Animation> anims);
    void unload();

    float getImportScale() const { return m_importScale; }
    Node* getRootNode() const { return m_rootNode; }
    size_t getNumBones() const { return m_boneInfo.size(); }
    BoneInfo* getBoneInfo(size_t index) { return (index < m_boneInfo.size()) ? &m_boneInfo[index] : nullptr; }
    size_t getNumVertices() const { return m_vertices.size(); }
    size_t getNumIndices() const { return m_indices.size(); }
    MeshHandle getMesh() const { return m_mesh; }
    size_t getNumMaterials() const { return m_materials.size(); }
    MaterialData* getMaterial(size_t index) { return (index < m_materials.size()) ? &m_materials[index] : nullptr; }
    size_t getNumSubsets() const { return m_subsets.size(); }
    Subset* getSubset(size_t index) { return (index < m_subsets.size()) ? &m_subsets[index] : nullptr; }
    size_t getNumTextures() const { return m_textures.size(); }
    TextureHandle getTextureHandle(size_t index) const { return (index < m_textures.size()) ? m_textures[index] : TextureHandle(); }
    size_t getNumAnimations() const { return m_animations.size(); }
    Animation* getAnimation(size_t index) { return (index < m_animations.size()) ? &m_animations[index] : nullptr; }
    std::span<Animation> getAnimations() { return m_animations; }
    bool isThisAnimationLoaded(const std::string& name) const;
    bool isSetUpGpu() { return m_mesh.isValid(); }

private:
    void processMaterials(const aiScene* scene, TextureManager& textureManager);
    Node* processNode(aiNode* node, const aiScene* scene, const Matrix& parentTransform);
    void processMesh(aiMesh* mesh, const aiScene* scene, const Matrix& transform);
    void processAnimations(const aiScene* scene);
    void setupMeshs();

    // モデルのファイルパス
    const std::filesystem::path m_path;

    // ルートノード
    Node* m_rootNode;

    // モデルデータのスケーリング値
    float m_importScale;

    // メッシュデータ
    std::vector<VertexModel> m_vertices;   // 頂点データ
    std::vector<unsigned int> m_indices;   // インデックスデータ
    std::vector<MaterialData> m_materials; // マテリアルデータ
    std::vector<Subset> m_subsets;         // サブセット
    std::vector<TextureHandle> m_textures; // テクスチャハンドルリスト

    // ボーンデータ
    std::vector<BoneInfo> m_boneInfo;                       // ボーンリスト (インデックスで管理)
    std::unordered_map<std::string, int> m_boneMapping;     // ボーン名 -> インデックスの検索用

    // アニメーションデータ
    std::vector<Animation> m_animations; // 読み込んだアニメーションリスト

    // GPUリソース
    Renderer& m_renderer; // レンダラー参照
    MeshHandle m_mesh;    // メッシュ
};

static constexpr double DEFAULT_TICKSPERSECOND = 24.0; // デフォルトのTICK

ModelResource::ModelResource(const std::filesystem::path& path, Renderer& renderer) : m_path(path), m_vertices{}, m_indices{}, m_materials{}, m_subsets{}, m_textures{}, m_rootNode{}, m_renderer(renderer), m_animations{}, m_boneInfo{}, m_boneMapping{}, m_importScale{}, m_mesh{} {}
ModelResource::~ModelResource() { unload(); }

//--------------
// モデルを読み込む関数
//--------------
bool ModelResource::load(TextureManager& textureManager, bool isAnimOnly)
{
    // モデルを読み込む
    Assimp::Importer importer;
    const aiScene* scene{};
    if (isAnimOnly)
    {// アニメーションだけ読み込み
        scene = importer.ReadFile(ToUtf8String(m_path), 0);
        if (!scene || !scene->mAnimations)
        {
            return false; // 読み込み失敗
        }
    }
    else
    {// モデル読み込み
        scene = importer.ReadFile(
            ToUtf8String(m_path), // ファイルパス
            LOAD_FLAGS            // 読み込みオプション
        );
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {// エラーチェック
            std::wstringstream st{};
            st << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            std::wstring e = st.str();
            OutputDebugString(e.c_str());
            return false;
        }

        // モデルの基準の大きさを取得
        m_importScale = 1.0f;
        if (scene && scene->mMetaData)
        {
            double unitScale;
            if (scene->mMetaData->Get("UnitScaleFactor", unitScale))
            {
                m_importScale = float(unitScale);
            }
        }

        // マテリアルを処理
        processMaterials(scene, textureManager);

        // ルートノードから再帰的に処理を開始
        m_rootNode = processNode(scene->mRootNode, scene, Matrix());

        // メッシュ生成
        setupMeshs();
    }

    // アニメーションを処理
    processAnimations(scene);

    return true;
}

//--------------
// アニメーションを読み込む関数
//--------------
bool ModelResource::setAnimation(std::span<Animation> anims)
{
    // アニメーション対象のノードが現在のモデルに存在するか確認
    for (const auto& anim : anims)
    {
        if (isThisAnimationLoaded(anim.name)) return false;

        bool isNodeMatch = true;
        // アニメーションが持っている「動かす対象のボーン名」を一つずつ確認する
        for (const auto& channel : anim.channels)
        {
            if (FindNode(m_rootNode, channel.nodeName) == nullptr)
            {
                if (FindNodeofExtract(m_rootNode, channel.nodeName) == nullptr)
                {
                    isNodeMatch = false; // ノードが存在しない
                }
            }
        }

        if (isNodeMatch)
        {
            m_animations.push_back(anim);
        }
    }
    return true;
}

//--------------
// モデルを解放する関数
//--------------
void ModelResource::unload()
{
    // ノードツリーの解放
    if (m_rootNode)
    {
        delete m_rootNode;
        m_rootNode = nullptr;
    }

    // 各種データの解放
    m_vertices.clear();
    m_vertices.shrink_to_fit();
    m_indices.clear();
    m_indices.shrink_to_fit();
    m_materials.clear();
    m_materials.shrink_to_fit();
    m_subsets.clear();
    m_subsets.shrink_to_fit();
    m_textures.clear();
    m_textures.shrink_to_fit();
}

//--------------
// 指定された名前のアニメーションが読み込まれているか調べる関数
//--------------
bool ModelResource::isThisAnimationLoaded(const std::string& name) const
{
    for (const auto& anim : m_animations)
    {
        if (anim.name == name)
        {
            return true;
        }
    }
    return false;
}

//--------------
// マテリアルを処理する関数
//--------------
void ModelResource::processMaterials(const aiScene* scene, TextureManager& textureManager)
{
    m_materials.clear();
    m_textures.clear();

    if (scene->HasMaterials())
    {
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial* mat = scene->mMaterials[i];
            MaterialData matData;

            aiString name;
            mat->Get(AI_MATKEY_NAME, name);
            matData.name = name.C_Str();

            // 色とスペキュラーの強さを取得
            aiColor4D color;
            float shininess = 0.0f;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) ||
                AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, color))
            {
                matData.diffuseColor = Color(color.r, color.g, color.b, color.a);
            }
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR, color))
            {
                matData.specularColor = Color(color.r, color.g, color.b, color.a);
            }
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE, color))
            {
                matData.emissiveColor = Color(color.r, color.g, color.b, color.a);
            }
            if (AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS, shininess))
            {
                matData.shininess = shininess;
            }
            else
            {
                // PBRの場合Shininessの代わりにRoughnessから計算する
                float roughness = 1.0f;
                if (AI_SUCCESS == mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
                {
                    matData.shininess = (1.0f - roughness) * 128.0f;
                }
            }

            aiString path;
            if (AI_SUCCESS == mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) || AI_SUCCESS == mat->GetTexture(aiTextureType_BASE_COLOR, 0, &path))
            {
                std::string pathStr = path.C_Str();
                std::filesystem::path u8path = reinterpret_cast<const char8_t*>(path.C_Str());
                std::filesystem::path fullPath = m_path.parent_path() / u8path;
                TextureHandle texHandle{};
                uint64_t texID = Hash(fullPath.u8string().c_str());

                // 埋め込みテクスチャか確認
                const aiTexture* embedded = scene->GetEmbeddedTexture(path.C_Str());
                if (embedded)
                {// 埋め込みテクスチャ
                    // コピー
                    if (embedded->mHeight == 0)
                    {// 圧縮テクスチャ (jpg, pngなど)
                        // バッファを確保してコピー
                        std::vector<uint8_t> buffer(embedded->mWidth);
                        const uint8_t* src = reinterpret_cast<const uint8_t*>(embedded->pcData);
                        buffer.assign(src, src + embedded->mWidth);

                        textureManager.registerByteData(texID, fullPath, buffer, embedded->achFormatHint);
                        texHandle = textureManager.getTextureHandle(texID);
                    }
                    else
                    {
                        unsigned char* src = reinterpret_cast<unsigned char*>(embedded->pcData);
                        textureManager.registerRawData(texID, fullPath, src, embedded->mWidth, embedded->mHeight);
                        texHandle = textureManager.getTextureHandle(texID);
                    }
                }
                else
                {// 通常のテクスチャファイル
                    // ファイルから読み込み
                    textureManager.registerPath(texID, fullPath);
                    texHandle = textureManager.getTextureHandle(texID);
                }

                // 読み込めたらリストに追加してインデックスを保存
                if (texHandle.isValid())
                {
                    m_textures.push_back(texHandle);
                    matData.textureIndex = (int)m_textures.size() - 1;
                }
            }
            m_materials.push_back(matData);
        }
    }
    // マテリアルがない場合のデフォルトを追加
    if (m_materials.empty()) m_materials.push_back(MaterialData());
}

//--------------
// ノードを再帰的に処理する関数
//--------------
Node* ModelResource::processNode(aiNode* node, const aiScene* scene, const Matrix& parentTransform)
{
    // ノード作成
    Node* newNode = new Node;
    newNode->name = node->mName.C_Str();

    // 行列を初期化
    newNode->defaultTransform = convertMatrix(node->mTransformation);

    // 親の変換行列と掛け合わせてグローバル変換行列を計算
    Matrix globalTransform = Matrix::Multiply(parentTransform, newNode->defaultTransform);

    // ノードが持つメッシュをすべて処理
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // メッシュへのインデックスを格納
        newNode->meshIndices.push_back(node->mMeshes[i]);
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        // メッシュを処理
        processMesh(mesh, scene, globalTransform);
    }

    // 子ノードを再帰的に処理
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        Node* childNode = processNode(node->mChildren[i], scene, globalTransform);
        childNode->parent = newNode;
        newNode->children.push_back(childNode);
    }

    return newNode;
}

//--------------
// メッシュデータを処理する関数
//--------------
void ModelResource::processMesh(aiMesh* mesh, const aiScene* scene, const Matrix& transform)
{
    unsigned int indexStart = static_cast<unsigned int>(m_indices.size());
    unsigned int vertexStart = static_cast<unsigned int>(m_vertices.size());

    bool hasBones = mesh->HasBones();

    // 頂点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        VertexModel vertex;

        // 位置
        vertex.pos = Vector3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (!hasBones)
        {
            vertex.pos.transformCoord(transform);
        }

        // 法線 (存在する場合)
        if (mesh->HasNormals())
        {
            vertex.nor = Vector3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            if (!hasBones)
            {
                vertex.nor.transformNormal(transform);
                vertex.nor.normalize(); // 正規化しておく
            }
        }

        // UV座標
        if (mesh->mTextureCoords[0])
        {
            vertex.uv.x = mesh->mTextureCoords[0][i].x;
            vertex.uv.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.uv = Vector2(0.0f, 0.0f);
        }

        // 頂点カラーを取得するように修正
        if (mesh->HasVertexColors(0))
        {
            vertex.col = Color(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a);
        }
        else
        {
            vertex.col = Color(1.0f, 1.0f, 1.0f, 1.0f); // デフォルトは白
        }

        // ウェイトとボーンIDを初期化
        for (int k = 0; k < 4; ++k)
        {
            vertex.weights[k] = 0.0f;
            vertex.boneIndices[k] = 0;
        }

        m_vertices.push_back(vertex);
    }

    // ボーン
    if (mesh->HasBones())
    {
        for (unsigned int i = 0; i < mesh->mNumBones; ++i)
        {
            aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            int boneIndex = 0;

            // ボーンが初登場なら登録、既知ならインデックス取得
            if (m_boneMapping.find(boneName) == m_boneMapping.end())
            {
                boneIndex = (int)m_boneInfo.size();

                BoneInfo bi;
                bi.name = boneName;
                bi.offsetMatrix = convertMatrix(bone->mOffsetMatrix); // オフセット行列を保存

                m_boneInfo.push_back(bi);

                m_boneMapping[boneName] = boneIndex;
            }
            else
            {
                boneIndex = m_boneMapping[boneName];
            }

            // このボーンが影響を与える頂点たちに、IDとウェイトを書き込む
            for (unsigned int j = 0; j < bone->mNumWeights; ++j)
            {
                // vertexID: メッシュ内のローカルインデックス
                unsigned int localVertexID = bone->mWeights[j].mVertexId;
                float weight = bone->mWeights[j].mWeight;

                VertexModel& v = m_vertices[vertexStart + localVertexID];

                for (int k = 0; k < 4; ++k)
                {
                    if (v.weights[k] == 0.0f) // 空きスロット発見
                    {
                        v.weights[k] = weight;
                        v.boneIndices[k] = (uint8_t)boneIndex;
                        break;
                    }
                }
            }
        }
    }

    // このメッシュ内の頂点ウェイトを正規化 ---
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VertexModel& v = m_vertices[vertexStart + i];
        float sum = v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3];
        if (sum > 1e-6f && fabsf(sum - 1.0f) > 1e-6f)
        {
            v.weights[0] /= sum;
            v.weights[1] /= sum;
            v.weights[2] /= sum;
            v.weights[3] /= sum;
        }
    }

    // インデックス
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // aiProcess_Triangulate フラグを指定したので、face.mNumIndices は常に 3 のはず
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            m_indices.push_back(face.mIndices[j] + vertexStart);
        }
    }

    // サブセットの登録
    Subset subset;
    subset.indexStart = indexStart;              // このメッシュのインデックス開始位置
    subset.indexCount = mesh->mNumFaces * 3;     // インデックス数
    subset.materialIndex = mesh->mMaterialIndex; // マテリアル番号
    m_subsets.push_back(subset);
}

//--------------
// アニメーションデータを処理する関数
//--------------
void ModelResource::processAnimations(const aiScene* scene)
{
    for (unsigned int cntAnim = 0; cntAnim < scene->mNumAnimations; ++cntAnim)
    {
        aiAnimation* srcAnim = scene->mAnimations[cntAnim];
        Animation dstAnim;
        dstAnim.name = srcAnim->mName.C_Str();
        dstAnim.duration = srcAnim->mDuration;
        dstAnim.ticksPerSecond = (srcAnim->mTicksPerSecond != 0) ? srcAnim->mTicksPerSecond : DEFAULT_TICKSPERSECOND; // 0ならデフォルト24fps

        // 各ノードの動き(チャンネル)を読み込む
        for (unsigned int cntChannel = 0; cntChannel < srcAnim->mNumChannels; ++cntChannel)
        {
            aiNodeAnim* srcChannel = srcAnim->mChannels[cntChannel];
            NodeAnimation dstChannel;
            dstChannel.nodeName = srcChannel->mNodeName.C_Str();

            // 位置キー
            for (unsigned int cntKey = 0; cntKey < srcChannel->mNumPositionKeys; ++cntKey)
            {
                auto& key = srcChannel->mPositionKeys[cntKey];
                dstChannel.positionKeys.push_back({ key.mTime, Vector3(key.mValue.x, key.mValue.y, key.mValue.z) });
            }
            // 回転キー
            for (unsigned int cntKey = 0; cntKey < srcChannel->mNumRotationKeys; ++cntKey)
            {
                auto& key = srcChannel->mRotationKeys[cntKey];
                dstChannel.rotationKeys.push_back({ key.mTime, Quaternion(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w) });
            }
            // スケールキー
            for (unsigned int cntKey = 0; cntKey < srcChannel->mNumScalingKeys; ++cntKey)
            {
                auto& key = srcChannel->mScalingKeys[cntKey];
                dstChannel.scalingKeys.push_back({ key.mTime, Vector3(key.mValue.x, key.mValue.y, key.mValue.z) });
            }

            dstAnim.channels.push_back(dstChannel);
        }
        m_animations.push_back(dstAnim);
    }
}

//--------------
// 頂点バッファとインデックスバッファの作成関数
//--------------
void ModelResource::setupMeshs()
{
    // メッシュの作成
    m_mesh = m_renderer.createMesh(VertexShaderType::VertexModel, m_vertices.data(), m_vertices.size(), m_indices.data(), m_indices.size());
}

//----------------------------
// モデルクラス
//----------------------------
static constexpr size_t START_POSE_ID = ~0u - 1u;  // ブレンド中のポーズからブレンドするときの特殊ID
static constexpr float MIN_MATERIAL_POWER = 32.0f; // 最小の鋭さ

Model::Model(ModelManager& modelManager, Renderer& renderer, const ModelHandle& handle) : m_modelManager(modelManager), m_renderer(renderer), m_handle(handle), m_nodeInstanceMaps{}, m_boneTransforms{}, m_currentAnimation{}, m_nextAnimation{}, m_blendDuration{}, m_blendTime{}, m_pBlendStartPose{}, m_isSync{}, m_transform{}, m_pixelShaderType{ PixelShaderType::BlinnPhong } {}

//--------------
// モデルの初期化
//--------------
void Model::init()
{
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        m_transform.scale *= stResource->getImportScale() / WORLD_SIZE;

        // ボーン変換行列配列の初期化
        m_boneTransforms.resize(stResource->getNumBones());

        // ノードインスタンスのセットアップ
        Node* rootNode = stResource->getRootNode();
        setupNodeInstances(rootNode, Matrix());
    }
}

//--------------
// モデルの解放
//--------------
void Model::uninit()
{
    if (m_pBlendStartPose != nullptr)
    {
        delete m_pBlendStartPose;
        m_pBlendStartPose = nullptr;
    }
}

//--------------
// モデルの行列を更新する関数
//--------------
void Model::update(float deltaTime, const Matrix& worldMatrix)
{
    // アニメーションの更新
    updateAnimation(deltaTime);

    // ノードの変換行列を更新
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        updateNodeTransforms(&m_nodeInstanceMaps[stResource->getRootNode()->name], worldMatrix);
    }

    // ボーンの最終変換行列を更新
    updateBoneTransforms();
}

//--------------
// モデルを描画する関数
//--------------
void Model::draw()
{
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        // メッシュを設定 全ノードで共通
        m_renderer.setMesh(stResource->getMesh());

        // ボーン変換行列の設定
        m_renderer.setBoneTransforms(m_boneTransforms);

        // ノードを再帰的に描画
        drawNode(&m_nodeInstanceMaps[stResource->getRootNode()->name]);
    }
}

//--------------
// アニメーションの設定関数
//--------------
void Model::setAnimation(size_t animationIndex, double blendDuration, bool isSync, bool isLoop, bool forceReset)
{
    // アニメーションインデックスのチェック
    Animation* anim{};
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        if (stResource->getAnimation(animationIndex) == nullptr)
        {
            return; // 無効なアニメーションインデックス
        }
    }

    if ((m_blendTime <= 0.01 && m_currentAnimation.animationIndex != animationIndex) || (m_blendTime > 0.01 && m_nextAnimation.animationIndex != animationIndex) || forceReset)
    {// 現在のアニメーションと異なる、または強制リセットの場合
        if ((!isSync || m_blendTime > 0.01) && m_currentAnimation.isPlaying)
        {// スナップショットブレンド指定またはブレンド中
            // 現在のアニメーションをブレンド開始ポーズとして保存
            setupBlendStartPose();                             // ブレンド開始ポーズをセットアップ (m_pBlendStartPose)
            m_currentAnimation.animationIndex = START_POSE_ID; // 特殊識別番号 (m_pBlendStartPose)
            m_currentAnimation.currentTime = 0.0;              // 時間は0で初期化
            m_currentAnimation.isPlaying = false;              // 再生停止
            m_currentAnimation.isLoop = false;                 // ループしない
            m_isSync = false;                                  // 同期ブレンドでない
        }
        else if (m_nextAnimation.animationIndex != INVALID_ANIM_ID)
        {
            m_currentAnimation = m_nextAnimation;
            m_isSync = true; // 同期ブレンド
        }
        else
        {
            m_isSync = true; // 同期ブレンド
        }

        // 次のアニメーションをセット
        m_nextAnimation.animationIndex = animationIndex;
        m_nextAnimation.currentTime = 0.0;
        m_nextAnimation.isPlaying = true;
        m_nextAnimation.isLoop = isLoop;

        // ブレンド時間をセット
        m_blendDuration = blendDuration;
        m_blendTime = 0.0;

        if (m_isSync)
        {
            m_nextAnimation.isLoop = true;
            m_currentAnimation.isLoop = true;
        }
    }
}

//-----------------------------------
// モデルをスケーリングする (最終調整用)
//-----------------------------------
void Model::setScale(float scale)
{
    m_transform.scale *= scale;
}

//--------------
// ノードインスタンスのセットアップ（再帰）
//--------------
void Model::setupNodeInstances(Node* node, const Matrix& parentTransform)
{
    // nodeからインスタンスを生成する
    NodeInstance instance{};
    instance.node = node;
    instance.localTransform = node->defaultTransform.toTransform();
    instance.globalTransform = Matrix::Multiply(node->defaultTransform, parentTransform);
    m_nodeInstanceMaps.try_emplace(node->name, instance);
    for (auto child : node->children)
    {
        setupNodeInstances(child, instance.globalTransform);
    }
}

//--------------
// アニメーションを更新する関数
//--------------
void Model::updateAnimation(double deltaTime)
{
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        // ブレンド時間の更新
        m_blendTime += deltaTime;
        if (m_blendTime > m_blendDuration)
        {
            m_blendTime = 0.0;
            m_blendDuration = 0.0;
            if (m_nextAnimation.animationIndex != INVALID_ANIM_ID)
            {
                m_currentAnimation = m_nextAnimation;
            }
            m_nextAnimation.isPlaying = false;
            m_nextAnimation.animationIndex = INVALID_ANIM_ID;
            m_nextAnimation.currentTime = 0.0;
        }

        // 現在のアニメーションの進行
        Animation* currentAnim{};
        if (m_currentAnimation.animationIndex == START_POSE_ID)
        {// ブレンド中の静止ポーズを使用する
            currentAnim = m_pBlendStartPose;
        }
        else
        {// 現在のアニメーション
            currentAnim = stResource->getAnimation(m_currentAnimation.animationIndex);
        }
        if (currentAnim != nullptr && m_currentAnimation.isPlaying)
        {
            m_currentAnimation.currentTime += deltaTime * currentAnim->ticksPerSecond;
            if (m_currentAnimation.currentTime >= currentAnim->duration)
            {
                if (m_currentAnimation.isLoop)
                {
                    m_currentAnimation.currentTime = fmod(m_currentAnimation.currentTime, currentAnim->duration);
                }
                else
                {
                    m_currentAnimation.currentTime = currentAnim->duration;
                    m_currentAnimation.isPlaying = false;
                }
            }
        }

        // 現在のアニメーションの進行
        Animation* nextAnim = stResource->getAnimation(m_nextAnimation.animationIndex);
        if (nextAnim != nullptr && m_nextAnimation.isPlaying)
        {
            if (m_isSync)
            {
                double phase = (currentAnim != nullptr && currentAnim->duration > 0.0001) ? m_currentAnimation.currentTime / currentAnim->duration : 0.0f;
                m_nextAnimation.currentTime = phase * nextAnim->duration;
            }
            else
            {
                m_nextAnimation.currentTime += deltaTime * nextAnim->ticksPerSecond;
            }
            if (m_nextAnimation.currentTime >= nextAnim->duration)
            {
                if (m_nextAnimation.isLoop)
                {
                    m_nextAnimation.currentTime = fmod(m_nextAnimation.currentTime, nextAnim->duration);
                }
                else
                {
                    m_nextAnimation.currentTime = nextAnim->duration;
                    m_nextAnimation.isPlaying = false;
                }
            }
        }

        // 各チャンネル（ノードの動き）を適用
        updateNodeAnimTransforms(&m_nodeInstanceMaps[stResource->getRootNode()->name], currentAnim, nextAnim, m_currentAnimation.currentTime, m_nextAnimation.currentTime, m_currentAnimation.isLoop, m_nextAnimation.isLoop);
    }
}

//--------------
// ノードのグローバルトランスフォームを計算
//--------------
void Model::updateNodeTransforms(NodeInstance* node, const Matrix& parentTransform)
{
    // 親から子へ行列をかけ合わせていく
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        if (node->node == stResource->getRootNode())
        {// ルートノードにはモデル全体の変換を合成する
            Matrix localTransform = node->localTransform.toMatrix();
            localTransform.multiply(m_transform.toMatrix());
            node->globalTransform = Matrix::Multiply(localTransform, parentTransform);
        }
        else
        {// 通常のノード
            node->globalTransform = Matrix::Multiply(node->localTransform.toMatrix(), parentTransform);
        }
    }

    for (auto child : node->node->children)
    {// 子へ送る
        updateNodeTransforms(&m_nodeInstanceMaps[child->name], node->globalTransform);
    }
}

//--------------
// ボーンの最終変換行列を更新する関数
//--------------
void Model::updateBoneTransforms()
{
    Animation* anim{};
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        // Modelクラスの m_boneTransforms 配列を更新
        for (size_t cnt = 0; cnt < stResource->getNumBones(); ++cnt)
        {
            const auto& boneInfo = stResource->getBoneInfo(cnt);
            auto node = m_nodeInstanceMaps.find(boneInfo->name);

            if (node != m_nodeInstanceMaps.end())
            {// ノードが見つかった場合
                m_boneTransforms[cnt] = Matrix::Multiply(boneInfo->offsetMatrix, node->second.globalTransform);
            }
            else
            {
                m_boneTransforms[cnt].identity(); // 見つからない場合は単位行列
            }
        }
    }
}

//--------------
// ノードを描画する関数
//--------------
void Model::drawNode(NodeInstance* node)
{
    auto resource = m_modelManager.getModelData(m_handle);
    if (auto stResource = resource.lock())
    {
        // サブセットごとに描画
        for (const auto& meshIndex : node->node->meshIndices)
        {
            const auto& subset = stResource->getSubset(meshIndex);
            const auto& matData = stResource->getMaterial(subset->materialIndex);

            if (subset->materialIndex < stResource->getNumMaterials())
            {
                // マテリアルの設定
                Material material{};
                material.Diffuse = matData->diffuseColor;
                material.Specular = matData->specularColor;
                material.Emissive = matData->emissiveColor;
                if (matData->shininess < 1.0f)
                {// スペキュラー無効
                    material.Power = MIN_MATERIAL_POWER; // 最小値補正
                    material.Specular = Color::Black();  // スペキュラー無効化
                }
                else
                {
                    material.Power = matData->shininess;
                }
                material.AlphaCutoff = 0.01f; // 固定値
                material.pixelShaderType = m_pixelShaderType;
                m_renderer.setMaterial(material);

                // テクスチャの設定
                if (matData->textureIndex != -1)
                {
                    m_renderer.setTexture(stResource->getTextureHandle(matData->textureIndex));
                }
                else
                {
                    m_renderer.setTexture(TextureHandle());
                }
            }

            // ポリゴンの描画
            m_renderer.drawIndexedPrimitive
            (
                VertexShaderType::VertexModel, // 頂点シェーダーの種類
                subset->indexCount,            // インデックス数
                subset->indexStart,            // インデックスバッファの開始位置
                0                              // 頂点バッファの開始位置
            );
        }

        for (auto child : node->node->children)
        {// 子へ送る
            drawNode(&m_nodeInstanceMaps[child->name]);
        }
    }
}

//--------------
// ノードにアニメーションを適応する
//--------------
void Model::updateNodeAnimTransforms(NodeInstance* node, Animation* currentAnim, Animation* nextAnim, double currentTime, double nextTime, bool isCurrentLoop, bool isNextLoop)
{
    if (currentAnim != nullptr)
    {
        auto resource = m_modelManager.getModelData(m_handle);
        if (auto stResource = resource.lock())
        {
            Transform  resultTransform{};
            // 今のアニメーション
            Transform currentTransform = getAnimatedTransform(node, currentAnim, node->node->defaultTransform.toTransform(), currentTime,isCurrentLoop);

            resultTransform = currentTransform;

            if (nextAnim != nullptr)
            {
                // 次のアニメーション
                Transform nextTransform = getAnimatedTransform(node, nextAnim, node->node->defaultTransform.toTransform(),nextTime,isNextLoop);

                // ブレンド
                float time = (m_blendDuration > 0.0001f) ? float(m_blendTime / m_blendDuration) : 1.0f;
                time = std::clamp(time, 0.0f, 1.0f);
                resultTransform = Transform::Slerp(currentTransform, nextTransform, time);
            }

            // 適応する
            node->localTransform = resultTransform;
        }
    }

    for (auto child : node->node->children)
    {// 子へ送る
        updateNodeAnimTransforms(&m_nodeInstanceMaps[child->name], currentAnim, nextAnim, currentTime, nextTime, isCurrentLoop, isNextLoop);
    }
}

//--------------
// ノードのアニメーションの変換を取得
//--------------
Transform Model::getAnimatedTransform(NodeInstance* node, const Animation* anim, const Transform& defaultTransform, double currentTime, bool isLoop)
{
    if (anim != nullptr)
    {
        auto resource = m_modelManager.getModelData(m_handle);
        if (auto stResource = resource.lock())
        {
            for (const auto& channel : anim->channels)
            {
                if (node->node->name == channel.nodeName)
                {
                    Transform transform;

                    // 時間に応じた値を計算
                    transform.position = CalcInterpolatedVector(currentTime, channel.positionKeys, anim->duration, isLoop, defaultTransform.position);
                    transform.rotation = CalcInterpolatedRotation(currentTime, channel.rotationKeys, anim->duration, isLoop, defaultTransform.rotation);
                    transform.scale = CalcInterpolatedVector(currentTime, channel.scalingKeys, anim->duration, isLoop, defaultTransform.scale);
                    return transform;
                }
            }
        }
    }
    return defaultTransform;
}

//--------------
// ブレンド開始ポーズアニメーションのセットアップ (現在のポーズの静止アニメーションを作成)
//--------------
void Model::setupBlendStartPose()
{
    if (m_pBlendStartPose != nullptr)
    {
        m_pBlendStartPose->channels.clear();
        delete m_pBlendStartPose;
        m_pBlendStartPose = nullptr;
    }

    m_pBlendStartPose = new Animation();
    m_pBlendStartPose->name = "BlendStartPose";
    m_pBlendStartPose->duration = 0.0;
    m_pBlendStartPose->ticksPerSecond = 0.0;

    for (const auto& pair : m_nodeInstanceMaps)
    {
        const std::string& name = pair.first;
        const NodeInstance& nodeInst = pair.second;

        NodeAnimation nodeAnim{};
        nodeAnim.nodeName = name;

        // 現在のローカル変換行列から、位置・回転・スケールを抽出してキーフレームにする
        Transform currentLocal = nodeInst.localTransform;

        nodeAnim.positionKeys.push_back({ 0.0, currentLocal.position });
        nodeAnim.rotationKeys.push_back({ 0.0, currentLocal.rotation });
        nodeAnim.scalingKeys.push_back({ 0.0, currentLocal.scale });

        m_pBlendStartPose->channels.push_back(nodeAnim);
    }
}

//----------------------------
// モデルマネージャークラス
//----------------------------

//--------------
// モデルを読み込む関数
//--------------
bool ModelManager::load(Renderer& renderer, TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback, uint64_t id)
{
    if (id != Hash(""))
    {// テクスチャが指定されている
        if (m_idToHandle.contains(id))
        {
            ModelHandle handle = m_idToHandle[id];
            if (handle.id < m_slots.size() && m_slots[handle.id].data == nullptr)
            {// 登録されておりまだデータが読み込まれていないテクスチャ
                // 読み込む
                std::shared_ptr<ModelResource> data = std::make_shared<ModelResource>(m_slots[handle.id].path, renderer);
                data->load(textureManager, m_slots[handle.id].isAnimationOnly);
                m_slots[handle.id].data = data;
            }
        }
        return true;
    }
    else
    {// 指定なし (登録テクスチャを全て読み込む)

        // 読み込み対象の数をカウント
        int totalCount = 0;
        for (const auto& slot : m_slots)
        {
            if (slot.data == nullptr) totalCount++;
        }
        if (totalCount == 0) return true; // 読み込むものがなければ終了

        std::vector<std::future<void>> futures{};
        std::atomic<int> finishedCount{ 0 }; // 完了した数
        std::atomic<int> activeThreads{ 0 };

        // メモリ確保
        futures.reserve(totalCount);

        for (size_t cnt = 0; cnt < m_slots.size(); cnt++)
        {
            if (m_slots[cnt].data == nullptr)
            {// 読み込まれていない
                // 空きが出るまで待つ
                while (activeThreads.load() >= (int)maxThread)
                {
                    // コールバックがあれば呼び出す
                    if (progressCallback != nullptr)
                    {
                        if (!progressCallback("Model", finishedCount.load(), totalCount))
                        {// キャンセルされた
                            for (auto& future : futures)
                            {
                                if (future.valid()) future.get();
                            }
                            return false;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                // 実行数
                ++activeThreads;

                // 読み込む
                std::filesystem::path path = m_slots[cnt].path;
                bool isAnimationOnly = m_slots[cnt].isAnimationOnly;
                futures.push_back(std::async(std::launch::async, [this, cnt, path, isAnimationOnly, &activeThreads, &finishedCount, &renderer, &textureManager]()
                    {
                        // 読み込む
                        std::shared_ptr<ModelResource> data = std::make_shared<ModelResource>(path, renderer);
                        data->load(textureManager, isAnimationOnly);

                        {// m_slotsは同時に触らない
                            std::lock_guard<std::mutex> lock(m_slotsMutex);
                            m_slots[cnt].data = data;
                        }

                        --activeThreads;
                        ++finishedCount;
                    }));
            }
        }

        // 残りのタスクが終わるのを待つ
        while (finishedCount.load() < totalCount)
        {
            if (progressCallback != nullptr)
            {
                if (!progressCallback("Model", finishedCount.load(), totalCount))
                {
                    // キャンセルされた場合
                    for (auto& future : futures)
                    {
                        if (future.valid()) future.get();
                    }
                    return false;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
        }

        // この関数はすべて読み込み終わってから終わります
        for (auto& future : futures)
        {
            if (future.valid()) future.get();
        }
        if (progressCallback != nullptr)
        {
            return progressCallback("Model", finishedCount.load(), totalCount);
        }
        return true;
    }
}

//----------------------------------
// Modelを登録する
//----------------------------------
bool ModelManager::registerPath(uint64_t id, const std::filesystem::path& path, bool isAnimationOnly)
{
    // キャッシュチェック
    if (m_idToHandle.contains(id))
    {
        // 既に読み込まれている
        return false;
    }

    // スロットに登録
    ModelSlot slot{};
    slot.path = path;
    slot.isAnimationOnly = isAnimationOnly;
    m_slots.push_back(slot);

    // ハンドルを登録
    m_idToHandle.try_emplace(id);
    m_idToHandle[id] = ModelHandle{ (unsigned int)(m_slots.size() - 1) };

    // 新しいリソースでモデルインスタンスを作成して返す
    return true;
}

//----------------------------------
// アニメーションをModelに登録する
//----------------------------------
bool ModelManager::setAnimation(uint64_t destModel, uint64_t srcAnim)
{
    // キャッシュチェック
    if (m_idToHandle.contains(destModel) && m_idToHandle.contains(srcAnim))
    {
        ModelSlot& destSlot = m_slots[m_idToHandle[destModel].id];
        ModelSlot& srcSlot = m_slots[m_idToHandle[srcAnim].id];

        for (const auto& handle : destSlot.motionHandles)
        {
            if (m_idToHandle[srcAnim].id == handle.id)
            {
                return false; // すでにアニメーションが読み込まれている
            }
        }

        // まだ読み込まれていない場合、アニメーションを読み込む
        if (destSlot.data->setAnimation(srcSlot.data->getAnimations()))
        {
            destSlot.motionHandles.push_back(m_idToHandle[srcAnim]);
            return true; // アニメーションが読み込めた
        }
        else
        {
            return false; // アニメーションが読み込めなかった
        }
    }
    return false; // モデルが存在しない
}

//--------------
// CPUリソースを解放する関数
//--------------
void ModelManager::releaseCpuResource(const ModelHandle& handle)
{
    if (m_slots.size() > handle.id)
    {
        auto data = m_slots[handle.id];
        data.data->unload();
        data.data.reset();
    }
}

//--------------
// CPUリソースを解放する関数
//--------------
void ModelManager::releaseCpuResources()
{
    for (auto& slot : m_slots)
    {
        if (slot.data != nullptr)
        {
            slot.data->unload();
            slot.data.reset();
        }
    }
}

//--------------
// モデルハンドルを取得する関数
//--------------
ModelHandle ModelManager::getModelHandle(uint64_t id)
{
    if (m_idToHandle.contains(id))
    {
        return m_idToHandle[id];
    }
    return ModelHandle();
}

//--------------
// モデルデータを取得する関数
//--------------
std::weak_ptr<ModelResource> ModelManager::getModelData(const ModelHandle& handle) const
{
    if (m_slots.size() > handle.id)
    {
        auto data = m_slots[handle.id];
        return data.data;
    }
    else
    {
        return std::weak_ptr<ModelResource>{};
    }
}
