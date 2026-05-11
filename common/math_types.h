//--------------------------------------------
//
// 数学型定義 [math_types.h]
// Author: Fuma Sato
// Row-Major (行ベクトル方式) 完全統一版
//
//--------------------------------------------
#pragma once
#include <cmath>
#include <algorithm>

// --------------------------------------------------------
// 1. 前方宣言
// --------------------------------------------------------
struct Matrix;
struct Vector3;
struct Quaternion;
struct Transform;
struct Color;

// --------------------------------------------------------
// 2. 構造体定義
// --------------------------------------------------------

struct Vector2
{
    float x, y;
    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}
    ~Vector2() = default;

    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(const Vector2& other) const { return Vector2(x * other.x, y * other.y); }
    Vector2 operator/(const Vector2& other) const { return Vector2(x / other.x, y / other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { if (scalar != 0.0f) { return Vector2(x / scalar, y / scalar); } return Vector2(); }
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(const Vector2& other) { x *= other.x; y *= other.y; return *this; }
    Vector2& operator/=(const Vector2& other) { x /= other.x; y /= other.y; return *this; }
    Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { if (scalar != 0.0f) { x /= scalar; y /= scalar; } return *this; }

    void zero() { x = 0.0f; y = 0.0f; }
    void one() { x = 1.0f; y = 1.0f; }

    float length() const { return sqrtf(x * x + y * y); }
    void normalize()
    {
        float len = length();
        if (len > 0.0f) { x /= len; y /= len; }
    }

    static Vector2 Zero() { return Vector2(0.0f, 0.0f); }
    static Vector2 One() { return Vector2(1.0f, 1.0f); }
};

struct Vector3
{
    float x, y, z;
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3(const Vector2& vec2, float z) : x(vec2.x), y(vec2.y), z(z) {}
    ~Vector3() = default;

    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(const Vector3& other) const { return Vector3(x * other.x, y * other.y, z * other.z); }
    Vector3 operator/(const Vector3& other) const { return Vector3(x / other.x, y / other.y, z / other.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3& operator*=(const Vector3& other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
    Vector3& operator/=(const Vector3& other) { x /= other.x; y /= other.y; z /= other.z; return *this; }
    Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vector3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

    void zero() { x = 0.0f; y = 0.0f; z = 0.0f; }
    void one() { x = 1.0f; y = 1.0f; z = 1.0f; }

    void transform(const Matrix& mat);
    void transformNormal(const Matrix& mat);
    void transformCoord(const Matrix& mat);

    float dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
    Vector3 cross(const Vector3& other) const
    {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    void normalize()
    {
        float len = length();
        if (len != 0.0f) { x /= len; y /= len; z /= len; }
    }
    float length() const { return sqrtf(x * x + y * y + z * z); }

    // 自身の位置から球座標を取得
    void toSpherical(float& radius, float& theta, float& phi) const
    {
        radius = length();
        if (radius == 0.0f)
        {
            theta = 0.0f;
            phi = 0.0f;
            return;
        }

        theta = atan2f(z, x);    // 方位角
        phi = acosf(y / radius); // 天頂角
    }

    static Vector3 Zero() { return Vector3(0.0f, 0.0f, 0.0f); }
    static Vector3 One() { return Vector3(1.0f, 1.0f, 1.0f); }

    static float Dot(const Vector3& a, const Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    static Vector3 Lerp(const Vector3& start, const Vector3& end, float t)
    {
        return start + (end - start) * t;
    }

    // 球座標から直交座標への変換
    static Vector3 FromSpherical(float radius, float theta, float phi)
    {
        float x = radius * sinf(phi) * cosf(theta); // 方位角theta、天頂角phi
        float z = radius * sinf(phi) * sinf(theta); // 方位角theta、天頂角phi
        float y = radius * cosf(phi);               // 天頂角phi
        return Vector3(x, y, z);
    }
};

struct Vector4
{
    float x, y, z, w;
    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(Vector3 vec3, float w) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {}
    ~Vector4() = default;

    Vector4 operator+(const Vector4& other) const { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vector4 operator-(const Vector4& other) const { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vector4 operator*(const Vector4& other) const { return Vector4(x * other.x, y * other.y, z * other.z, w * other.w); }
    Vector4 operator/(const Vector4& other) const { return Vector4(x / other.x, y / other.y, z / other.z, w / other.w); }
    Vector4 operator*(float scalar) const { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
    Vector4 operator/(float scalar) const { return Vector4(x / scalar, y / scalar, z / scalar, w / scalar); }
    Vector4& operator+=(const Vector4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    Vector4& operator-=(const Vector4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
    Vector4& operator*=(const Vector4& other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this; }
    Vector4& operator/=(const Vector4& other) { x /= other.x; y /= other.y; z /= other.z; w /= other.w; return *this; }
    Vector4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    Vector4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    void zero() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }
    void one() { x = 1.0f; y = 1.0f; z = 1.0f; w = 1.0f; }

    float dot(const Vector4& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }

    static Vector4 Zero() { return Vector4(0.0f, 0.0f, 0.0f, 0.0f); }
    static Vector4 One() { return Vector4(1.0f, 1.0f, 1.0f, 1.0f); }
};

// --------------------------------------------------------
// Row-Major 4x4行列
// メモリレイアウト: m[row][col]
// ベクトル変換: v * M (行ベクトル × 行列)
// --------------------------------------------------------
struct Matrix
{
    float m[4][4];

    Matrix() { identity(); }
    Matrix(int flag)
    {
        if (flag == 0) zero();
        else identity();
    }
    Matrix(float elements[4][4]) { std::memcpy(m, elements, sizeof(m)); }
    ~Matrix() = default;

    Matrix& operator=(const Matrix& other)
    {
        if (this != &other) std::memcpy(m, other.m, sizeof(m));
        return *this;
    }
    Matrix& operator+=(const Matrix& other)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] += other.m[i][j];
        return *this;
    }
    Matrix& operator-=(const Matrix& other)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] -= other.m[i][j];
        return *this;
    }
    Matrix& operator*=(const Matrix& other)
    {
        Matrix result = (*this) * other;
        std::memcpy(m, result.m, sizeof(m));
        return *this;
    }
    Matrix operator+(const Matrix& other) const
    {
        Matrix result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = m[i][j] + other.m[i][j];
        return result;
    }
    Matrix operator-(const Matrix& other) const
    {
        Matrix result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = m[i][j] - other.m[i][j];
        return result;
    }
    Matrix operator*(const Matrix& other) const
    {
        Matrix result(0);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result.m[i][j] += m[i][k] * other.m[k][j];
        return result;
    }

    void zero() { std::memset(m, 0, sizeof(m)); }
    void identity()
    {
        std::memset(m, 0, sizeof(m));
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }

    void multiply(const Matrix& other)
    {
        float result[4][4] = { 0 };
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result[i][j] += m[i][k] * other.m[k][j];
        std::memcpy(m, result, sizeof(m));
    }

    static Matrix Multiply(const Matrix& a, const Matrix& b)
    {
        Matrix result(0);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result.m[i][j] += a.m[i][k] * b.m[k][j];
        return result;
    }

    void transpose()
    {
        Matrix temp;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                temp.m[i][j] = m[j][i];
        std::memcpy(m, temp.m, sizeof(m));
    }

    static Matrix Transpose(const Matrix& mat)
    {
        Matrix result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = mat.m[j][i];
        return result;
    }

    void inverse();
    static bool Inverse(const Matrix& src, Matrix& dst);

    void setElement(int row, int col, float value)
    {
        if (row >= 0 && row < 4 && col >= 0 && col < 4) m[row][col] = value;
    }

    // Row-Major: 移動成分は m[3][0], m[3][1], m[3][2]
    void setPosition(float x, float y, float z) { m[3][0] = x; m[3][1] = y; m[3][2] = z; }
    void addPosition(float dx, float dy, float dz)
    {
        Matrix trans;
        trans.setPosition(dx, dy, dz);
        multiply(trans);
    }

    void setScale(float sx, float sy, float sz) { m[0][0] = sx; m[1][1] = sy; m[2][2] = sz; }
    void addScale(float sx, float sy, float sz)
    {
        Matrix scale;
        scale.setScale(sx, sy, sz);
        multiply(scale);
    }

    void setRotationYawPitchRoll(float yaw, float pitch, float roll);
    void addRotationYawPitchRoll(float yaw, float pitch, float roll)
    {
        Matrix rot;
        rot.setRotationYawPitchRoll(yaw, pitch, roll);
        multiply(rot);
    }

    void setQuaternion(float x, float y, float z, float w)
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        // Row-Major配置
        m[0][0] = 1.0f - 2.0f * (yy + zz); m[0][1] = 2.0f * (xy + wz);        m[0][2] = 2.0f * (xz - wy);        m[0][3] = 0.0f;
        m[1][0] = 2.0f * (xy - wz);        m[1][1] = 1.0f - 2.0f * (xx + zz); m[1][2] = 2.0f * (yz + wx);        m[1][3] = 0.0f;
        m[2][0] = 2.0f * (xz + wy);        m[2][1] = 2.0f * (yz - wx);        m[2][2] = 1.0f - 2.0f * (xx + yy); m[2][3] = 0.0f;
        m[3][0] = 0.0f;                    m[3][1] = 0.0f;                    m[3][2] = 0.0f;                    m[3][3] = 1.0f;
    }
    void addQuaternion(float x, float y, float z, float w)
    {
        Matrix rot;
        rot.setQuaternion(x, y, z, w);
        multiply(rot);
    }

    // Row-Major: 各行が基底ベクトル、第4列が移動成分
    Vector3 getPosition() const;
    Vector3 getScale() const;
    Vector3 getRight() const;
    Vector3 getUp() const;
    Vector3 getForward() const;
    Quaternion getQuaternion() const;
    Transform toTransform() const;

    static Matrix LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up);

    static Matrix PerspectiveFovLH(float fovY, float aspect, float zn, float zf)
    {
        float yScale = 1.0f / tanf(fovY / 2.0f);
        float xScale = yScale / aspect;
        Matrix proj(0);

        proj.m[0][0] = xScale;
        proj.m[1][1] = yScale;
        proj.m[2][2] = zf / (zf - zn);
        proj.m[2][3] = 1.0f;              // w成分の係数
        proj.m[3][2] = -zn * zf / (zf - zn); // zオフセット
        proj.m[3][3] = 0.0f;
        return proj;
    }

    static Matrix Orthographic(float width, float height, float zn, float zf)
    {
        Matrix ortho(0);
        ortho.m[0][0] = 2.0f / width;
        ortho.m[1][1] = 2.0f / height;
        ortho.m[2][2] = 1.0f / (zf - zn);

        // Row-Major: 移動成分は第4行
        ortho.m[3][0] = 0.0f;
        ortho.m[3][1] = 0.0f;
        ortho.m[3][2] = -zn / (zf - zn);
        ortho.m[3][3] = 1.0f;
        return ortho;
    }

    static Matrix OrthographicOffCenter(float left, float right, float bottom, float top, float zn, float zf)
    {
        Matrix ortho(0);
        ortho.m[0][0] = 2.0f / (right - left);
        ortho.m[1][1] = 2.0f / (top - bottom);
        ortho.m[2][2] = 1.0f / (zf - zn);

        // Row-Major: 移動成分は第4行
        ortho.m[3][0] = -(right + left) / (right - left);
        ortho.m[3][1] = -(top + bottom) / (top - bottom);
        ortho.m[3][2] = -zn / (zf - zn);
        ortho.m[3][3] = 1.0f;
        return ortho;
    }
};

struct Quaternion
{
    float x, y, z, w;
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    ~Quaternion() = default;

    Quaternion operator+(const Quaternion& other) const { return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w); }
    Quaternion operator-(const Quaternion& other) const { return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w); }
    Quaternion operator*(const Quaternion& other) const
    {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }
    Quaternion operator/(const Quaternion& other) const
    {
        float denom = other.x * other.x + other.y * other.y + other.z * other.z + other.w * other.w;
        if (denom == 0.0f) return Quaternion();
        float invDenom = 1.0f / denom;
        return Quaternion(
            (x * other.w + w * other.x + y * other.z - z * other.y) * invDenom,
            (y * other.w + w * other.y + z * other.x - x * other.z) * invDenom,
            (z * other.w + w * other.z + x * other.y - y * other.x) * invDenom,
            (w * other.w - x * other.x - y * other.y - z * other.z) * invDenom
        );
    }
    Quaternion operator*(float scalar) const { return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar); }
    Quaternion operator/(float scalar) const { return Quaternion(x / scalar, y / scalar, z / scalar, w / scalar); }

    // クォータニオンによるベクトル回転: q * v
    Vector3 operator*(const Vector3& v) const
    {
        Vector3 qvec(x, y, z);
        Vector3 t = qvec.cross(v) * 2.0f;
        return v + t * w + qvec.cross(t);
    }

    Quaternion& operator+=(const Quaternion& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    Quaternion& operator-=(const Quaternion& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
    Quaternion& operator*=(const Quaternion& other) { *this = *this * other; return *this; }
    Quaternion& operator/=(const Quaternion& other) { *this = *this / other; return *this; }
    Quaternion& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    Quaternion& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    void zero() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }
    void identity() { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; }

    static Quaternion Zero() { return Quaternion(0.0f, 0.0f, 0.0f, 0.0f); }
    static Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

    static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t)
    {
        return Quaternion(
            a.x * (1 - t) + b.x * t,
            a.y * (1 - t) + b.y * t,
            a.z * (1 - t) + b.z * t,
            a.w * (1 - t) + b.w * t
        );
    }

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
    {
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        Quaternion target = b;

        if (dot < 0.0f) {
            target.x = -b.x;
            target.y = -b.y;
            target.z = -b.z;
            target.w = -b.w;
            dot = -dot;
        }

        const float DOT_THRESHOLD = 0.9995f;
        if (dot > DOT_THRESHOLD) {
            Quaternion r = Quaternion(
                a.x + (target.x - a.x) * t,
                a.y + (target.y - a.y) * t,
                a.z + (target.z - a.z) * t,
                a.w + (target.w - a.w) * t
            );
            r.normalize();
            return r;
        }

        float theta_0 = acosf(std::clamp(dot, -1.0f, 1.0f));
        float theta = theta_0 * t;
        float sin_theta_0 = sinf(theta_0);
        float w0 = sinf((1.0f - t) * theta_0) / sin_theta_0;
        float w1 = sinf(t * theta_0) / sin_theta_0;

        return Quaternion(
            a.x * w0 + target.x * w1,
            a.y * w0 + target.y * w1,
            a.z * w0 + target.z * w1,
            a.w * w0 + target.w * w1
        );
    }

    void normalize()
    {
        float len = sqrtf(x * x + y * y + z * z + w * w);
        if (len != 0.0f) { x /= len; y /= len; z /= len; w /= len; }
    }

    void conjugate() { x = -x; y = -y; z = -z; }

    void inverse()
    {
        float denom = x * x + y * y + z * z + w * w;
        if (denom != 0.0f)
        {
            float invDenom = 1.0f / denom;
            x = -x * invDenom;
            y = -y * invDenom;
            z = -z * invDenom;
            w = w * invDenom;
        }
    }

    void setYawPitchRoll(float yaw, float pitch, float roll)
    {
        float cy = cosf(yaw * 0.5f);
        float sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f);
        float sp = sinf(pitch * 0.5f);
        float cr = cosf(roll * 0.5f);
        float sr = sinf(roll * 0.5f);

        x = sp * cy * cr + cp * sy * sr;
        y = cp * sy * cr - sp * cy * sr;
        z = cp * cy * sr + sp * sy * cr;
        w = cp * cy * cr - sp * sy * sr;
    }

    static Quaternion RotationYawPitchRoll(float yaw, float pitch, float roll)
    {
        Quaternion q;
        q.setYawPitchRoll(yaw, pitch, roll);
        return q;
    }

    void toAxisAngle(Vector3& axis, float& angle) const
    {
        Quaternion q = *this;
        if (q.w > 1.0f) q.normalize();

        angle = 2.0f * acosf(q.w);
        float s = sqrtf(1.0f - q.w * q.w);

        if (s < 0.001f) {
            axis.x = q.x;
            axis.y = q.y;
            axis.z = q.z;
        }
        else {
            axis.x = q.x / s;
            axis.y = q.y / s;
            axis.z = q.z / s;
        }
    }

    void fromAxisAngle(const Vector3& axis, float angle)
    {
        float halfAngle = angle / 2.0f;
        float s = sinf(halfAngle);
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = cosf(halfAngle);
    }

    Matrix toMatrix() const;
};

