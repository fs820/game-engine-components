// Outline.hlsli
#include "Common.hlsli"

struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float4 OutlineColor : COLOR0;
    float4 Color : COLOR1;
    float2 UV : TEXCOORD0;
};

// b4: アウトライン
cbuffer OutlineBuffer : register(b4)
{
    float4 OutlineColor; // 色
    float OutlineWidth;  // 太さ
    float3 paddings;
}
