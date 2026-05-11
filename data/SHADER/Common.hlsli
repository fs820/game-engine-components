// Common.hlsli
#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

// --------------------------------------------------------
// 定数バッファ (スロット番号を固定管理)
// --------------------------------------------------------

// b1: ビュー・プロジェクション行列 (VSで使用)
cbuffer VPMatrixBuffer : register(b0)
{
    row_major matrix View;
    row_major matrix Proj;
}

// b2: ワールド行列 (VSで使用)
cbuffer WMatrixBuffer : register(b1)
{
    row_major matrix World;
}

// b3: マテリアル色 (PSで使用)
cbuffer MaterialBuffer : register(b2)
{
    float4 MaterialDiffuse;    // 拡散反射色
    float4 MaterialSpecular;   // 鏡面反射色
    float4 MaterialEmissive;   // 発光色
    float MaterialPower;       // 鏡面反射の鋭さ
    float MaterialAlphaCutoff; // アルファテストのしきい値
    int PixelShaderType;       // ピクセルシェーダの種類
    float padding;             // 16バイト境界合わせ用
}

// --------------------------------------------------------
// シェーダー間データ (VS -> PS)
// --------------------------------------------------------
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 WorldPos : POSITION; // ライティング計算用
};

// --------------------------------------------------------
// テクスチャ・サンプラー (PSで使用)
// --------------------------------------------------------
Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

#endif
