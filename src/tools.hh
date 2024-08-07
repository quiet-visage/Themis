#pragma once
#include <raylib.h>

#include <cassert>
#include <cstddef>

template <typename T>
struct Vec2 {
    T x;
    T y;

    operator Vector2() { return {(float)x, (float)y}; }

    inline float& operator[](size_t idx) {
        assert(idx <= 2);
        return ((T[2])this)[idx];
    }

    inline Vec2<T> operator*(Vec2<T> e) { return {x * e.x, y * e.y}; }
};

template <typename T>
inline Vec2<T> operator*(T e, Vec2<T> t) {
    return {t.x * e, t.y * e};
}

template <typename T>
inline Vec2<T> operator*(Vec2<T> t, T e) {
    return {t.x * e, t.y * e};
}

template <typename T>
inline Vec2<T> operator+(Vec2<T> t, T e) {
    return {t.x + e, t.y + e};
}

template <typename T>
inline Vec2<T> operator+(Vec2<T> t, Vec2<T> e) {
    return {t.x + e.x, t.y + e.y};
}

template <typename T>
inline Vec2<T> operator+(T e, Vec2<T> t) {
    return {t.x + e, t.y + e};
}
