#pragma once
#include "TY/Array.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Vector3D.h"

namespace Combat
{
    struct Lambert_b10
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct CourseNode
    {
        Float3 position;
    };

    struct DebugEditorState : IInlineComponent
    {
        ConstantBufferWrapper<Lambert_b10> lambert;

        Array<Float3> nodeList{};
    };

    inline InlineComponent<DebugEditorState> g_debugEditorState{};
}
