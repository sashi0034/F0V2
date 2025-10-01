#pragma once
#include "TY/Array.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Vector3D.h"
#include "TY/InlineComponent.h"

namespace Editor
{
    struct Lambert_b10
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct CourseNode
    {
        Float3 pos;
    };

    struct CourseData
    {
        Array<CourseNode> nodes{};
    };

    struct DebugEditorState : IInlineComponent
    {
        ConstantBufferWrapper<Lambert_b10> lambert{};

        CourseData course{};
    };

    inline InlineComponent<DebugEditorState> g_editorState{};
}
