// GeometryPS.hlsl
#include "Common.hlsli"

// エントリーポイント: PS
void PS(PS_INPUT input)
{
    // テクスチャの色とマテリアルの色を合成
    float4 baseColor = myTexture.Sample(mySampler, input.UV) * input.Color * MaterialDiffuse;

    // アルファカットオフ
    clip(baseColor.a - MaterialAlphaCutoff);
}
