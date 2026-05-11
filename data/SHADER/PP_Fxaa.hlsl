#include "PostProcess.hlsli"

// 輝度(Luma)を計算する関数
float GetLuma(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

// ----------------------------------------------------
// FXAA (Fast Approximate Anti-Aliasing)
// ----------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    // 1ピクセルのUVサイズ
    float2 rcpFrame = ScreenSize.zw;

    // 周囲の輝度を取得 (N=上, S=下, W=左, E=右, M=中心)
    float3 rgbM = gSceneTexture.Sample(gSampler, input.UV).rgb;
    float3 rgbN = gSceneTexture.Sample(gSampler, input.UV + float2(0, -1) * rcpFrame).rgb;
    float3 rgbS = gSceneTexture.Sample(gSampler, input.UV + float2(0, +1) * rcpFrame).rgb;
    float3 rgbE = gSceneTexture.Sample(gSampler, input.UV + float2(+1, 0) * rcpFrame).rgb;
    float3 rgbW = gSceneTexture.Sample(gSampler, input.UV + float2(-1, 0) * rcpFrame).rgb;

    float lumaM = GetLuma(rgbM);
    float lumaN = GetLuma(rgbN);
    float lumaS = GetLuma(rgbS);
    float lumaE = GetLuma(rgbE);
    float lumaW = GetLuma(rgbW);

    // エッジ検出 (最大輝度と最小輝度の差)
    float lumaMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
    float lumaRange = lumaMax - lumaMin;

    // コントラストが低い(エッジじゃない)なら、そのまま中心の色を返す
    if (lumaRange < 0.05f) // しきい値
    {
        return float4(rgbM, 1.0f);
    }

    // さらに広範囲(コーナー)の輝度を取得
    float3 rgbNW = gSceneTexture.Sample(gSampler, input.UV + float2(-1, -1) * rcpFrame).rgb;
    float3 rgbNE = gSceneTexture.Sample(gSampler, input.UV + float2(+1, -1) * rcpFrame).rgb;
    float3 rgbSW = gSceneTexture.Sample(gSampler, input.UV + float2(-1, +1) * rcpFrame).rgb;
    float3 rgbSE = gSceneTexture.Sample(gSampler, input.UV + float2(+1, +1) * rcpFrame).rgb;

    float lumaNW = GetLuma(rgbNW);
    float lumaNE = GetLuma(rgbNE);
    float lumaSW = GetLuma(rgbSW);
    float lumaSE = GetLuma(rgbSE);

    // ぼかし計算 (2種類のブレンド結果を作る)
    float3 rgbL = (rgbN + rgbS + rgbE + rgbW + rgbM) * (1.0 / 5.0); // 十字ブレンド
    float3 rgbK = (rgbNW + rgbNE + rgbSW + rgbSE) * (1.0 / 4.0); // 四隅ブレンド
    
    // 重みづけ合成 (簡易版)
    // 本来のFXAAはもっと複雑なエッジ方向検出を行いますが、
    // 簡易版でも十分にジャギーは消えます
    
    // 十字方向の輝度変化と、四隅方向の輝度変化を比較
    float dirN = abs(lumaN - lumaM);
    float dirS = abs(lumaS - lumaM);
    float dirE = abs(lumaE - lumaM);
    float dirW = abs(lumaW - lumaM);
    
    // 変化が激しい方向に対して垂直にぼかすのが理想だが、
    // ここでは単純に「周囲4点ブレンド」と「中心含む5点ブレンド」を混ぜる
    
    // より高品質なFXAA (Console版ロジックに近いもの)
    float dirReduce = 1.0 / 128.0;
    float dirMin = 1.0 / 8.0;
    
    float lumaSum = lumaN + lumaS + lumaE + lumaW;
    float dir1 = min(abs(lumaN + lumaS - 2.0 * lumaM), abs(lumaE + lumaW - 2.0 * lumaM));
    float dir2 = max(abs(lumaN + lumaS - 2.0 * lumaM), abs(lumaE + lumaW - 2.0 * lumaM));
    
    // ベクトル計算
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce2 = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * dirReduce), dirMin);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce2);
    
    dir = min(float2(8.0, 8.0), max(float2(-8.0, -8.0), dir * rcpDirMin)) * rcpFrame;
    
    // 2回サンプリングしてブレンド
    float3 rgbA = 0.5 * (
        gSceneTexture.Sample(gSampler, input.UV + dir * (1.0 / 3.0 - 0.5)).rgb +
        gSceneTexture.Sample(gSampler, input.UV + dir * (2.0 / 3.0 - 0.5)).rgb);
        
    float3 rgbB = rgbA * 0.5 + 0.25 * (
        gSceneTexture.Sample(gSampler, input.UV + dir * -0.5).rgb +
        gSceneTexture.Sample(gSampler, input.UV + dir * 0.5).rgb);
        
    float lumaB = GetLuma(rgbB);
    
    // ブレンド結果が輝度範囲外なら A を採用、範囲内なら B を採用
    float3 finalColor;
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        finalColor = rgbA;
    }
    else
    {
        finalColor = rgbB;
    }
    
    return float4(finalColor, 1.0f);
}
