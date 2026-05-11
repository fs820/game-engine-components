#include "Common.hlsli"
#include "Lighting.hlsli"

// エントリーポイント
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

    // Fog
    // フォグ色から空色へ変化させる
    
    // 高さが低いほどフォグ色
    float height = input.WorldPos.y - horizonHeight;
    float fogFactor = saturate(1.0f - (height / skyFogHeight));
    
    fogFactor = pow(fogFactor, skyFogPower);

    // 空の色とフォグ色をブレンド
    finalColor = lerp(finalColor, fogColor, fogFactor);

    // 出力
    return finalColor;
}
