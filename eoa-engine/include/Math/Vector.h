#pragma once

#include "Core/Types.h"
#include <cmath>

namespace EOA {

template<typename T>
struct Vec2 {
    T X, Y;
    
    Vec2() : X(0), Y(0) {}
    Vec2(T x, T y) : X(x), Y(y) {}
    
    Vec2 operator+(const Vec2& other) const { return Vec2(X + other.X, Y + other.Y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(X - other.X, Y - other.Y); }
    Vec2 operator*(T s) const { return Vec2(X * s, Y * s); }
    Vec2 operator/(T s) const { return Vec2(X / s, Y / s); }
    
    T Length() const { return std::sqrt(X * X + Y * Y); }
    T LengthSquared() const { return X * X + Y * Y; }
    
    Vec2 Normalize() const {
        T len = Length();
        return len > 0 ? *this / len : Vec2();
    }
};

template<typename T>
struct Vec3 {
    T X, Y, Z;
    
    Vec3() : X(0), Y(0), Z(0) {}
    Vec3(T x, T y, T z) : X(x), Y(y), Z(z) {}
    
    Vec3 operator+(const Vec3& other) const { return Vec3(X + other.X, Y + other.Y, Z + other.Z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(X - other.X, Y - other.Y, Z - other.Z); }
    Vec3 operator*(T s) const { return Vec3(X * s, Y * s, Z * s); }
    Vec3 operator/(T s) const { return Vec3(X / s, Y / s, Z / s); }
    Vec3 operator-() const { return Vec3(-X, -Y, -Z); }
    
    T Dot(const Vec3& other) const { return X * other.X + Y * other.Y + Z * other.Z; }
    
    Vec3 Cross(const Vec3& other) const {
        return Vec3(
            Y * other.Z - Z * other.Y,
            Z * other.X - X * other.Z,
            X * other.Y - Y * other.X
        );
    }
    
    T Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    T LengthSquared() const { return X * X + Y * Y + Z * Z; }
    
    Vec3 Normalize() const {
        T len = Length();
        return len > 0 ? *this / len : Vec3();
    }
};

template<typename T>
struct Vec4 {
    T X, Y, Z, W;
    
    Vec4() : X(0), Y(0), Z(0), W(0) {}
    Vec4(T x, T y, T z, T w) : X(x), Y(y), Z(z), W(w) {}
    Vec4(const Vec3<T>& v, T w) : X(v.X), Y(v.Y), Z(v.Z), W(w) {}
    
    Vec4 operator+(const Vec4& other) const { return Vec4(X + other.X, Y + other.Y, Z + other.Z, W + other.W); }
    Vec4 operator-(const Vec4& other) const { return Vec4(X - other.X, Y - other.Y, Z - other.Z, W - other.W); }
    Vec4 operator*(T s) const { return Vec4(X * s, Y * s, Z * s, W * s); }
    Vec4 operator/(T s) const { return Vec4(X / s, Y / s, Z / s, W / s); }
};

using Vector2 = Vec2<float>;
using Vector3 = Vec3<float>;
using Vector4 = Vec4<float>;
using IntVector2 = Vec2<int>;

} // namespace EOA
