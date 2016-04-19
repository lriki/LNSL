
#pragma once
#include <vector>
#include <string>


//	�T���v���ϐ��ƃe�N�X�`���ϐ��̌��т����s���N���X�B
class SamplerLinker
{
public:

	struct SamplerPair
	{
		std::string		SamplerVarName;
		std::string		TextureVarName;

		bool operator == ( const SamplerPair& val ) { return SamplerVarName == val.SamplerVarName; }
	};

	/// �e�N�X�`���^�ϐ��Ƃ��̒l
	struct TextureVar
	{
		std::string				Name;
		IDirect3DBaseTexture9*	Texture;
	};

	typedef std::vector<SamplerPair>	SamplerPairArray;
	typedef std::vector<TextureVar>		TextureVarArray;

	SamplerPairArray	mSamplerPairArray;	// output

public:

	~SamplerLinker();

	void analyze( IDirect3DDevice9* device, ID3DXEffect* effect );
	void Parse(const ln::parser::TokenListPtr& tokenList);

	const char* getTextureNameBySampler( const char* name );

private:

	void analyzeSampler( D3DXHANDLE handle, const char* name );

	TextureVar* findTextureVar( IDirect3DBaseTexture9* texture );

	void addSamplerPair( const SamplerPair& var );

private:

	

	IDirect3DDevice9*	mDxDevice;
	ID3DXEffect*		mDxEffect;
	TextureVarArray		mTextureVarArray;
};

