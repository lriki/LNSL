
#include "Loader.h"
#include "DXDevice.h"

Loader::Loader()
	: m_dxEffect(nullptr)
{
}

Loader::~Loader()
{
	SAFE_RELEASE(m_dxEffect);
}

void Loader::Load(Effect* effect, const ln::PathName& hlslFilePath)
{
	DXDevice dxDevice;
	dxDevice.initlaize();

	ln::PathName tmpHLSLFilePath = hlslFilePath.ChangeExtension(_T("tmp"));

	//--------------------------------------------------------------
	// 組み込み (先頭に #include 追加)

	//Base::ReferenceBuffer* text = FileIO::File::readAllBytes(filePath);
	//std::string orgHLSLText = std::string((const lnChar*)text->getPointer(), text->getSize());

	//std::string newHLSLText = std::string((const lnChar*)BuiltInHLSL/*, BuiltInHLSL_Size*/);
	//newHLSLText += "\n#line 1 \"";
	//newHLSLText += filePath;
	//newHLSLText += "\"\n";
	//newHLSLText += orgHLSLText;
	//FileIO::File::writeAllBytes(tmpHLSLFileName.c_str(), (const lnByte*)newHLSLText.c_str(), newHLSLText.size());
	//text->release();

	ln::ByteBuffer text = ln::FileSystem::ReadAllBytes(hlslFilePath.c_str());
	ln::FileSystem::WriteAllBytes(tmpHLSLFilePath.c_str(), text.GetConstData(), text.GetSize());

	//--------------------------------------------------------------
	// プリプロセス
	LPD3DXBUFFER pShaderText;
	LPD3DXBUFFER pErrorMsgs = NULL;
	HRESULT hr = ::D3DXPreprocessShaderFromFile(
		tmpHLSLFilePath.c_str(),
		NULL,
		NULL,
		&pShaderText,
		&pErrorMsgs);
	if (FAILED(hr))
	{
		std::string errorMsg((char*)pErrorMsgs->GetBufferPointer(), pErrorMsgs->GetBufferSize());
		printf("%s\n", errorMsg.c_str());
		SAFE_RELEASE(pErrorMsgs);
		LN_THROW(0, ln::InvalidFormatException);
	}

	m_preprocessedHLSLCode = StringA((char*)pShaderText->GetBufferPointer(), pShaderText->GetBufferSize());
	//ln::String input((char*)pShaderText->GetBufferPointer(), pShaderText->GetBufferSize());
	//ln::FileSystem::WriteAllText("tmp.txt", input);

	SAFE_RELEASE(pShaderText);
	SAFE_RELEASE(pErrorMsgs);


	ln::parser::CppLexer lex;
	ln::parser::DiagnosticsItemSet diag;
	ln::parser::TokenListPtr tokens = lex.Tokenize(m_preprocessedHLSLCode.c_str(), &diag);
	
	SamplerLinker samplerLinker(effect);
	effect->convertableCode = samplerLinker.Parse(tokens);

	//--------------------------------------------------------------
	// コンパイルして Effect 作成
	ID3DXBuffer* errorBuf = NULL;
	hr = ::D3DXCreateEffect(
		dxDevice.getDxDevice(),
		m_preprocessedHLSLCode.c_str(),
		m_preprocessedHLSLCode.GetLength(),
		NULL,
		NULL,
		D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
		NULL,
		&m_dxEffect,
		&errorBuf);
	if (FAILED(hr))
	{
		std::string errorMsg((char*)errorBuf->GetBufferPointer(), errorBuf->GetBufferSize());
		printf("%s\n", errorMsg.c_str());
		SAFE_RELEASE(errorBuf);
		LN_THROW(0, ln::InvalidFormatException);
	}
	SAFE_RELEASE(errorBuf);

	//--------------------------------------------------------------
	// パラメータ
	D3DXHANDLE handle;
	UINT idx = 0;
	while (true)
	{
		handle = m_dxEffect->GetParameter(NULL, idx);
		if (!handle) break;

		D3DXPARAMETER_DESC desc;
		m_dxEffect->GetParameterDesc(handle, &desc);

		ParameterInfo info;
		info.name = desc.Name;
		info.semantic = desc.Semantic ? desc.Semantic : "";
		info.shared = ((desc.Flags & D3DX_PARAMETER_SHARED) != 0);

		// サンプラ型の場合は、関連付けられているテクスチャ型変数名も格納
		switch (desc.Type)
		{
		case D3DXPT_SAMPLER:
		case D3DXPT_SAMPLER1D:
		case D3DXPT_SAMPLER2D:
		case D3DXPT_SAMPLER3D:
		case D3DXPT_SAMPLERCUBE:
		{
			String texName = samplerLinker.GetTextureNameBySampler(desc.Name);
			if (!texName.IsEmpty())
			{
				info.samplerName = info.name;
				info.name = texName;
			}
			break;
		}
		default:
			break;
		}

		// アノテーション
		for (UINT i = 0; i < desc.Annotations; ++i)
		{
			D3DXHANDLE anno = m_dxEffect->GetAnnotation(handle, i);
			D3DXPARAMETER_DESC desc;
			const char* value = NULL;
			m_dxEffect->GetParameterDesc(anno, &desc);
			m_dxEffect->GetString(anno, &value);

			AnnotationInfo annoinfo;
			annoinfo.name = desc.Name;
			annoinfo.value = value;
			info.annotations.Add(annoinfo);
		}

		effect->parameterList.Add(info);
		++idx;
	}

	SAFE_RELEASE(m_dxEffect);
}
