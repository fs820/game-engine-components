// PP_GaussianBlur.hlsl
#include "PostProcess.hlsli"

static const float Weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 PS(VS_OUTPUT input) : SV_Target
{
    // 方向 * 1ピクセル幅 = UVのずらす量 (ブルームは1/4サイズ *4.0倍する)
    float2 offset = BlurDir * ScreenSize.zw * 4.0f;

    float3 result = gSceneTexture.Sample(gSampler, input.UV).rgb * Weights[0];

    for (int i = 1; i < 5; ++i)
    {
        result += gSceneTexture.Sample(gSampler, input.UV + offset * i).rgb * Weights[i];
        result += gSceneTexture.Sample(gSampler, input.UV - offset * i).rgb * Weights[i];
    }

    return float4(result, 1.0f);
}
