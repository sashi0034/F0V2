#pragma once
#include "CB/Lambert.h"
#include "Race/Common/CourseData.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Vector3D.h"
#include "TY/InlineComponent.h"

namespace Editor
{
    struct DebugEditorState : IInlineComponent
    {
        ConstantBufferWrapper<Lambert_b10> lambert{};

        Race::CourseData course{};
    };

    inline InlineComponent<DebugEditorState> g_editorState{};
}
