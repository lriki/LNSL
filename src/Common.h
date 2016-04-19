
#pragma once

#include "hlsl2glslfork/include/hlsl2glsl.h"
//#include "hlsl2glslfork/hlslang/GLSLCodeGen/hlslLinker.h"

#include <d3d9.h>
#include <D3DX9Shader.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <LuminoCore.h>
using namespace ln;

#include "../external/Fluorite/src/Parser/DiagnosticsManager.h"
#include "../external/Fluorite/src/Parser/Frontend/Cpp/CppLexer.h"

#define SAFE_RELEASE	LN_SAFE_RELEASE
