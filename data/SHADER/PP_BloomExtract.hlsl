#include "PostProcess.hlsli"

float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 color = gSceneTexture.Sample(gSampler, input.UV);
    
    // 輝度(明るさ)を計算
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    
    // 閾値を超えた分だけ残す
    float contribution = max(0.0f, brightness - bloomThreshold);
    
    // 元の色味を残しつつ抽出
    // 明るさが0なら真っ黒になる
    return color * contribution;
}
