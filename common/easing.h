//---------------------------------------
//
//イージング処理の定義・宣言[easing.h]
//Author null
//
//---------------------------------------
#pragma once
#include <cmath> // 計算関数使用

//---------------------------------------
// double型のイージング処理の名前空間
//---------------------------------------
namespace easing
{
    // 数学定数
    inline constexpr double PI = 3.14159265358979323846;
    inline constexpr double TWO_PI = 6.28318530717958647692;
    inline constexpr double HALF_PI = 1.57079632679489661923;
	inline constexpr double QUARTER_PI = 0.78539816339744830962;

    // Linear
    inline double easeLinear(double x)
    {
        return x;
    }

    // Sine
    inline double easeInSine(double x)
    {
        return 1 - std::cos((x * PI) / 2);
    }

    inline double easeOutSine(double x)
    {
        return std::sin((x * PI) / 2);
    }

    inline double easeInOutSine(double x)
    {
        return -(std::cos(PI * x) - 1) / 2;
    }

    // Quad
    inline double easeInQuad(double x)
    {
        return x * x;
    }

    inline double easeOutQuad(double x)
    {
        return 1 - (1 - x) * (1 - x);
    }

    inline double easeInOutQuad(double x)
    {
        return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2;
    }

    // Cubic
    inline double easeInCubic(double x)
    {
        return x * x * x;
    }

    inline double easeOutCubic(double x)
    {
        return 1 - std::pow(1 - x, 3);
    }

