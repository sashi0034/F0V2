#pragma once
#include "ResourcePathWrapper.h"

namespace Race
{
    struct CourseFileInfo
    {
        std::string filepath;
        std::u32string displayName;
        std::u32string description1;
        std::u32string description2;
        int difficulty;
        MusicAudioPathWrapper music;
        std::array<AudioLoopRange, 3> musicLoopRanges;
    };

    const Array<CourseFileInfo>& GetAllCourseFileInfos();

    const CourseFileInfo& GetCourseFileInfoByPath(const std::string& path);
}