struct Transform
{
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;

    Transform() : position(), rotation(), scale(1.0f, 1.0f, 1.0f) {}
    Transform(const Vector3& pos, const Quaternion& rot, const Vector3& scl)
        : position(pos), rotation(rot), scale(scl) {
    }
    ~Transform() = default;

    Transform operator*(const Transform& other) const
    {
        Transform result;

        // スケールの合成
        result.scale = scale * other.scale;

        // 回転の合成
        result.rotation = rotation * other.rotation;

        // 位置の合成
        Vector3 scaledChildPos = other.position * scale;
        Vector3 rotatedChildPos = rotation * scaledChildPos;
        result.position = position + rotatedChildPos;

        return result;
    }

    Transform operator/(const Transform& other) const
    {
        return Transform(
            Vector3(
                position.x / other.scale.x,
                position.y / other.scale.y,
                position.z / other.scale.z
            ) - other.position,
            rotation / other.rotation,
            Vector3(
                scale.x / other.scale.x,
                scale.y / other.scale.y,
                scale.z / other.scale.z
            )
        );
    }

    Transform operator*(float scalar) const
    {
        return Transform(position * scalar, rotation * scalar, scale * scalar);
    }

    Transform operator/(float scalar) const
    {
        return Transform(position / scalar, rotation / scalar, scale / scalar);
    }

