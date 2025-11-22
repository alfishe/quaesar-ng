#pragma once
#include "qd/base/base.h"
#include <cmath>
#include "Point3.h"


#define INLINE __forceinline

namespace qd {
// TMatrix - Transformation matrix
/**
  4x3 transformation matrix.
  last column (3rd, counting from zero) is translation
  @sa Matrix3 TMatrix4 Point3 Point2 Point4
*/
class TMatrix
{
public:
    union {
        Point3 col[4];
        float m[4][3];
        float array[12];
    };

    /// identity and zero constant matrices
    static const TMatrix IDENT, ZERO;

    INLINE TMatrix() = default;
    INLINE explicit TMatrix(float);

    INLINE void identity();
    INLINE void zero();

    INLINE const float* operator[] (int i) const { return m[i]; }
    INLINE float* operator[] (int i) { return m[i]; }
    INLINE TMatrix operator- () const;
    INLINE TMatrix operator+ () const { return *this; }

    INLINE TMatrix operator* (float) const;

    /// multiply matrices
    INLINE TMatrix operator* (const TMatrix& b) const {
        TMatrix r;

        r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
        r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
        r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
        r.m[3][0] = m[3][0] + m[0][0] * b.m[3][0] + m[1][0] * b.m[3][1] + m[2][0] * b.m[3][2];

        r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
        r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
        r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
        r.m[3][1] = m[3][1] + m[0][1] * b.m[3][0] + m[1][1] * b.m[3][1] + m[2][1] * b.m[3][2];

        r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
        r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
        r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
        r.m[3][2] = m[3][2] + m[0][2] * b.m[3][0] + m[1][2] * b.m[3][1] + m[2][2] * b.m[3][2];
        return r;
    }
    /// multiply only 3x3 parts, translation column is zero
    INLINE TMatrix operator% (const TMatrix& b) const {
        TMatrix r;

        r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
        r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
        r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
        r.m[3][0] = 0.f;

        r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
        r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
        r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
        r.m[3][1] = 0.f;

        r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
        r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
        r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
        r.m[3][2] = 0.f;
        return r;
    }

    /// transform #Point3 point through this matrix
    INLINE Point3 operator* (const Point3& p) const {
        Point3 r;
        r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0];
        r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1];
        r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + m[3][2];
        return r;
    }

    INLINE Point2 operator* (const Point2& p) const {
        Point2 r;
        r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[3][0];
        r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[3][1];
        return r;
    }

    /// transform #Point3 vector through this matrix,
    /// translation is not applied
    INLINE Point3 operator% (const Point3& p) const {
        Point3 r;
        r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2];
        r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2];
        r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2];
        return r;
    }
    INLINE TMatrix operator+ (const TMatrix&) const;
    INLINE TMatrix operator- (const TMatrix&) const;

    INLINE TMatrix& operator+= (const TMatrix&);
    INLINE TMatrix& operator-= (const TMatrix&);
    INLINE TMatrix& operator*= (const TMatrix&);
    INLINE TMatrix& operator*= (float);

    INLINE float det() const;

    INLINE void setcol(int i, const Point3& v) { col[i] = v; }
    INLINE void setcol(int i, float x, float y, float z) {
        m[i][0] = x;
        m[i][1] = y;
        m[i][2] = z;
    }
    INLINE const Point3& getcol(int i) const { return col[i]; }

#define eqtm(i, j) (m[(i)][(j)] == a.m[(i)][(j)])
    INLINE bool operator== (const TMatrix& a) const {
        return (eqtm(0, 0) && eqtm(0, 1) && eqtm(0, 2) && eqtm(1, 0) && eqtm(1, 1) && eqtm(1, 2) && eqtm(2, 0) && eqtm(2, 1) &&
            eqtm(2, 2) && eqtm(3, 0) && eqtm(3, 1) && eqtm(3, 2));
    }
#undef eqtm

#define netm(i, j) (m[(i)][(j)] != a.m[(i)][(j)])
    INLINE bool operator!= (const TMatrix& a) const {
        return (netm(0, 0) || netm(0, 1) || netm(0, 2) || netm(1, 0) || netm(1, 1) || netm(1, 2) || netm(2, 0) || netm(2, 1) ||
            netm(2, 2) || netm(3, 0) || netm(3, 1) || netm(3, 2));
    }
