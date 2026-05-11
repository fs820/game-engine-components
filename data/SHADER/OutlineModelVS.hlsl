// OutlineModelVS.hlsl
#include "Outline.hlsli"

#define MAX_BONES 256

cbuffer BoneBuffer : register(b3)
{
    row_major matrix BoneTransforms[MAX_BONES];
}

struct VS_SKIN_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float4 Weights : WEIGHTS;
    uint4 Bones : BONES;
};

VS_OUT VS(VS_SKIN_INPUT input)
{
    VS_OUT output = (VS_OUT) 0;

    float4 pos = float4(input.Pos, 1.0f);
    float3 normal = input.Normal;
    float weightSum = dot(input.Weights, float4(1, 1, 1, 1));

    float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);

    // スキニング計算
    if (weightSum > 0.0001f)
    {
        float4 w = input.Weights / weightSum;
        uint4 bones = input.Bones;

        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            if (w[i] > 0.0001f)
            {
                matrix bm = BoneTransforms[bones[i]];
                
                // World
                skinnedPos += mul(pos, bm) * w[i];
                
                // 法線World
                skinnedNormal += mul(float4(normal, 0.0f), bm).xyz * w[i];
            }
        }
    }
    else
    {
        // ボーンがない場合 (スタティック)
        
        // World行列
        skinnedPos = mul(pos, World);
        
        // 法線World
        skinnedNormal = mul(float4(normal, 0.0f), World).xyz;
    }

    // アウトライン押し出し

    // プロジェクション空間へ変換
    output.Pos = mul(mul(skinnedPos, View), Proj);

    // 画面上の法線の向き
    float3 vNormal = mul(skinnedNormal, (float3x3) View);
    float2 offset = normalize(vNormal.xy); // Z成分(奥行き)は無視する

    // アスペクト比補正
    offset.x *= Proj[1][1] / Proj[0][0];

    // 押し出し
    output.Pos.xy += offset * OutlineWidth * output.Pos.w;
    
    output.Color = input.Color;
    output.UV = input.UV;
    output.OutlineColor = OutlineColor;

    return output;
}
