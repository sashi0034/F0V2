#pragma once
#include "TY/Array.h"

namespace RaceSetup
{
    struct CourseFileInfo
    {
        std::string filepath;
        std::u32string displayName;
        std::u32string description1;
        std::u32string description2;
        int difficulty;
    };

    const Array<CourseFileInfo>& GetAllCourseFileInfos();
}
