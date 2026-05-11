// Outline3DVS.hlsl
#include "Outline.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

VS_OUT VS(VS_INPUT input)
{
    VS_OUT output = (VS_OUT) 0;

    // ワールド・ビュー・プロジェクション変換
    output.Pos = mul(mul(mul(float4(input.Pos, 1.0f), World), View), Proj);

    // 画面上の法線の向き
    float3 vNormal = mul(mul(float4(input.Normal, 0.0f), World).xyz, (float3x3) View);
    float2 offset = normalize(vNormal.xy); // Z成分(奥行き)は無視する

    // アスペクト比補正
    offset.x *= Proj[1][1] / Proj[0][0];

    // 押し出し
    output.Pos.xy += offset * OutlineWidth * output.Pos.w;

    output.Color = input.Color;
    output.UV = input.UV;
    output.OutlineColor = OutlineColor;

    return output;
}
