#pragma once

#include "Math/Vector.h"
#include <array>
#include <cmath>

namespace EOA {

struct Matrix4 {
    std::array<float, 16> Data;
    
    Matrix4() {
        Data.fill(0.0f);
        Data[0] = Data[5] = Data[10] = Data[15] = 1.0f; // Identity
    }
    
    float& operator()(int row, int col) { return Data[row * 4 + col]; }
    float operator()(int row, int col) const { return Data[row * 4 + col]; }
    
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                result(r, c) = 
                    (*this)(r, 0) * other(0, c) +
                    (*this)(r, 1) * other(1, c) +
                    (*this)(r, 2) * other(2, c) +
                    (*this)(r, 3) * other(3, c);
            }
        }
        return result;
    }
    
    Vector3 TransformPoint(const Vector3& point) const {
        float w = (*this)(3, 0) * point.X + (*this)(3, 1) * point.Y + (*this)(3, 2) * point.Z + (*this)(3, 3);
        return Vector3(
            ((*this)(0, 0) * point.X + (*this)(0, 1) * point.Y + (*this)(0, 2) * point.Z + (*this)(0, 3)) / w,
            ((*this)(1, 0) * point.X + (*this)(1, 1) * point.Y + (*this)(1, 2) * point.Z + (*this)(1, 3)) / w,
            ((*this)(2, 0) * point.X + (*this)(2, 1) * point.Y + (*this)(2, 2) * point.Z + (*this)(2, 3)) / w
        );
    }
    
    Vector3 TransformVector(const Vector3& vec) const {
        return Vector3(
            (*this)(0, 0) * vec.X + (*this)(0, 1) * vec.Y + (*this)(0, 2) * vec.Z,
            (*this)(1, 0) * vec.X + (*this)(1, 1) * vec.Y + (*this)(1, 2) * vec.Z,
            (*this)(2, 0) * vec.X + (*this)(2, 1) * vec.Y + (*this)(2, 2) * vec.Z
        );
    }
    
    static Matrix4 Identity() {
        return Matrix4();
    }
    
    static Matrix4 Perspective(float fov, float aspect, float nearPlane, float farPlane) {
        Matrix4 result;
        float tanHalfFov = std::tan(fov / 2.0f);
        
        result(0, 0) = 1.0f / (aspect * tanHalfFov);
        result(1, 1) = 1.0f / tanHalfFov;
        result(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result(3, 2) = -1.0f;
        result(3, 3) = 0.0f;
        
        return result;
    }
    
    static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Matrix4 result;
        Vector3 f = (target - eye).Normalize();
        Vector3 s = f.Cross(up).Normalize();
        Vector3 u = s.Cross(f);
        
        result(0, 0) = s.X; result(0, 1) = s.Y; result(0, 2) = s.Z;
        result(1, 0) = u.X; result(1, 1) = u.Y; result(1, 2) = u.Z;
        result(2, 0) = -f.X; result(2, 1) = -f.Y; result(2, 2) = -f.Z;
        result(3, 0) = -s.Dot(eye);
        result(3, 1) = -u.Dot(eye);
        result(3, 2) = f.Dot(eye);
        result(3, 3) = 1.0f;
        
        return result;
    }
    
    static Matrix4 Translation(const Vector3& t) {
        Matrix4 result;
        result(0, 3) = t.X;
        result(1, 3) = t.Y;
        result(2, 3) = t.Z;
        return result;
    }
    
    static Matrix4 Scale(const Vector3& s) {
        Matrix4 result;
        result(0, 0) = s.X;
        result(1, 1) = s.Y;
        result(2, 2) = s.Z;
        return result;
    }
    
    static Matrix4 RotationX(float angle) {
        Matrix4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result(1, 1) = c; result(1, 2) = -s;
        result(2, 1) = s; result(2, 2) = c;
        return result;
    }
    
    static Matrix4 RotationY(float angle) {
        Matrix4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result(0, 0) = c; result(0, 2) = s;
        result(2, 0) = -s; result(2, 2) = c;
        return result;
    }
    
    static Matrix4 RotationZ(float angle) {
        Matrix4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result(0, 0) = c; result(0, 1) = -s;
        result(1, 0) = s; result(1, 1) = c;
        return result;
    }
};

} // namespace EOA