    Transform& operator*=(const Transform& other)
    {
        *this = *this * other;
        return *this;
    }

    Transform& operator/=(const Transform& other)
    {
        *this = *this / other;
        return *this;
    }

    void zero()
    {
        position.zero();
        rotation.zero();
        scale.zero();
    }
    void identity()
    {
        position.zero();
        rotation.identity();
        scale.one();
    }

    Vector3 getForward() const
    {
        return rotation * Vector3(0.0f, 0.0f, 1.0f);
    }
    Vector3 getRight() const
    {
        return rotation * Vector3(1.0f, 0.0f, 0.0f);
    }
    Vector3 getUp() const
    {
        return rotation * Vector3(0.0f, 1.0f, 0.0f);
    }

    Matrix toMatrix() const;

    static Transform Zero()
    {
        return Transform(Vector3::Zero(), Quaternion::Zero(), Vector3::Zero());
    }
    static Transform Identity()
    {
        return Transform(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
    }

    static Transform Lerp(const Transform& a, const Transform& b, float t)
    {
        Transform result;
        result.position = Vector3::Lerp(a.position, b.position, t);
        result.scale = Vector3::Lerp(a.scale, b.scale, t);
        result.rotation = Quaternion::Lerp(a.rotation, b.rotation, t);
        return result;
    }

    static Transform Slerp(const Transform& a, const Transform& b, float t)
    {
        Transform result;
        result.position = Vector3::Lerp(a.position, b.position, t);
        result.scale = Vector3::Lerp(a.scale, b.scale, t);
        result.rotation = Quaternion::Slerp(a.rotation, b.rotation, t);
        return result;
    }
};

struct Color
{
    float r, g, b, a;

    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

