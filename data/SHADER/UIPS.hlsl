#include "Common.hlsli"

float4 PS(PS_INPUT input) : SV_Target
{    
    // テクスチャの色
    float4 texColor = myTexture.Sample(mySampler, input.UV);
    
    // 頂点カラー・マテリアル色
    float4 finalColor = texColor * input.Color * MaterialDiffuse;

    // アルファテスト
    clip(finalColor.a - MaterialAlphaCutoff);

    // エミッシブ
    finalColor.rgb += MaterialEmissive.rgb;

    // 出力
    return finalColor;
}
