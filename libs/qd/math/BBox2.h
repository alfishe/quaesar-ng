#pragma once
#include "Point2.h"
#include <cfloat>


namespace qd {


inline float fsel(float a, float b, float c) { return (a >= 0.0f) ? b : c; }


class BBox2
{
public:
    Point2 lim[2];
    BBox2() { setEmpty(); }
    BBox2(const Point2& a, float s) { makebox(a, s); }
    BBox2(const Point2& left_top, const Point2& right_bottom)
    {
        lim[0] = left_top;
        lim[1] = right_bottom;
    }
    BBox2(float left, float top, float right, float bottom)
    {
        lim[0] = Point2(left, top);
        lim[1] = Point2(right, bottom);
    }

    void setEmpty()
    {
        lim[0] = Point2(FLT_MAX / 4, FLT_MAX / 4);
        lim[1] = Point2(FLT_MIN / 4, FLT_MIN / 4);
    }
    bool isEmpty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y; }
    void makebox(const Point2& p, float s)
    {
        Point2 d(s / 2, s / 2);
        lim[0] = p - d;
        lim[1] = p + d;
    }
    Point2 center() const { return (lim[0] + lim[1]) * 0.5; }
    Point2 width() const { return lim[1] - lim[0]; }

    const Point2& operator[](int i) const { return lim[i]; }
    Point2& operator[](int i) { return lim[i]; }
    operator const Point2* () const { return lim; }
    operator Point2* () { return lim; }

    float float_is_empty() const { return fsel(lim[1].x - lim[0].x, 0.0f, 1.0f) + fsel(lim[1].y - lim[0].y, 0.0f, 1.0f); }
    BBox2& operator+=(const Point2& p)
    {
        lim[0].x = fsel(lim[0].x - p.x, p.x, lim[0].x);
        lim[1].x = fsel(p.x - lim[1].x, p.x, lim[1].x);
        lim[0].y = fsel(lim[0].y - p.y, p.y, lim[0].y);
        lim[1].y = fsel(p.y - lim[1].y, p.y, lim[1].y);
        return *this;
    }
    BBox2& operator+=(const BBox2& b)
    {
        if (b.isEmpty())
            return *this;
        lim[0].x = fsel(lim[0].x - b.lim[0].x, b.lim[0].x, lim[0].x);
        lim[1].x = fsel(b.lim[1].x - lim[1].x, b.lim[1].x, lim[1].x);
        lim[0].y = fsel(lim[0].y - b.lim[0].y, b.lim[0].y, lim[0].y);
        lim[1].y = fsel(b.lim[1].y - lim[1].y, b.lim[1].y, lim[1].y);
        return *this;
    }

    bool operator&(const Point2& p) const
    {
        if (p.x < lim[0].x)
            return 0;
        if (p.x > lim[1].x)
            return 0;
        if (p.y < lim[0].y)
            return 0;
        if (p.y > lim[1].y)
            return 0;
        return 1;
    }
    bool operator&(const BBox2& b) const
    {
        if (b.isEmpty())
            return 0;
        if (b.lim[0].x > lim[1].x)
            return 0;
        if (b.lim[1].x < lim[0].x)
            return 0;
        if (b.lim[0].y > lim[1].y)
            return 0;
        if (b.lim[1].y < lim[0].y)
            return 0;
        return 1;
    }

    void inflate(float val)
    {
        lim[0].x -= val;
        lim[0].y -= val;
        lim[1].x += val;
        lim[1].y += val;
    }

    void scale(float val)
    {
        const Point2 c = center();
        lim[0] = (lim[0] - c) * val + c;
        lim[1] = (lim[1] - c) * val + c;
    }

    float left() const { return lim[0].x; }
    float right() const { return lim[1].x; }
    float top() const { return lim[0].y; }
    float bottom() const { return lim[1].y; }
    const Point2& getMin() const { return lim[0]; }
    const Point2& getMax() const { return lim[1]; }
    Point2 size() const { return lim[1] - lim[0]; }

    const Point2& leftTop() const { return lim[0]; }
    Point2 rightTop() const { return Point2(lim[1].x, lim[0].y); }
    Point2 leftBottom() const { return Point2(lim[0].x, lim[1].y); }
    const Point2& rightBottom() const { return lim[1]; }

    template <class T>
    static BBox2 xz(const T& a)
    {
        return BBox2(Point2::xz(a.lim[0]), Point2::xz(a.lim[1]));
    }
    template <class T>
    static BBox2 yz(const T& a)
    {
        return BBox2(Point2::yz(a.lim[0]), Point2::yz(a.lim[1]));
    }
    template <class T>
    static BBox2 xy(const T& a)
    {
        return BBox2(Point2::xy(a.lim[0]), Point2::xy(a.lim[1]));
    }
}; // class BBox2


}; // namespace qd
