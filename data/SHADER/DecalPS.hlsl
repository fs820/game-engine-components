// DecalPS.hlsl
#include "Common.hlsli"

// デカール用定数バッファ (b4)
cbuffer DecalBuffer : register(b4)
{
    row_major matrix InverseWorld; // デカールの逆行列
    float4 DecalColor;             // 色
}

// G-Bufferの座標テクスチャ (t1)
Texture2D gPositionTex : register(t1);

struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float4 ScreenPos : TEXCOORD0;
};

// RT0 (Albedo/Diffuse) だけに書き込むので float4 を返す
float4 PS(VS_OUT input) : SV_Target0
{
    // スクリーンUVを計算 (0.0 ~ 1.0)
    // ClipSpace(-1~1) を UV(0~1) に変換
    float2 screenUV = input.ScreenPos.xy / input.ScreenPos.w;
    screenUV.x = screenUV.x * 0.5f + 0.5f;
    screenUV.y = screenUV.y * -0.5f + 0.5f;

    // そのピクセルのワールド座標を取得
    float4 worldPosData = gPositionTex.Sample(mySampler, screenUV);
    float3 worldPos = worldPosData.xyz;

    // 座標がない場所は描画しない
    if (worldPosData.w <= 0.0f)
        discard;

    // ワールド座標 -> デカールローカル座標へ変換 (逆行列)
    float4 localPos = mul(float4(worldPos, 1.0f), InverseWorld);

    // Boxの範囲内 (-0.5 ~ 0.5) かチェック
    // 範囲外なら描画しない (clip)
    // マージンを持たせるなら 0.5f
    clip(0.5f - abs(localPos.x));
    clip(0.5f - abs(localPos.y));
    clip(0.5f - abs(localPos.z));

    // テクスチャマッピング
    // localPos (-0.5 ~ 0.5) => UV (0.0 ~ 1.0)
    float2 decalUV = localPos.xz + 0.5f;
    decalUV.y = 1.0f - decalUV.y; // V反転 (テクスチャによる)

    // テクスチャサンプリング
    float4 texColor = myTexture.Sample(mySampler, decalUV);

    // 色合成
    float4 finalColor = texColor * DecalColor;
    
    // 透明部分は描かない
    clip(finalColor.a - 0.01f);

    return finalColor;
}
