#include "pch.h"
#include "CourseFileInfo.h"

namespace RaceSetup
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
            .filename = "asset/course/course_smoothRoad.toml",
            .displayName = U"始まりの小惑星",
            .description1 = U"広大な平原と緩やかな丘陵が広がり、初心者に最適なコースです。",
            .description2 = U"銀河連邦の広報ホログラムでも「安全・安心」と謳われています。",
            .difficulty = 1,
        });

        s_infos.push_back({
            .filename = "asset/course/course_twistedRoad.toml",
            .displayName = U"惑星ナピルサキアリガ",
            .description1 = U"重力や磁場が乱れた危険な惑星であり、高度なテクニックが必要です。",
            .description2 = U"ヘリウム3の採掘所としても知られ、近年は恒星間企業勢力の利権争いが激化しています。",
            .difficulty = 5,
        });

        return s_infos;
    }
}
