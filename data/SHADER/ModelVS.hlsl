// ModelVS.hlsl
#include "Common.hlsli"

#define MAX_BONES 256 // ボーンの最大数

// ボーン行列配列
cbuffer BoneBuffer : register(b3)
{
    row_major matrix BoneTransforms[MAX_BONES];
}

// 入力頂点データ (VertexModelに対応)
struct VS_SKIN_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float4 Weights : WEIGHTS;
    uint4 Bones : BONES;
};

// --------------------------------------------------------
// 頂点シェーダー (スキニング計算の本体)
// --------------------------------------------------------
PS_INPUT VS(VS_SKIN_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // 入力の位置・法線
    float4 pos = float4(input.Pos, 1.0f);
    float3 normal = input.Normal;

    // ウェイト合計（ゼロ割防止）
    float weightSum = dot(input.Weights, float4(1, 1, 1, 1));

    if (weightSum > 0.0001f)
    {
        // ウェイトを正規化（念のため）
        float4 w = input.Weights / weightSum;

        float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);

        uint4 bones = input.Bones;

        // 各ボーンごとに位置と法線を変換してウェイト合成する（行列の合算を避ける）
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            float wi = w[i];
            if (wi > 0.0001f)
            {
                uint bi = bones[i];
                matrix bm = BoneTransforms[bi];

                // pos をボーン行列で変換してウェイトを掛ける
                float4 tp = mul(pos, bm); // row-vector * matrix の順
                skinnedPos += tp * wi;

                // 法線は回転成分のみ扱うが、ここでは簡易に行列を適用（w=0.0）
                float4 tn = mul(float4(normal, 0.0f), bm);
                skinnedNormal += tn.xyz * wi;
            }
        }

        // BoneTransforms は World を含んでいる

        // ワールド座標を出力
        output.WorldPos = skinnedPos.xyz;
        output.Pos = mul(mul(skinnedPos, View), Proj);
        output.Normal = normalize(skinnedNormal);
    }
    else
    {
        // ボーンがない場合（スタティック）
        pos = float4(input.Pos, 1.0f);
        
        // ワールド変換
        float4 wPos = mul(pos, World);

        // ワールド座標を出力
        output.WorldPos = wPos.xyz;

        // ビュー・プロジェクション変換
        output.Pos = mul(mul(wPos, View), Proj);
                
        float4 tn = mul(float4(input.Normal, 0.0f), World);
        output.Normal = normalize(tn.xyz);
    }

    // その他
    output.Color = input.Color;
    output.UV = input.UV;

    return output;
}
