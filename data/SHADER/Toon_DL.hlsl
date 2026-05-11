// DeferredLighting.hlsl

#include "Lighting.hlsli"

// --------------------------------------------------------
// ピクセルシェーダー (Pixel Shader)
// --------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    int3 sampleCoord = int3(input.Pos.xy, 0);

    // G-Buffer読み込み
    float4 diffData = gDiffuse.Load(sampleCoord);
    float4 normData = gNormal.Load(sampleCoord);
    float4 posData = gPosition.Load(sampleCoord);
    float4 emiData = gEmissive.Load(sampleCoord);

    // データ展開
    float3 Albedo = diffData.rgb; // リニア色
    float SpecIntensity = diffData.a; // スペキュラー強度
    float3 Normal = normalize(normData.xyz); // 法線
    float LightFlag = normData.w; // ライト有効フラグ
    float3 WorldPos = posData.xyz; // ワールド座標
    float3 Emissive = emiData.rgb; // 自己発光
    float SpecPower = emiData.a * 100.0f; // 圧縮解除

    // 結果格納用
    float3 finalColor = float3(0, 0, 0);

    // ライティング計算
    if (LightFlag > 0.5f)
    {
        //--------------------
        // 影の計算
        //--------------------
        float shadowFactor = 1.0f;
        
        // ワールド座標をライトのプロジェクション空間へ変換
        float4 lightSpacePos = mul(float4(WorldPos, 1.0f), LightViewProj);
        
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
            float closestDepth = gShadowMap.Sample(gSampler, shadowUV).r;

            // 今のピクセルの深度 (ライトからの距離)
            float currentDepth = lightSpacePos.z;

            // 今の場所が、一番近い場所より奥にあれば「影」
            float3 L = normalize(Lights[0].Position.xyz - WorldPos);
            float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
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

        // 視線ベクトル (カメラへ向かうベクトル)
        float3 ViewDir = normalize(CameraPos.xyz - WorldPos);

        // --------------------------------------------------
        // ライト計算ループ
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
                float3 lightDir = Lights[i].Position.xyz - WorldPos;
                float dist = length(lightDir);
                L = normalize(lightDir);
                // 簡易減衰
                attenuation = clamp(1.0f - dist / 1000.0f, 0.0f, 1.0f);
            }

            float3 lightColor = Lights[i].Color.rgb * attenuation;

            // ------------------------------------------------
            // トゥーン拡散反射
            // ------------------------------------------------
            float NdotL = dot(Normal, L); // -1.0 ~ 1.0
            
            // 影の閾値
            float threshold = 0.5f;
            // 境界のぼかし幅 (0.01くらいでアンチエイリアス効果、0.0だとジャギーが出る)
            float smoothWidth = 0.01f;

            // shadowFactor が 0.9未満なら「影」とみなして、NdotL を強制的にマイナスにする
            if (shadowFactor < 0.9f)
            {
                NdotL = -1.0f;
            }

            // トゥーン化 (Ramp計算)
            // NdotL が threshold を超えたら 1.0 (明るい)、以下なら 0.0 (暗い)
            float ramp = smoothstep(threshold - smoothWidth, threshold + smoothWidth, NdotL);

            // 加算 (影の色は Ambient に任せるため、ここは単純に光の分を足す)
            totalDiffuse += lightColor * ramp;


            // ------------------------------------------------
            // トゥーン鏡面反射
            // ------------------------------------------------
            // 光が当たっている場所(ramp > 0)かつ、スペキュラー強度が設定されている場合のみ
            if (ramp > 0.0f && SpecIntensity > 0.0f)
            {
                // ハーフベクトル
                float3 H = normalize(L + ViewDir);
                float NdotH = dot(Normal, H);

                // ハイライトの大きさを調整 (Powerが大きいほど小さくなる)
                // スペキュラーもパキッと出すために step 関数を使う
                // 0.9 などの閾値を超えたら 1.0、それ以外は 0.0
                float specThreshold = 1.0f - (1.0f / max(1.0f, SpecPower * 0.1f));
                float specRamp = step(specThreshold, NdotH);

                totalSpecular += lightColor * SpecIntensity * specRamp;
            }
        }

        // 最終合成
        // 影の部分は Ambient の色になり、光が当たるところは +Diffuse になる
        // 以前のように shadowFactor を乗算する必要はありません (ループ内で処理済み)
        finalColor = (GlobalAmbient.rgb * Albedo)
                   + (totalDiffuse * Albedo)
                   + totalSpecular
                   + Emissive;
    }
    else
    {
        // ライト無効時
        finalColor = Albedo + Emissive;
    }

    // Fog
    finalColor = CalculateFog(finalColor, WorldPos.xyz);
    
    return float4(finalColor, 1.0f);
}
