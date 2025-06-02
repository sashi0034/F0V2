#include "pch.h"

#include "Title/Title_Rendering.h"

void Main()
{
#if 1
    Title_Rendering();
#else
    EntryPoint_AS();
#endif
}

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