    Color operator*(float scalar) const
    {
        return Color(r * scalar, g * scalar, b * scalar, a * scalar);
    }

    Color operator/(float scalar) const
    {
        return Color(r / scalar, g / scalar, b / scalar, a / scalar);
    }

    Color& operator*=(const float& other)
    {
        *this = *this * other;
        return *this;
    }

    Color& operator/=(const float& other)
    {
        *this = *this / other;
        return *this;
    }

    unsigned long toARGB() const
    {
        unsigned char A = static_cast<unsigned char>(a * 255.0f);
        unsigned char R = static_cast<unsigned char>(r * 255.0f);
        unsigned char G = static_cast<unsigned char>(g * 255.0f);
        unsigned char B = static_cast<unsigned char>(b * 255.0f);
        return (A << 24) | (R << 16) | (G << 8) | B;
    }

    static Color FromARGB(unsigned long argb)
    {
        unsigned char A = (argb >> 24) & 0xFF;
        unsigned char R = (argb >> 16) & 0xFF;
        unsigned char G = (argb >> 8) & 0xFF;
        unsigned char B = argb & 0xFF;
        return Color(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
    }

    static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
};

// --------------------------------------------------------
// 3. 遅延実装
// --------------------------------------------------------

// Row-Major: v * M (行ベクトル × 行列)
inline void Vector3::transform(const Matrix& mat)
{
    float tx = x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + mat.m[3][0];
    float ty = x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + mat.m[3][1];
    float tz = x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + mat.m[3][2];
    x = tx; y = ty; z = tz;
}

inline void Vector3::transformNormal(const Matrix& mat)
{
    float tx = x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0];
    float ty = x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1];
    float tz = x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2];
    x = tx; y = ty; z = tz;
}

