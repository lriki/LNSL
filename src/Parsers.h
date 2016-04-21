
#pragma once
#include "Symbols.h"


//	�T���v���ϐ��ƃe�N�X�`���ϐ��̌��т����s���N���X�B
class SamplerLinker
{
public:

	//struct SamplerPair
	//{
	//	std::string		SamplerVarName;
	//	std::string		TextureVarName;

	//	bool operator == ( const SamplerPair& val ) { return SamplerVarName == val.SamplerVarName; }
	//};

	///// �e�N�X�`���^�ϐ��Ƃ��̒l
	//struct TextureVar
	//{
	//	std::string				Name;
	//	IDirect3DBaseTexture9*	Texture;
	//};

	//typedef std::vector<SamplerPair>	SamplerPairArray;
	//typedef std::vector<TextureVar>		TextureVarArray;

	//SamplerPairArray	mSamplerPairArray;	// output

public:

	SamplerLinker(Effect* effect);
	~SamplerLinker();

	void analyze( IDirect3DDevice9* device, ID3DXEffect* effect );
	String Parse(ln::parser::TokenListPtr& tokenList);

	String GetTextureNameBySampler(const String& name)
	{
		return m_samplerTextureMap[name];
	}

private:

	void analyzeSampler( D3DXHANDLE handle, const char* name );

	//TextureVar* findTextureVar( IDirect3DBaseTexture9* texture );

	//void addSamplerPair( const SamplerPair& var );

private:
	Effect*	m_effect;
	std::map<String, String>	m_samplerTextureMap;

	//IDirect3DDevice9*	mDxDevice;
	//ID3DXEffect*		mDxEffect;
	//TextureVarArray		mTextureVarArray;

};

