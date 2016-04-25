
#pragma once
#include "Common.h"
#include "Symbols.h"
#include "Generator.h"

class Generator
{
public:
	void Generate(Effect* effect, const PathName& output);

	StringA Convert(const StringA& input, const StringA& entryPoint, EShLanguage codeType);

	static StringA Hlsl2Glsl(const StringA& input, const StringA& entryPoint, EShLanguage codeType);

private:
	Effect*	m_effect;
};