inline void Vector3::transformCoord(const Matrix& mat)
{
    float tx = x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + mat.m[3][0];
    float ty = x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + mat.m[3][1];
    float tz = x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + mat.m[3][2];
    float tw = x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + mat.m[3][3];
    if (tw != 0.0f) { x = tx / tw; y = ty / tw; z = tz / tw; }
}

inline void Matrix::inverse()
{
    Matrix inv;
    if (Inverse(*this, inv))
    {
        std::memcpy(m, inv.m, sizeof(m));
    }
    else
    {
        identity();
    }
}

inline bool Matrix::Inverse(const Matrix& src, Matrix& dst)
{
    float m00 = src.m[0][0], m01 = src.m[0][1], m02 = src.m[0][2], m03 = src.m[0][3];
    float m10 = src.m[1][0], m11 = src.m[1][1], m12 = src.m[1][2], m13 = src.m[1][3];
    float m20 = src.m[2][0], m21 = src.m[2][1], m22 = src.m[2][2], m23 = src.m[2][3];
    float m30 = src.m[3][0], m31 = src.m[3][1], m32 = src.m[3][2], m33 = src.m[3][3];

    float b00 = m00 * m11 - m01 * m10;
    float b01 = m00 * m12 - m02 * m10;
    float b02 = m00 * m13 - m03 * m10;
    float b03 = m01 * m12 - m02 * m11;
    float b04 = m01 * m13 - m03 * m11;
    float b05 = m02 * m13 - m03 * m12;
    float b06 = m20 * m31 - m21 * m30;
    float b07 = m20 * m32 - m22 * m30;
    float b08 = m20 * m33 - m23 * m30;
    float b09 = m21 * m32 - m22 * m31;
    float b10 = m21 * m33 - m23 * m31;
    float b11 = m22 * m33 - m23 * m32;

    float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

    if (std::abs(det) < 1e-6f) return false;

    float invDet = 1.0f / det;

    dst.m[0][0] = (m11 * b11 - m12 * b10 + m13 * b09) * invDet;
    dst.m[0][1] = (-m01 * b11 + m02 * b10 - m03 * b09) * invDet;
    dst.m[0][2] = (m31 * b05 - m32 * b04 + m33 * b03) * invDet;
    dst.m[0][3] = (-m21 * b05 + m22 * b04 - m23 * b03) * invDet;

    dst.m[1][0] = (-m10 * b11 + m12 * b08 - m13 * b07) * invDet;
    dst.m[1][1] = (m00 * b11 - m02 * b08 + m03 * b07) * invDet;
    dst.m[1][2] = (-m30 * b05 + m32 * b02 - m33 * b01) * invDet;
    dst.m[1][3] = (m20 * b05 - m22 * b02 + m23 * b01) * invDet;

    dst.m[2][0] = (m10 * b10 - m11 * b08 + m13 * b06) * invDet;
    dst.m[2][1] = (-m00 * b10 + m01 * b08 - m03 * b06) * invDet;
    dst.m[2][2] = (m30 * b04 - m31 * b02 + m33 * b00) * invDet;
    dst.m[2][3] = (-m20 * b04 + m21 * b02 - m23 * b00) * invDet;

    dst.m[3][0] = (-m10 * b09 + m11 * b07 - m12 * b06) * invDet;
    dst.m[3][1] = (m00 * b09 - m01 * b07 + m02 * b06) * invDet;
    dst.m[3][2] = (-m30 * b03 + m31 * b01 - m32 * b00) * invDet;
    dst.m[3][3] = (m20 * b03 - m21 * b01 + m22 * b00) * invDet;

    return true;
}

