// OutlinePS.hlsl
#include "Outline.hlsli"

// カラーをそのまま返すだけ
float4 PS(VS_OUT input) : SV_Target
{
    // テクスチャの色とマテリアルの色を合成
    float4 baseColor = myTexture.Sample(mySampler, input.UV) * input.Color * MaterialDiffuse;
    
    // アルファカット
    clip(baseColor.a - MaterialAlphaCutoff);
    
    return input.OutlineColor;
}