#undef netm
    INLINE void rotxTM(float a) {
        identity();
        if (a == 0)
            return;
        m[1][1] = m[2][2] = cosf(a);
        m[1][2] = -(m[2][1] = sinf(a));
    }

    INLINE void rotyTM(float a) {
        identity();
        if (a == 0)
            return;
        m[2][2] = m[0][0] = cosf(a);
        m[2][0] = -(m[0][2] = sinf(a));
    }

    INLINE void rotzTM(float a) {
        identity();
        if (a == 0)
            return;
        m[0][0] = m[1][1] = cosf(a);
        m[0][1] = -(m[1][0] = sinf(a));
    }
    INLINE void makeTM(const Point3& axis, float ang) {
        float s, c;
        float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

        Point3 a = axis;
        c = length(a);
        if (c == 0) {
            identity();
            return;
        }

        a /= c;
        s = sinf(ang);
        c = cosf(ang);

        xx = a.x * a.x;
        yy = a.y * a.y;
        zz = a.z * a.z;
        xy = a.x * a.y;
        yz = a.y * a.z;
        zx = a.z * a.x;
        xs = a.x * s;
        ys = a.y * s;
        zs = a.z * s;
        one_c = 1 - c;

        m[0][0] = one_c * xx + c;
        m[1][0] = one_c * xy - zs;
        m[2][0] = one_c * zx + ys;

        m[0][1] = one_c * xy + zs;
        m[1][1] = one_c * yy + c;
        m[2][1] = one_c * yz - xs;

        m[0][2] = one_c * zx - ys;
        m[1][2] = one_c * yz + xs;
        m[2][2] = one_c * zz + c;

        m[3][0] = m[3][1] = m[3][2] = 0;
    }

    INLINE float getScalingFactor() const { return cbrt(fabsf(det())); }

    void orthonormalize() // Remove scale.
    {
        setcol(2, normalize(getcol(0) % getcol(1)));
        setcol(1, normalize(getcol(2) % getcol(0)));
        setcol(0, normalize(getcol(1) % getcol(2)));
    }
}; // class TMatrix
//////////////////////////////////////////////////////////////////////////



TMatrix operator* (float, const TMatrix&);
TMatrix inverse(const TMatrix&, float determinant);
TMatrix inverse(const TMatrix&);

INLINE TMatrix orthonormalized_inverse(const TMatrix& a) {
    TMatrix r;
    r.m[0][0] = a.m[0][0];
    r.m[0][1] = a.m[1][0];
    r.m[0][2] = a.m[2][0];
    r.m[1][0] = a.m[0][1];
    r.m[1][1] = a.m[1][1];
    r.m[1][2] = a.m[2][1];
    r.m[2][0] = a.m[0][2];
    r.m[2][1] = a.m[1][2];
    r.m[2][2] = a.m[2][2];
    r.setcol(3, -(r % a.getcol(3)));
    return r;
}


INLINE TMatrix rotxTM(float a) {
    TMatrix m;
    m.identity();
    if (a == 0)
        return m;
    m.m[1][1] = m.m[2][2] = cosf(a);
    m.m[1][2] = -(m.m[2][1] = sinf(a));
    return m;
}

INLINE TMatrix rotyTM(float a) {
    TMatrix m;
    m.identity();
    if (a == 0)
        return m;
    m.m[2][2] = m.m[0][0] = cosf(a);
    m.m[2][0] = -(m.m[0][2] = sinf(a));
    return m;
}

INLINE TMatrix rotzTM(float a) {
    TMatrix m;
    m.identity();
    if (a == 0)
        return m;
    m.m[0][0] = m.m[1][1] = cosf(a);
    m.m[0][1] = -(m.m[1][0] = sinf(a));
    return m;
}

INLINE TMatrix::TMatrix(float a) {
    memset(m, 0, sizeof(m));
    m[0][0] = m[1][1] = m[2][2] = a;
}

INLINE void TMatrix::zero() {
    memset(m, 0, sizeof(m));
}

INLINE void TMatrix::identity() {
    memset(m, 0, sizeof(m));
    m[0][0] = m[1][1] = m[2][2] = 1;
}

INLINE TMatrix TMatrix::operator- () const {
    TMatrix a;
    for (int i = 0; i < 4 * 3; ++i)
        a.array[i] = -array[i];
    return a;
}

INLINE TMatrix TMatrix::operator+ (const TMatrix& b) const {
    TMatrix r;
    for (int i = 0; i < 4 * 3; ++i)
        r.array[i] = array[i] + b.array[i];
    return r;
}