inline void Matrix::setRotationYawPitchRoll(float yaw, float pitch, float roll)
{
    float cy = cosf(yaw), sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cr = cosf(roll), sr = sinf(roll);

    // Row-Major配置
    m[0][0] = cy * cr + sy * sp * sr;
    m[0][1] = sr * cp;
    m[0][2] = -sy * cr + cy * sp * sr;
    m[0][3] = 0.0f;

    m[1][0] = -cy * sr + sy * sp * cr;
    m[1][1] = cr * cp;
    m[1][2] = sr * sy + cy * sp * cr;
    m[1][3] = 0.0f;

    m[2][0] = sy * cp;
    m[2][1] = -sp;
    m[2][2] = cy * cp;
    m[2][3] = 0.0f;

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

// Row-Major: 移動成分は第4行
inline Vector3 Matrix::getPosition() const
{
    return Vector3(m[3][0], m[3][1], m[3][2]);
}

// Row-Major: 各行ベクトルの長さ
inline Vector3 Matrix::getScale() const
{
    Vector3 right(m[0][0], m[0][1], m[0][2]);
    Vector3 up(m[1][0], m[1][1], m[1][2]);
    Vector3 forward(m[2][0], m[2][1], m[2][2]);
    return Vector3(right.length(), up.length(), forward.length());
}

// Row-Major: 基底ベクトルは各行
inline Vector3 Matrix::getRight() const
{
    return Vector3(m[0][0], m[0][1], m[0][2]);
}

inline Vector3 Matrix::getUp() const
{
    return Vector3(m[1][0], m[1][1], m[1][2]);
}

inline Vector3 Matrix::getForward() const
{
    return Vector3(m[2][0], m[2][1], m[2][2]);
}

inline Quaternion Matrix::getQuaternion() const
{
    // 1. まず各軸のスケール（ベクトルの長さ）を取得して正規化する準備
    Vector3 scale = getScale();

    // ゼロ除算対策
    if (scale.x < 0.0001f || scale.y < 0.0001f || scale.z < 0.0001f)
    {
        return Quaternion(); // スケールが0なら回転なしとする
    }

    // 2. 正規化された回転行列成分を作成
    // Row-Major: m[0]がX軸, m[1]がY軸, m[2]がZ軸
    float r00 = m[0][0] / scale.x; float r01 = m[0][1] / scale.x; float r02 = m[0][2] / scale.x;
    float r10 = m[1][0] / scale.y; float r11 = m[1][1] / scale.y; float r12 = m[1][2] / scale.y;
    float r20 = m[2][0] / scale.z; float r21 = m[2][1] / scale.z; float r22 = m[2][2] / scale.z;

    // 3. 正規化された成分からクォータニオンを抽出
    Quaternion q;
    float trace = r00 + r11 + r22;

    if (trace > 0.0f)
    {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (r12 - r21) * s;
        q.y = (r20 - r02) * s;
        q.z = (r01 - r10) * s;
    }
    else
    {
        if (r00 > r11 && r00 > r22)
        {
            float s = 2.0f * sqrtf(1.0f + r00 - r11 - r22);
            q.w = (r12 - r21) / s;
            q.x = 0.25f * s;
            q.y = (r01 + r10) / s;
            q.z = (r02 + r20) / s;
        }
        else if (r11 > r22)
        {
            float s = 2.0f * sqrtf(1.0f + r11 - r00 - r22);
            q.w = (r20 - r02) / s;
            q.x = (r01 + r10) / s;
            q.y = 0.25f * s;
            q.z = (r12 + r21) / s;
        }
        else
        {
            float s = 2.0f * sqrtf(1.0f + r22 - r00 - r11);
            q.w = (r01 - r10) / s;
            q.x = (r02 + r20) / s;
            q.y = (r12 + r21) / s;
            q.z = 0.25f * s;
        }
    }
    q.normalize();
    return q;
}

inline Transform Matrix::toTransform() const
{
    Transform t;
    t.position = getPosition();
    t.scale = getScale();
    t.rotation = getQuaternion();
    return t;
}

// Row-Major LookAtLH
inline Matrix Matrix::LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Vector3 zaxis = target - eye;
    zaxis.normalize();
    Vector3 xaxis = up.cross(zaxis);
    xaxis.normalize();
    Vector3 yaxis = zaxis.cross(xaxis);

    Matrix view(0);

    // Row-Major: 基底ベクトルを各行に配置
    view.m[0][0] = xaxis.x; view.m[1][0] = xaxis.y; view.m[2][0] = xaxis.z;
    view.m[0][1] = yaxis.x; view.m[1][1] = yaxis.y; view.m[2][1] = yaxis.z;
    view.m[0][2] = zaxis.x; view.m[1][2] = zaxis.y; view.m[2][2] = zaxis.z;

    // Row-Major: 移動成分は第4行
    view.m[3][0] = -xaxis.dot(eye);
    view.m[3][1] = -yaxis.dot(eye);
    view.m[3][2] = -zaxis.dot(eye);
    view.m[3][3] = 1.0f;

    return view;
}

inline Matrix Quaternion::toMatrix() const
{
    Matrix m;
    m.setQuaternion(x, y, z, w);
    return m;
}

inline Matrix Transform::toMatrix() const
{
    // SRT順序: Scale → Rotation → Translation
    Matrix scaleMatrix;
    scaleMatrix.identity();
    scaleMatrix.setScale(scale.x, scale.y, scale.z);

    Matrix rotMatrix = rotation.toMatrix();
    Matrix transMatrix;
    transMatrix.identity();
    transMatrix.setPosition(position.x, position.y, position.z);

    // Row-Major: 右から左に適用 (Scale * Rotation * Translation)
    return scaleMatrix * rotMatrix * transMatrix;
}
