#include "PostProcess.hlsli"

float4 PS(VS_OUTPUT input) : SV_Target
{
    return gSceneTexture.Sample(gSampler, input.UV);
}
