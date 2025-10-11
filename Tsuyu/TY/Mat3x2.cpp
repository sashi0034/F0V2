#include "pch.h"
#include "Mat3x2.h"

namespace TY
{
    Mat3x2 Mat3x2::inverse() const noexcept
    {
        const float det = determinant();
        assert(det != 0.0f);

        const float detInv = (1.0f / det);

        Mat3x2 result;
        result._11 = (_22 * detInv);
        result._12 = -(_12 * detInv);
        result._21 = -(_21 * detInv);
        result._22 = (_11 * detInv);
        result._31 = (_21 * _32 - _22 * _31) * detInv;
        result._32 = (_12 * _31 - _11 * _32) * detInv;
        return result;
    }

    bool Mat3x2::operator==(const Mat3x2& rhs) const noexcept
    {
        return std::memcmp(this, &rhs, sizeof(Mat3x2)) == 0;
    }

    bool Mat3x2::operator!=(const Mat3x2& rhs) const noexcept
    {
        return !(*this == rhs);
    }
}
