#ifndef VECTOR2D_H
#define VECTOR2D_H

template <typename T>
struct Vector2D {
    T x;
    T y;

    bool operator==(const Vector2D&) const = default;
};

#endif