    inline double easeInOutCubic(double x)
    {
        return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3) / 2;
    }

    // Quart
    inline double easeInQuart(double x)
    {
        return x * x * x * x;
    }

    inline double easeOutQuart(double x)
    {
        return 1 - std::pow(1 - x, 4);
    }

    inline double easeInOutQuart(double x)
    {
        return x < 0.5 ? 8 * x * x * x * x : 1 - std::pow(-2 * x + 2, 4) / 2;
    }

    // Quint
    inline double easeInQuint(double x)
    {
        return x * x * x * x * x;
    }

    inline double easeOutQuint(double x)
    {
        return 1 - std::pow(1 - x, 5);
    }

    inline double easeInOutQuint(double x)
    {
        return x < 0.5 ? 16 * x * x * x * x * x : 1 - std::pow(-2 * x + 2, 5) / 2;
    }

    // Expo
    inline double easeInExpo(double x)
    {
        return x == 0 ? 0 : std::pow(2, 10 * x - 10);
    }

    inline double easeOutExpo(double x)
    {
        return x == 1 ? 1 : 1 - std::pow(2, -10 * x);
    }

    inline double easeInOutExpo(double x)
    {
        return x == 0
            ? 0
            : x == 1
            ? 1
            : x < 0.5 ? std::pow(2, 20 * x - 10) / 2
            : (2 - std::pow(2, -20 * x + 10)) / 2;
    }

    // Circ
    inline double easeInCirc(double x)
    {
        return 1 - std::sqrt(1 - std::pow(x, 2));
    }

    inline double easeOutCirc(double x)
    {
        return std::sqrt(1 - std::pow(x - 1, 2));
    }

    inline double easeInOutCirc(double x)
    {
        return x < 0.5
            ? (1 - std::sqrt(1 - std::pow(2 * x, 2))) / 2
            : (std::sqrt(1 - std::pow(-2 * x + 2, 2)) + 1) / 2;
    }

    // Back
    inline double easeInBack(double x)
    {
        const double c1 = 1.70158;
        const double c3 = c1 + 1;

        return c3 * x * x * x - c1 * x * x;
    }

    inline double easeOutBack(double x)
    {
        const double c1 = 1.70158;
        const double c3 = c1 + 1;

        return 1 + c3 * std::pow(x - 1, 3) + c1 * std::pow(x - 1, 2);
    }

    inline double easeInOutBack(double x)
    {
        const double c1 = 1.70158;
        const double c2 = c1 * 1.525;

        return x < 0.5
            ? (std::pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
            : (std::pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
    }

    // Elastic
    inline double easeInElastic(double x)
    {
        const double c4 = (2 * PI) / 3;

        return x == 0
            ? 0
            : x == 1
            ? 1
            : -std::pow(2, 10 * x - 10) * std::sin((x * 10 - 10.75) * c4);
    }

    inline double easeOutElastic(double x)
    {
        const double c4 = (2 * PI) / 3;

        return x == 0
            ? 0
            : x == 1
            ? 1
            : std::pow(2, -10 * x) * std::sin((x * 10 - 0.75) * c4) + 1;
    }

    inline double easeInOutElastic(double x)
    {
        const double c5 = (2 * PI) / 4.5;

        return x == 0
            ? 0
            : x == 1
            ? 1
            : x < 0.5
            ? -(std::pow(2, 20 * x - 10) * std::sin((20 * x - 11.125) * c5)) / 2
            : (std::pow(2, -20 * x + 10) * std::sin((20 * x - 11.125) * c5)) / 2 + 1;
    }

    // Bounce
    inline double easeOutBounce(double x)
    {
        const double n1 = 7.5625;
        const double d1 = 2.75;

        if (x < 1 / d1)
        {
            return n1 * x * x;
        }
        else if (x < 2 / d1)
        {
            return n1 * (x -= 1.5 / d1) * x + 0.75;
        }
        else if (x < 2.5 / d1)
        {
            return n1 * (x -= 2.25 / d1) * x + 0.9375;
        }
        else
        {
            return n1 * (x -= 2.625 / d1) * x + 0.984375;
        }
    }

    inline double easeInBounce(double x)
    {
        return 1 - easeOutBounce(1 - x);
    }

    inline double easeInOutBounce(double x)
    {
        return x < 0.5
            ? (1 - easeOutBounce(1 - 2 * x)) / 2
            : (1 + easeOutBounce(2 * x - 1)) / 2;
    }
}

//---------------------------------------
// float型のイージング処理の名前空間
//---------------------------------------
namespace easingf
{
    // 数学定数
    inline constexpr float PI = 3.14159265358979323846f;
    inline constexpr float TWO_PI = 6.28318530717958647692f;
    inline constexpr float HALF_PI = 1.57079632679489661923f;
    inline constexpr float QUARTER_PI = 0.78539816339744830962f;

    // Linear
    inline float easeLinear(float x)
    {
        return x;
    }

    // Sine
    inline float easeInSine(float x)
    {
        return 1.0f - std::cosf((x * PI) / 2.0f);
    }

    inline float easeOutSine(float x)
    {
        return std::sinf((x * PI) / 2.0f);
    }

    inline float easeInOutSine(float x)
    {
        return -(std::cosf(PI * x) - 1.0f) / 2.0f;
    }

    // Quad
    inline float easeInQuad(float x)
    {
        return x * x;
    }

    inline float easeOutQuad(float x)
    {
        return 1.0f - (1.0f - x) * (1.0f - x);
    }

    inline float easeInOutQuad(float x)
    {
        return x < 0.5f ? 2.0f * x * x : 1.0f - std::powf(-2.0f * x + 2.0f, 2.0f) / 2.0f;
    }

    // Cubic
    inline float easeInCubic(float x)
    {
        return x * x * x;
    }

    inline float easeOutCubic(float x)
    {
        return 1.0f - std::powf(1.0f - x, 3.0f);
    }

    inline float easeInOutCubic(float x)
    {
        return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::powf(-2.0f * x + 2.0f, 3.0f) / 2.0f;
    }

    // Quart
    inline float easeInQuart(float x)
    {
        return x * x * x * x;
    }

    inline float easeOutQuart(float x)
    {
        return 1.0f - std::powf(1.0f - x, 4.0f);
    }

    inline float easeInOutQuart(float x)
    {
        return x < 0.5f ? 8.0f * x * x * x * x : 1.0f - std::powf(-2.0f * x + 2.0f, 4.0f) / 2.0f;
    }

    // Quint
    inline float easeInQuint(float x)
    {
        return x * x * x * x * x;
    }

    inline float easeOutQuint(float x)
    {
        return 1.0f - std::powf(1.0f - x, 5.0f);
    }

    inline float easeInOutQuint(float x)
    {
        return x < 0.5f ? 16.0f * x * x * x * x * x : 1.0f - std::powf(-2.0f * x + 2.0f, 5.0f) / 2.0f;
    }

    // Expo
    inline float easeInExpo(float x)
    {
        return x == 0.0f ? 0.0f : std::powf(2.0f, 10.0f * x - 10.0f);
    }

    inline float easeOutExpo(float x)
    {
        return x == 1.0f ? 1.0f : 1.0f - std::powf(2.0f, -10.0f * x);
    }

    inline float easeInOutExpo(float x)
    {
        return x == 0.0f
            ? 0.0f
            : x == 1.0f
            ? 1.0f
            : x < 0.5f ? std::powf(2.0f, 20.0f * x - 10.0f) / 2.0f
            : (2.0f - std::powf(2.0f, -20.0f * x + 10.0f)) / 2.0f;
    }

    // Circ
    inline float easeInCirc(float x)
    {
        return 1.0f - std::sqrtf(1.0f - std::powf(x, 2.0f));
    }

    inline float easeOutCirc(float x)
    {
        return std::sqrtf(1.0f - std::powf(x - 1.0f, 2.0f));
    }

    inline float easeInOutCirc(float x)
    {
        return x < 0.5f
            ? (1.0f - std::sqrtf(1.0f - std::powf(2.0f * x, 2.0f))) / 2.0f
            : (std::sqrtf(1.0f - std::powf(-2.0f * x + 2.0f, 2.0f)) + 1.0f) / 2.0f;
    }

    // Back
    inline float easeInBack(float x)
    {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;

        return c3 * x * x * x - c1 * x * x;
    }

    inline float easeOutBack(float x)
    {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;

        return 1.0f + c3 * std::powf(x - 1.0f, 3.0f) + c1 * std::powf(x - 1.0f, 2.0f);
    }

    inline float easeInOutBack(float x)
    {
        const float c1 = 1.70158f;
        const float c2 = c1 * 1.525f;

        return x < 0.5f
            ? (std::powf(2.0f * x, 2.0f) * ((c2 + 1.0f) * 2.0f * x - c2)) / 2.0f
            : (std::powf(2.0f * x - 2.0f, 2.0f) * ((c2 + 1.0f) * (x * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
    }

    // Elastic
    inline float easeInElastic(float x)
    {
        const float c4 = (2.0f * PI) / 3.0f;

        return x == 0.0f
            ? 0.0f
            : x == 1.0f
            ? 1.0f
            : -std::powf(2.0f, 10.0f * x - 10.0f) * std::sinf((x * 10.0f - 10.75f) * c4);
    }

    inline float easeOutElastic(float x)
    {
        const float c4 = (2.0f * PI) / 3.0f;

        return x == 0.0f
            ? 0.0f
            : x == 1.0f
            ? 1.0f
            : std::powf(2.0f, -10.0f * x) * std::sinf((x * 10.0f - 0.75f) * c4) + 1.0f;
    }

    inline float easeInOutElastic(float x)
    {
        const float c5 = (2.0f * PI) / 4.5f;

        return x == 0.0f
            ? 0.0f
            : x == 1.0f
            ? 1.0f
            : x < 0.5f
            ? -(std::powf(2.0f, 20.0f * x - 10.0f) * std::sinf((20.0f * x - 11.125f) * c5)) / 2.0f
            : (std::powf(2.0f, -20.0f * x + 10.0f) * std::sinf((20.0f * x - 11.125f) * c5)) / 2.0f + 1.0f;
    }

    // Bounce
    inline float easeOutBounce(float x)
    {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;

        if (x < 1.0f / d1)
        {
            return n1 * x * x;
        }
        else if (x < 2.0f / d1)
        {
            return n1 * (x -= 1.5f / d1) * x + 0.75f;
        }
        else if (x < 2.5f / d1)
        {
            return n1 * (x -= 2.25f / d1) * x + 0.9375f;
        }
        else
        {
            return n1 * (x -= 2.625f / d1) * x + 0.984375f;
        }
    }

    inline float easeInBounce(float x)
    {
        return 1.0f - easeOutBounce(1.0f - x);
    }

    inline float easeInOutBounce(float x)
    {
        return x < 0.5f
            ? (1.0f - easeOutBounce(1.0f - 2.0f * x)) / 2.0f
            : (1.0f + easeOutBounce(2.0f * x - 1.0f)) / 2.0f;
    }
}
