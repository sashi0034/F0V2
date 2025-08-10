#include "pch.h"
#include "Mat3x2.h"

namespace TY
{
    bool Mat3x2::operator==(const Mat3x2& rhs) const noexcept
    {
        return std::memcmp(this, &rhs, sizeof(Mat3x2)) == 0;
    }

    bool Mat3x2::operator!=(const Mat3x2& rhs) const noexcept
    {
        return !(*this == rhs);
    }
}
