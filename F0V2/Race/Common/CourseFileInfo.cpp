#include "pch.h"
#include "CourseFileInfo.h"

#include "Asset.generated.h"

namespace Race
{
    const Array<CourseFileInfo>& GetAllCourseFileInfos()
    {
        static Array<CourseFileInfo> s_infos{};
        if (not s_infos.empty())
        {
            return s_infos;
        }

        // -----------------------------------------------

        s_infos.push_back({
            .filepath = "asset/course/course_smoothRoad.toml",
            .displayName = U"始まりの小惑星",
            .description1 = U"広大な平原と緩やかな丘陵が広がり、初心者に最適なコースです。",
            .description2 = U"銀河連邦の広報ホログラムでも「安全・安心」と謳われています。",
            .difficulty = 1,
            .music = Asset_music::GURUGURU_loop,
            .musicLoopRanges = {
                AudioLoopRange{10.000f, 81.250f},
                AudioLoopRange{40.000, 111.250f},
                AudioLoopRange{126.100f, 183.700f},
            },
        });

        s_infos.push_back({
            .filepath = "asset/course/course_twistedRoad.toml",
            .displayName = U"惑星ナピルサキアリガ",
            .description1 = U"重力や磁場が乱れた危険な惑星であり、高度なテクニックが必要です。",
            .description2 = U"ヘリウム3の採掘所としても知られ、近年は恒星間企業勢力の利権争いが激化しています。",
            .difficulty = 5,
            .music = Asset_music::MetropolitanBreeze_loop,
            .musicLoopRanges = {
                AudioLoopRange{2.500f, 40.000f},
                AudioLoopRange{40.000f, 60.000f},
                AudioLoopRange{84.700f, 102.800f},
            },
        });

        return s_infos;
    }

    const CourseFileInfo& GetCourseFileInfoByPath(const std::string& path)
    {
        static CourseFileInfo s_default{};
        static std::unordered_map<std::string, CourseFileInfo> s_table{};
        if (s_table.empty())
        {
            for (const auto& info : GetAllCourseFileInfos())
            {
                s_table[info.filepath] = info;
            }
        }

        const auto it = s_table.find(path);
        return it != s_table.end() ? it->second : s_default;
    }
}
