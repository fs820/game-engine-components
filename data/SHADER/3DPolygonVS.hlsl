// 3DPolygonVS.hlsl
#include "Common.hlsli"

// 入力頂点データ: C++の Vertex3D 構造体と対応させる
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// 頂点シェーダー (Vertex Shader)
// 座標変換を担当
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // ワールド変換
    float4 wPos = mul(float4(input.Pos, 1.0f), World);

    // ワールド座標を渡す
    output.WorldPos = wPos.xyz;

    // ビュー・プロジェクション変換
    output.Pos = mul(mul(wPos, View), Proj);
    
    // 法線の回転 (平行移動は無視するため w=0 にする)
    float4 normal = float4(input.Normal, 0.0f);
    output.Normal = normalize(mul(normal, World).xyz);

    // 色とUVはそのまま渡す
    output.Color = input.Color;
    output.UV = input.UV;

    return output;
}
