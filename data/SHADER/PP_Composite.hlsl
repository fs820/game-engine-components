// PP_Composite.hlsl
#include "PostProcess.hlsli"

// ブルーム画像 (t1)
Texture2D gBloomTexture : register(t1);

// トーンマッピングの種類 (画面の色合いが変わります)
static const int Linear = 0;   // トーンマッピングなし (そのままの色) (白飛び注意)
static const int Reinhard = 1; // 落ち着いた色 (汎用的)
static const int ACEST = 2;    // リアルより
static const int Anime = 3;    // アニメ調

// トーンマッピングなし (色がなんか違うが起こらない) (光の白飛びが起こりやすい)
float3 LinearToneMapping(float3 x)
{
    return saturate(x); // 切る
}

// ラインハルト (シンプルで落ち着いている) 汎用性が高い
float3 ReinhardToneMapping(float3 x)
{
    return x / (x + 1.0f);
}

// ACESフィルムトーンマッピング (映画のような色調補正) (リアルより) (トゥーンシェーダなどのアニメ調には不向き)
float3 ACESToneMapping(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// 簡易的な彩度強調付きACES (アニメ調のトーンマッピング)
float3 ACESToneMapping_AnimeStyle(float3 x)
{
    // 輝度だけを取り出してACESをかける
    float luma = dot(x, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = (luma * (2.51f * luma + 0.03f)) / (luma * (2.43f * luma + 0.59f) + 0.14f);
    
    // 元の色を輝度で割って色味だけにする
    float3 colorScale = x / (luma + 0.0001f);
    
    // トーンマップ後の輝度に、元の色味を掛け戻す
    return toneMappedLuma * colorScale;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    // メインシーン (HDR)
    float3 sceneColor = gSceneTexture.Sample(gSampler, input.UV).rgb;
    
    // ブルーム (HDR)
    float3 bloomColor = gBloomTexture.Sample(gSampler, input.UV).rgb;
    
    // ブルーム強度調整
    sceneColor += bloomColor * bloomIntensity; // 加算合成

    // トーンマッピング (HDR -> LDR)
    switch (toneMappingType)
    {
        default:
        case Linear:
            sceneColor = LinearToneMapping(sceneColor);
            break;
        case Reinhard:
            sceneColor = ReinhardToneMapping(sceneColor);
            break;
        case ACEST:
            sceneColor = ACESToneMapping(sceneColor);
            break;
        case Anime:
            sceneColor = ACESToneMapping_AnimeStyle(sceneColor);
            break;
    }

    // フォーマットがsRGBでない場合 pow(sceneColor, 1.0/2.2);を追加してください
    return float4(sceneColor, 1.0f);
}
