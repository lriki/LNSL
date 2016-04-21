
#pragma once
#include "Common.h"
#include "Symbols.h"
#include "Generator.h"

class Generator
{
public:
	void Generate(Effect* effect, const PathName& output);

	static StringA Convert(const StringA& input, const StringA& entryPoint, EShLanguage codeType);

private:
};
