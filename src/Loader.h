
#pragma once
#include "Common.h"
#include "Parsers.h"

class Loader
{
public:
	Loader();
	~Loader();

	void Load(const ln::PathName& hlslFilePath);

	const StringA& GetPreprocessedHLSLCode() const { return m_preprocessedHLSLCode; }

private:
	ID3DXEffect*	m_dxEffect;
	StringA			m_preprocessedHLSLCode;
};
