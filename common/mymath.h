#pragma once
#include "math_types.h"

namespace math
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = PI * 2.0f;

    inline float degreesToRadians(float degrees)
    {
        return degrees * (PI / 180.0f);
    }
    inline float radiansToDegrees(float radians)
    {
        return radians * (180.0f / PI);
    }

    inline float lerp(float start, float end, float t)
    {
        return start + (end - start) * t;
    }

    // ThetaのLerp (最短ルートを通る)
    inline float lerpTheta(float start, float end, float t)
    {
        float diff = end - start;

        // 差分を -PI ～ +PI の範囲に補正する
        // これにより「近い方」が選ばれる
        while (diff < -PI) diff += TWO_PI;
        while (diff > PI)  diff -= TWO_PI;

        return start + diff * t;
    }

    // theta（方位角）の正規化: 0 ～ 2π の範囲に収める
    inline float normalizeTheta(float theta)
    {
        // fmod で剰余を取る
        theta = fmodf(theta, 2.0f * PI);

        // 負の値の場合は正の値に変換
        if (theta < 0.0f)
            theta += 2.0f * PI;

        return theta;
    }

    // phi（天頂角）の正規化: epsilon ～ π-epsilon の範囲に制限
    inline float normalizePhi(float phi, float epsilon = 0.01f)
    {
        // π の範囲に制限
        if (phi < epsilon)
            phi = epsilon;
        else if (phi > PI - epsilon)
            phi = PI - epsilon;

        return phi;
    }
}
