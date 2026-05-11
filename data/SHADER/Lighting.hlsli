#ifndef __LIGHTING_HLSLI__
#define __LIGHTING_HLSLI__

#define MAX_LIGHTS 8 // ライト数

// --------------------------------------------------------
// ライティングモデル定義
// --------------------------------------------------------
#define SHADING_MODEL_UNLIT 0
#define SHADING_MODEL_PHONG 1
#define SHADING_MODEL_TOON  2

// --------------------------------------------------------
// 構造体定義
// --------------------------------------------------------

// 位置とUVのみでいい
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

struct LightData
{
    float4 Position; // w=0:Directional, w=1:Point
    float4 Color;
    float4 Direction;
};

// b3: ライト情報
cbuffer LightBuffer : register(b3)
{
    float4 GlobalAmbient;
    float4 CameraPos;
    int LightCount;
    float3 _padding;
    LightData Lights[MAX_LIGHTS];
}

// b4: シャドウ用ライト行列
cbuffer ShadowBuffer : register(b4)
{
    row_major matrix LightViewProj; // ライト視点の View * Proj
}

// b5: Fog
cbuffer FogBuffer : register(b5)
{
    float4 fogColor;     // 色
    float fogStart;      // 開始距離
    float fogEnd;        // 終了距離
    float skyFogHeight;  // Fogの高さ (これ以上はFogがかからない)
    float horizonHeight; // 地平線の高さ
    float fogPower;      // Fogのかかり方の係数
    float skyFogPower;   // SkyFogのかかり方の係数
    float2 padding_;
}

// --------------------------------------------------------
// G-Buffer テクスチャ (SRV)
// --------------------------------------------------------
Texture2D gDiffuse : register(t1); // Albedo
Texture2D gNormal : register(t2); // Normal
Texture2D gPosition : register(t3); // WorldPos
Texture2D gEmissive : register(t4); // Emissive

// --------------------------------------------------------
// ShadowMap テクスチャ (SRV)
// --------------------------------------------------------
Texture2D gShadowMap : register(t5); // ShadowMap

// --------------------------------------------------------
// Lambert (拡散反射) 計算関数
// --------------------------------------------------------
// N: 法線 (正規化済み)
// L: ライト方向 (正規化済み, 光源へ向かう向き)
// lightColor: ライトの色 * 減衰率
// diffuseColor: [出力] 結果を加算する変数
// --------------------------------------------------------
void CalculateLambert(
    float3 N,
    float3 L,
    float3 lightColor,
    inout float3 diffuseColor
)
{
    // 法線とライトの内積 (0未満はカット)
    float NdotL = max(0.0f, dot(N, L));
    
    // ライト色 * 強さ を加算
    diffuseColor += lightColor * NdotL;
}

// --------------------------------------------------------
// Blinn-Phong (拡散 + 鏡面反射) 計算関数
// --------------------------------------------------------
// V: 視線ベクトル (正規化済み)
// specPower: スペキュラーの鋭さ (Material Power)
// specIntensity: スペキュラーの強さ (Material Specular)
// specularColor: [出力] 結果を加算する変数
// --------------------------------------------------------
void CalculateBlinnPhong(
    float3 N,
    float3 L,
    float3 V,
    float3 lightColor,
    float specPower,
    float specIntensity,
    inout float3 diffuseColor,
    inout float3 specularColor
)
{
    // 法線とライトの内積 (0未満はカット)
    float NdotL = max(0.0f, dot(N, L));
    
    // ライト色 * 強さ を加算
    diffuseColor += lightColor * NdotL;
    
    if (NdotL > 0.0f && specIntensity > 0.0f)
    {
        // ハーフベクトル
        float3 H = normalize(L + V);
        float NdotH = max(0.0f, dot(N, H));
        
        // スペキュラー計算
        float spec = pow(NdotH, specPower) * specIntensity;
        
        // 加算
        specularColor += lightColor * spec;

        // 拡散反射色をスペキュラー分だけ減衰させる
        diffuseColor *= (1.0 - specIntensity);
    }
}

// --------------------------------------------------------
// Toon (NPR) 計算関数
// --------------------------------------------------------
void CalculateToon(
    float3 N,
    float3 L,
    float3 V,
    float3 lightColor,
    float specPower,
    float specIntensity,
    float shadowFactor,
    inout float3 diffuseColor,
    inout float3 specularColor
)
{
    float NdotL = dot(N, L); // -1.0 ~ 1.0

    // 影の閾値とぼかし
    float threshold = 0.5f;
    float smoothWidth = 0.01f;

    // シャドウマップで影になっている場合、強制的に暗部へ
    if (shadowFactor < 0.9f)
    {
        NdotL = -1.0f;
    }

    // Ramp計算 (2階調化)
    float ramp = smoothstep(threshold - smoothWidth, threshold + smoothWidth, NdotL);
    diffuseColor += lightColor * ramp;

    // スペキュラー
    if (ramp > 0.0f && specIntensity > 0.0f)
    {
        float3 H = normalize(L + V);
        float NdotH = dot(N, H);
        
        // スペキュラーもパキッとさせる
        float specThreshold = 1.0f - (1.0f / max(1.0f, specPower * 0.1f));
        float specRamp = step(specThreshold, NdotH);
        
        specularColor += lightColor * specIntensity * specRamp;
    }
}

float3 CalculateFog(float3 color,float3 WorldPos)
{
    // カメラからの距離を計算
    float dist = length(CameraPos.xyz - WorldPos);

    // フォグ係数
    float fogFactor = pow(saturate((dist - fogStart) / (fogEnd - fogStart)), fogPower);

    // フォグの色を混ぜる
    return lerp(color, fogColor.rgb, fogFactor);
}

#endif // __LIGHTING_HLSLI__
