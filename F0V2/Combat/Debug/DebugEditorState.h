#pragma once
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Combat
{
    struct CourseNode
    {
        Float3 position;
    };

    struct DebugEditorState
    {
        Array<Float3> nodeList{};
    };

    inline DebugEditorState g_debugEditorState{};
}
