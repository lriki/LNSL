
#pragma once
#include "Common.h"
#include "Parsers.h"

class Loader
{
public:
	Loader();
	~Loader();

	void Load(Effect* effect, const ln::PathName& hlslFilePath);

	const StringA& GetPreprocessedHLSLCode() const { return m_preprocessedHLSLCode; }

	void EncodeAnnotationTypeName(D3DXHANDLE handle, D3DXPARAMETER_DESC desc, String* outType, String* outValue);

private:
	ID3DXEffect*	m_dxEffect;
	StringA			m_preprocessedHLSLCode;
	//std::map<D3DXPARAMETER_TYPE, String>	m_typeNameTable;
};
