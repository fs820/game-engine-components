// Blinn-Phong_DL.hlsl

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
    float3 Albedo = diffData.rgb;            // リニア色
    float SpecIntensity = diffData.a;        // スペキュラー強度
    float3 Normal = normalize(normData.xyz); // 法線
    float LightFlag = normData.w;            // ライト有効フラグ
    float3 WorldPos = posData.xyz;           // ワールド座標
    float3 Emissive = emiData.rgb;           // 自己発光
    float SpecPower = emiData.a * 100.0f;    // 圧縮解除

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

            // 関数化部分: Blinn-Phong計算
            CalculateBlinnPhong(
                Normal,
                L,
                ViewDir,
                Lights[i].Color.rgb * attenuation, // 減衰込みのライト色
                SpecPower,
                SpecIntensity,
                totalDiffuse, // 結果を加算して返す
                totalSpecular // 結果を加算して返す
            );
        }

        // 最終合成: (Ambient + Diffuse) * Albedo + Specular + Emissive
        finalColor = (GlobalAmbient.rgb * Albedo)
                   + (totalDiffuse * Albedo * shadowFactor)
                   + (totalSpecular * shadowFactor)
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
