// DecalVS.hlsl
#include "Common.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float4 ScreenPos : TEXCOORD0; // スクリーン座標計算用
};

VS_OUT VS(VS_INPUT input)
{
    VS_OUT output;

    // ワールド・ビュー・プロジェクション変換
    float4 wPos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(mul(wPos, View), Proj);

    // 画面上のどこか
    output.ScreenPos = output.Pos;

    return output;
}
