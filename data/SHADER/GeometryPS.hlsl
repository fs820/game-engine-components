// GeometryPS.hlsl
#include "Common.hlsli"

// 出力用構造体を定義
struct PS_OUTPUT
{
    float4 RT0 : SV_Target0; // Color + SpecIntensity
    float4 RT1 : SV_Target1; // Normal + LightFlag
    float4 RT2 : SV_Target2; // Position
    float4 RT3 : SV_Target3; // Emissive + SpecPower
};

// エントリーポイント: PS
PS_OUTPUT PS(PS_INPUT input)
{
    PS_OUTPUT output;
    
    // テクスチャの色とマテリアルの色を合成
    float4 baseColor = myTexture.Sample(mySampler, input.UV) * input.Color * MaterialDiffuse;

    // アルファカットオフ
    clip(baseColor.a - MaterialAlphaCutoff);
    
    // RT0: 色とスペキュラー強度
    output.RT0.rgb = baseColor.rgb;
    // Alphaにスペキュラーの強さを入れる
    output.RT0.a = (MaterialSpecular.r + MaterialSpecular.g + MaterialSpecular.b) / 3.0f;

    // RT1: 法線とライトフラグ
    output.RT1.rgb = normalize(input.Normal);
    // Alphaにピクセルシェーダの種類を入れる
    output.RT1.a = float(PixelShaderType);

    // RT2: 座標
    output.RT2 = float4(input.WorldPos, 1.0f);

    // RT3: 発光とスペキュラーの鋭さ
    output.RT3.rgb = MaterialEmissive.rgb;
    // Alphaに鋭さ(Power)を入れる。
    output.RT3.a = saturate(MaterialPower / 100.0f);

    return output;
}