INLINE TMatrix TMatrix::operator- (const TMatrix& b) const {
    TMatrix r;
    for (int i = 0; i < 4 * 3; ++i)
        r.array[i] = array[i] - b.array[i];
    return r;
}

INLINE TMatrix& TMatrix::operator+= (const TMatrix& a) {
    for (int i = 0; i < 4 * 3; ++i)
        array[i] += a.array[i];
    return *this;
}

INLINE TMatrix& TMatrix::operator-= (const TMatrix& a) {
    for (int i = 0; i < 4 * 3; ++i)
        array[i] -= a.array[i];
    return *this;
}

INLINE TMatrix TMatrix::operator* (float f) const {
    TMatrix a;
    for (int i = 0; i < 4 * 3; ++i)
        a.array[i] = array[i] * f;
    return a;
}

INLINE TMatrix operator* (float f, const TMatrix& a) {
    return a * f;
}

INLINE TMatrix& TMatrix::operator*= (float f) {
    for (int i = 0; i < 4 * 3; ++i)
        array[i] *= f;
    return *this;
}


INLINE Point3 operator* (const Point3& p, const TMatrix& m) {
    Point3 r;
    for (int i = 0; i < 3; ++i) {
        r[i] = m.m[3][i];
        for (int j = 0; j < 3; ++j)
            r[i] += p[j] * m.m[j][i];
    }
    return r;
}


INLINE TMatrix& TMatrix::operator*= (const TMatrix& b) {
    TMatrix r;
    r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
    r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
    r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
    r.m[3][0] = m[3][0] + m[0][0] * b.m[3][0] + m[1][0] * b.m[3][1] + m[2][0] * b.m[3][2];

    r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
    r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
    r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
    r.m[3][1] = m[3][1] + m[0][1] * b.m[3][0] + m[1][1] * b.m[3][1] + m[2][1] * b.m[3][2];

    r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
    r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
    r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
    r.m[3][2] = m[3][2] + m[0][2] * b.m[3][0] + m[1][2] * b.m[3][1] + m[2][2] * b.m[3][2];

    float* dst = &m[0][0], * src = &r.m[0][0];
    for (int i = 0; i < 12; i++)
        dst[i] = src[i];

    return *this;
}


INLINE float TMatrix::det() const {
    return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
        m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
}

INLINE TMatrix inverse(const TMatrix& a, float d) {
    TMatrix r;
    //assert(fabsf(d) > 1e-12f);
    float inv_d = 1.0f / d;
    r.m[0][0] = (a.m[1][1] * a.m[2][2] - a.m[2][1] * a.m[1][2]) * inv_d;
    r.m[0][1] = (-a.m[0][1] * a.m[2][2] + a.m[2][1] * a.m[0][2]) * inv_d;
    r.m[0][2] = (a.m[0][1] * a.m[1][2] - a.m[1][1] * a.m[0][2]) * inv_d;
    r.m[1][0] = (-a.m[1][0] * a.m[2][2] + a.m[2][0] * a.m[1][2]) * inv_d;
    r.m[1][1] = (a.m[0][0] * a.m[2][2] - a.m[2][0] * a.m[0][2]) * inv_d;
    r.m[1][2] = (-a.m[0][0] * a.m[1][2] + a.m[1][0] * a.m[0][2]) * inv_d;
    r.m[2][0] = (a.m[1][0] * a.m[2][1] - a.m[2][0] * a.m[1][1]) * inv_d;
    r.m[2][1] = (-a.m[0][0] * a.m[2][1] + a.m[2][0] * a.m[0][1]) * inv_d;
    r.m[2][2] = (a.m[0][0] * a.m[1][1] - a.m[1][0] * a.m[0][1]) * inv_d;

    r.m[3][0] = -r.m[0][0] * a.m[3][0] - r.m[1][0] * a.m[3][1] - r.m[2][0] * a.m[3][2];
    r.m[3][1] = -r.m[0][1] * a.m[3][0] - r.m[1][1] * a.m[3][1] - r.m[2][1] * a.m[3][2];
    r.m[3][2] = -r.m[0][2] * a.m[3][0] - r.m[1][2] * a.m[3][1] - r.m[2][2] * a.m[3][2];
    return r;
}
INLINE TMatrix inverse(const TMatrix& a) {
    return inverse(a, a.det());
}

/// make matrix that rotates around specified axis
INLINE TMatrix makeTM(const Point3& axis, float ang) {
    TMatrix m;
    m.makeTM(axis, ang);
    return m;
}


#undef INLINE
}; // namespace qd
