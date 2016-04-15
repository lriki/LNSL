cmake_minimum_required (VERSION 3.0.0)
project (lnsl)

#------------------------------------------------------------------------------
# Files
#------------------------------------------------------------------------------

set(LN_SOURCES_lnsl
	src/DXDevice.cpp
	src/DXDevice.h
	src/Generator.cpp
	src/Generator.h
	src/Loader.cpp
	src/Loader.h
	src/Main.cpp
	src/Symbols.cpp
	src/Symbols.h
)

set(LN_SOURCES_hlsl2glslfork
	src/hlsl2glslfork/hlslang/GLSLCodeGen/typeSamplers.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslCommon.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslCommon.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslFunction.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslFunction.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslOutput.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslOutput.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslStruct.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslStruct.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslSymbol.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/glslSymbol.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslCrossCompiler.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslCrossCompiler.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslLinker.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslLinker.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslSupportLib.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/hlslSupportLib.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/propagateMutable.cpp
	src/hlsl2glslfork/hlslang/GLSLCodeGen/propagateMutable.h
	src/hlsl2glslfork/hlslang/GLSLCodeGen/typeSamplers.cpp
	src/hlsl2glslfork/hlslang/Include/PoolAlloc.h
	src/hlsl2glslfork/hlslang/Include/Types.h
	src/hlsl2glslfork/hlslang/Include/BaseTypes.h
	src/hlsl2glslfork/hlslang/Include/Common.h
	src/hlsl2glslfork/hlslang/Include/InfoSink.h
	src/hlsl2glslfork/hlslang/Include/InitializeGlobals.h
	src/hlsl2glslfork/hlslang/Include/InitializeParseContext.h
	src/hlsl2glslfork/hlslang/Include/intermediate.h
	src/hlsl2glslfork/hlslang/MachineIndependent/SymbolTable.h
	src/hlsl2glslfork/hlslang/MachineIndependent/ConstantFolding.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/HLSL2GLSL.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/hlslang.l
	src/hlsl2glslfork/hlslang/MachineIndependent/hlslang.y
	src/hlsl2glslfork/hlslang/MachineIndependent/InfoSink.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/Initialize.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/Initialize.h
	src/hlsl2glslfork/hlslang/MachineIndependent/Intermediate.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/intermOut.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/IntermTraverse.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/localintermediate.h
	src/hlsl2glslfork/hlslang/MachineIndependent/ParseHelper.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/ParseHelper.h
	src/hlsl2glslfork/hlslang/MachineIndependent/PoolAlloc.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/RemoveTree.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/RemoveTree.h
	src/hlsl2glslfork/hlslang/MachineIndependent/SymbolTable.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/sourceloc.h
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader.h
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader_common.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader_internal.h
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader_lexer.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader_lexer.re
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/mojoshader_preprocessor.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/preprocessor/sourceloc.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/hlslang_tab.h
	src/hlsl2glslfork/hlslang/MachineIndependent/Gen_hlslang.cpp
	src/hlsl2glslfork/hlslang/MachineIndependent/Gen_hlslang_tab.cpp
	src/hlsl2glslfork/hlslang/OSDependent/Windows/unistd.h
	src/hlsl2glslfork/hlslang/OSDependent/Windows/osinclude.h
	src/hlsl2glslfork/hlslang/OSDependent/Windows/ossource.cpp
)

source_group("lnsl"				FILES ${LN_SOURCES_lnsl})
source_group("hlsl2glslfork"	FILES ${LN_SOURCES_hlsl2glslfork})

set(LN_SOURCES
	${LN_SOURCES_lnsl}
	${LN_SOURCES_hlsl2glslfork}
)

set(LN_INCLUDE_DIRECTORIES
	$ENV{DXSDK_DIR}/Include
	${PROJECT_SOURCE_DIR}/src/hlsl2glslfork/include
	${PROJECT_SOURCE_DIR}/src/hlsl2glslfork/hlslang/GLSLCodeGen
	${PROJECT_SOURCE_DIR}/src/hlsl2glslfork/hlslang/Include
	${PROJECT_SOURCE_DIR}/src/hlsl2glslfork/hlslang/MachineIndependent
	${PROJECT_SOURCE_DIR}/src/hlsl2glslfork/hlslang/OSDependent/Windows
	${CMAKE_SOURCE_DIR}/external/Fluorite/external/Lumino.Core/include
	${CMAKE_SOURCE_DIR}/external/Lumino.Math/include
)

#------------------------------------------------------------------------------
# Options
#------------------------------------------------------------------------------
include(external/Fluorite/external/Lumino.Core/cmake/LNCommon.cmake NO_POLICY_SCOPE)

#------------------------------------------------------------------------------
# Output
#------------------------------------------------------------------------------

add_subdirectory(external/Fluorite)
link_libraries(Fluorite_Static)

link_directories($ENV{DXSDK_DIR}Lib/x86)

ln_add_executable(${PROJECT_NAME})

