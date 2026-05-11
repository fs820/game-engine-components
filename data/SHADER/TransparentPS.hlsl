#include "Common.hlsli"
#include "Lighting.hlsli"

// エントリーポイント
float4 PS(PS_INPUT input) : SV_Target
{
    // テクスチャの色とマテリアルの色を合成
    float4 baseColor = myTexture.Sample(mySampler, input.UV) * input.Color * MaterialDiffuse;

    // アルファカットオフ
    clip(baseColor.a - MaterialAlphaCutoff);

    // 結果格納用
    float3 finalColor = float3(0, 0, 0);

    // ライティング計算 (Unlit以外なら計算)
    if (PixelShaderType != SHADING_MODEL_UNLIT)
    {
        //--------------------
        // 影の計算
        //--------------------
        float shadowFactor = 1.0f;
        
        // ワールド座標をライトのプロジェクション空間へ変換
        float4 lightSpacePos = mul(float4(input.WorldPos, 1.0f), LightViewProj);
        
        // w除算 (正規化デバイス座標へ: -1.0 ~ 1.0)
        lightSpacePos.xyz /= lightSpacePos.w;

        // UV座標へ変換 (-1~1 -> 0~1)
        // X: (-1 -> 0, 1 -> 1)  => *0.5 + 0.5
        // Y: ( 1 -> 0, -1 -> 1) => *-0.5 + 0.5 (Yは上下反転)
        float2 shadowUV = lightSpacePos.xy * float2(0.5f, -0.5f) + 0.5f;

        // シャドウマップ範囲内かチェック
        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
            shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
        {
            // シャドウマップから深度値を取得 (ライトから一番近い距離)
            float closestDepth = gShadowMap.Sample(mySampler, shadowUV).r;

            // 今のピクセルの深度 (ライトからの距離)
            float currentDepth = lightSpacePos.z;

            // 今の場所が、一番近い場所より奥にあれば「影」
            float3 L = normalize(Lights[0].Position.xyz - input.WorldPos);
            float cosTheta = clamp(dot(normalize(input.Normal), L), 0.0, 1.0);
            float bias = 0.005 * tan(acos(cosTheta)); // 角度がきついほどバイアスを増やす
            bias = clamp(bias, 0.0, 0.01);
            if (currentDepth - bias > closestDepth)
            {
                shadowFactor = 0.5f;
            }
        }
        //--------------------
        // 影の計算 End
        //--------------------
        
        float3 totalDiffuse = float3(0, 0, 0);
        float3 totalSpecular = float3(0, 0, 0);
        float3 ViewDir = normalize(CameraPos.xyz - input.WorldPos);

        // --------------------------------------------------
        // ライトループ
        // --------------------------------------------------
        for (int i = 0; i < LightCount; ++i)
        {
            float3 L;
            float attenuation = 1.0f;
            
            // ライトの種類判定
            if (Lights[i].Position.w == 0.0f)
            {
                // 平行光源
                L = normalize(-Lights[i].Direction.xyz);
            }
            else
            {
                // 点光源
                float3 lightDir = Lights[i].Position.xyz - input.WorldPos;
                float dist = length(lightDir);
                L = normalize(lightDir);
                // 簡易減衰
                attenuation = clamp(1.0f - dist / 1000.0f, 0.0f, 1.0f);
            }
            
            float3 lightColor = Lights[i].Color.rgb * attenuation;

            // ★分岐して計算関数を呼ぶ
            if (PixelShaderType == SHADING_MODEL_PHONG)
            {
                CalculateBlinnPhong(normalize(input.Normal), L, ViewDir, lightColor, MaterialPower, (MaterialSpecular.r + MaterialSpecular.g + MaterialSpecular.b) / 3.0f, totalDiffuse, totalSpecular);
            }
            else if (PixelShaderType == SHADING_MODEL_TOON)
            {
                // Toonは影の影響を内部で処理するためshadowFactorを渡す
                CalculateToon(normalize(input.Normal), L, ViewDir, lightColor, MaterialPower, (MaterialSpecular.r + MaterialSpecular.g + MaterialSpecular.b) / 3.0f, shadowFactor, totalDiffuse, totalSpecular);
            }
        }

        // 最終合成
        // Toonの場合はCalculateToon内でshadowFactorを考慮済みだが、Diffuse項には影響させたい場合調整
        // BlinnPhongの場合はここでshadowFactorを乗算
        if (PixelShaderType == SHADING_MODEL_PHONG)
        {
            totalDiffuse *= shadowFactor;
            totalSpecular *= shadowFactor;
        }

        finalColor = (GlobalAmbient.rgb * baseColor.rgb)
                   + (totalDiffuse * baseColor.rgb)
                   + totalSpecular
                   + MaterialEmissive.rgb;
    }
    else
    {
        // Unlit
        finalColor = baseColor.rgb + MaterialEmissive.rgb;
    }

    // Fog
    finalColor = CalculateFog(finalColor, input.WorldPos.xyz);
    
    return float4(finalColor, baseColor.a);
}
