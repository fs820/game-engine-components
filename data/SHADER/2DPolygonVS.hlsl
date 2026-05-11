// 2DPolygonVS.hlsl
#include "Common.hlsli"

// 入力頂点データ: C++の Vertex2D 構造体と対応させる
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// 頂点シェーダー (Vertex Shader)
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 座標変換 (World * View * Proj)
    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Proj);
    
    output.Color = input.Color;
    output.UV = input.UV;
    
    // 法線不要
    output.Normal = float3(0, 0, -1);
    output.WorldPos = float3(0, 0, 0);
    return output;
}
