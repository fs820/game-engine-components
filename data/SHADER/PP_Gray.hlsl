#include "PostProcess.hlsli"

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 col = gSceneTexture.Sample(gSampler, input.UV);

    float gray = dot(col, float3(0.2126, 0.7152, 0.0722));

    return float4(gray, gray, gray, 1.0);
}
