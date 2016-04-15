
#pragma once
#include "Common.h"

class Loader
{
public:
	Loader();
	~Loader();

	void Load(const ln::PathName& hlslFilePath);

private:
	ID3DXEffect*	m_dxEffect;
};
