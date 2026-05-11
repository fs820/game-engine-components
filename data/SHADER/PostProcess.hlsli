// PostProcess.hlsli

// シーンテクスチャ
Texture2D gSceneTexture : register(t0);
SamplerState gSampler : register(s0);

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// 定数バッファ (b0)
cbuffer PostProcessBuffer : register(b0)
{
    float4 ScreenSize;    // x:W, y:H, z:1/W, w:1/H
    float2 BlurDir;       // x,y: ぼかす方向 (1,0) or (0,1)
    float bloomThreshold; // ブルームで抽出する明るさの閾値
    float bloomIntensity; // ブルーム強度
    int toneMappingType;  // トーンマッピングの種類
    float padding[3];
}
