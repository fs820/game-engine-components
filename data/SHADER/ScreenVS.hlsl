#include "Lighting.hlsli"

// --------------------------------------------------------
// 頂点シェーダー (Vertex Shader)
// 頂点バッファを使わず、VertexIDから巨大な三角形を作って画面を覆う
// --------------------------------------------------------
VS_OUTPUT VS(uint id : SV_VertexID)
{
    VS_OUTPUT output;
    
    // UV: (0,0), (2,0), (0,2) の三角形を作る（画面全体 [-1,1] を覆う）
    output.UV = float2((id << 1) & 2, id & 2);
    
    // 座標: [-1,1] の範囲に変換 Yは反転
    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return output;
}
