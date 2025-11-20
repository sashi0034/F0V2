#pragma once
#include "Race/Common/CourseData.h"
#include "TY/InlineComponent.h"

namespace Editor
{
    struct DebugEditorState : IInlineComponent
    {
        Race::CourseData course{};
    };

    inline InlineComponent<DebugEditorState> g_editorState{};
}
