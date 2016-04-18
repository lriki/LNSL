
#include "Common.h"
#include "SamplerLinker.h"

SamplerLinker::~SamplerLinker()
{
	for( TextureVar& var : mTextureVarArray )
	{
		SAFE_RELEASE( var.Texture );
	}
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void SamplerLinker::analyze( IDirect3DDevice9* device, ID3DXEffect* effect )
{
	mDxDevice = device;
	mDxEffect = effect;

	// まずはテクスチャ型変数にダミーテクスチャを作って設定
    for ( UINT i = 0; ; ++i )
    {
        D3DXHANDLE handle = mDxEffect->GetParameter( NULL, i );
        if ( !handle ) break;

		D3DXPARAMETER_DESC desc;
		mDxEffect->GetParameterDesc( handle, &desc );

		switch ( desc.Type )
		{
			case D3DXPT_TEXTURE:
			case D3DXPT_TEXTURE1D:
			case D3DXPT_TEXTURE2D:
			case D3DXPT_TEXTURE3D:
			case D3DXPT_TEXTURECUBE:
			{
				TextureVar texVer;
				texVer.Name = desc.Name;
				IDirect3DTexture9* dxtex;
				HRESULT hr = mDxDevice->CreateTexture( 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &dxtex, NULL );
				texVer.Texture = dxtex;
				if ( FAILED( hr ) ) {
					printf( "failed create dummy texture." );
				}
				mTextureVarArray.push_back( texVer );
				mDxEffect->SetTexture( handle, texVer.Texture );
				break;
			}
			default:
				break;
		}
	}

	// サンプラ型変数ごとに解析
    for ( UINT i = 0; ; ++i )
    {
        D3DXHANDLE handle = mDxEffect->GetParameter( NULL, i );
        if ( !handle ) break;

		D3DXPARAMETER_DESC desc;
		mDxEffect->GetParameterDesc( handle, &desc );

		switch ( desc.Type )
		{
			case D3DXPT_SAMPLER:
			case D3DXPT_SAMPLER1D:
			case D3DXPT_SAMPLER2D:
			case D3DXPT_SAMPLER3D:
			case D3DXPT_SAMPLERCUBE:
				analyzeSampler( handle, desc.Name );
				break;
			default:
				break;
		}
	}
}




class ParserCursor;
class ParserManager;

// Result の決まりごと
//	- 成否 (true/false) を持つ
//	- 値を持つ (失敗の場合は不正値)
//	- 次の読み取り位置を持つ (remainder)
// T はパーサ関数の戻す値
template<typename T>
class Result
{
public:
	static Result<T> Success(const T& value, const ParserCursor& remainder)
	{
		return Result<T>(value, remainder, true);
	}
	static Result<T> Failed(const ParserCursor& remainder)
	{
		return Result<T>(T(), remainder, false);
	}

	const T& GetValue() const { return m_value; }
	const ParserCursor& GetRemainder() const { return m_remainder; }	// 評価後の次の読み取り位置
	bool IsSucceed() const { return m_isSuccess; }
	bool IsFailed() const { return !m_isSuccess; }

	
private:
	Result(const T& value, const ParserCursor& remainder, bool isSuccess)
		: m_value(value)
		, m_remainder(remainder)
		, m_isSuccess(isSuccess)
	{
	}

	T				m_value;
	ParserCursor	m_remainder;
	bool			m_isSuccess;
};


class ParserCursor
{
public:
	ParserCursor()
		: m_tokenList(nullptr)
		, m_position(0)
	{}

	// パース開始時の初期化用
	ParserCursor(const ln::parser::TokenList* tokenList)
		: m_tokenList(tokenList)
		, m_position(0)
	{
	}

	// パース開始時の初期化用
	ParserCursor(const ln::parser::TokenList* tokenList, int position)
		: m_tokenList(tokenList)
		, m_position(position)
	{
	}

	ParserCursor(const ParserCursor& obj)
		: m_tokenList(obj.m_tokenList)
		, m_position(obj.m_position)
	{
	}

	const ln::parser::Token& GetCurrentValue() const
	{
		return m_tokenList->GetAt(m_position);
	}

	ParserCursor Advance() const
	{
		if (m_position == m_tokenList->GetCount())
		{
			LN_THROW(0, ln::InvalidOperationException, "end of source.");
		}
		return ParserCursor(m_tokenList, m_position + 1);
	}

private:
	const ln::parser::TokenList*	m_tokenList;
	int								m_position;
};


template<typename T>
class Parser : public ln::Delegate<Result<T>(ParserManager& parser)>
{
public:
	template<typename TParserFunc>
	Parser(TParserFunc func)
		: ln::Delegate<Result<T>(ParserManager& parser)>(func)
	{
	}

	//Result<T> TryParse(const ln::parser::TokenList* tokenList)
	//{
	//	return this->Call(ParserCursor(tokenList));
	//}
};

// 現在位置と実行関数(ユーティリティ)
class ParserManager
{
public:
	ParserManager() {}
	ParserManager(const ParserCursor& pos) : m_currentinEval(pos) {}

	template<typename T>
	Result<T> TryParse(const Parser<T>& parser, const ln::parser::TokenList* tokenList)
	{
		m_currentinEval = ParserCursor(tokenList);
		Result<T> result = parser.Call(*this);
		if (result.IsFailed())
			LN_THROW(0, ln::InvalidFormatException);
		return result;
	}


	template<typename T>
	T Eval(const Parser<T>& parser)
	{
		ParserCursor oldPos = m_currentinEval;
		Result<T> result = parser.Call(*this);
		if (result.IsFailed())
		{
			m_currentinEval = oldPos;
			LN_THROW(0, ln::InvalidFormatException);
		}
		m_currentinEval = result.GetRemainder();
		return result.GetValue();
	}


	template<typename T>
	Result<T> DoParser(const Parser<T>& parser)
	{
		ParserCursor oldPos = m_currentinEval;	// エラー時の復帰用に覚えておく
		try
		{
			Result<T> result = parser.Call(*this);
			if (result.IsFailed())
			{
				m_currentinEval = oldPos;
			}
			else
			{
				m_currentinEval = result.GetRemainder();
			}
			return result;
		}
		catch (ln::Exception& e)
		{
			return Result<T>::Failed(m_currentinEval);
		}
		return Result<T>::Failed(m_currentinEval);
	}

	template<typename T>
	Result<T> Success(const T& value, const ParserCursor& remainder)
	{
		return Result<T>::Success(value, remainder/*, m_currentinEval*/);
	}
	template<typename T>
	Result<T> Failed(const ParserCursor& remainder)
	{
		return Result<T>::Failed(remainder);
	}

	const ParserCursor& GetPosition() const { return m_currentinEval; }	// パーサ関数成功後、次の読み取り位置をいくつ進めるかはパーサ関数の役目なので公開する

	const ln::parser::Token& GetCurrentValue() const
	{
		return m_currentinEval.GetCurrentValue();
	}

private:
	ParserCursor	m_currentinEval;
};



class ParseLib
{
public:
	using ValueT = ln::parser::Token;

	static Parser<ValueT> Token(ln::parser::CommonTokenType type)
	{
		return [type](ParserManager& parser)
		{
			if (parser.GetCurrentValue().GetCommonType() == type)
				return parser.Success(parser.GetCurrentValue(), parser.GetPosition().Advance());//Result<ln::parser::Token>::Success(parser.GetCurrentValue(), parser.Advance());
			return parser.Failed<ValueT>(parser.GetPosition());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, char ch)
	{
		return [type, ch](ParserManager& parser)
		{
			if (parser.GetCurrentValue().GetCommonType() == type && parser.GetCurrentValue().EqualChar(ch))
				return parser.Success(parser.GetCurrentValue(), parser.GetPosition().Advance());
			return parser.Failed<ValueT>(parser.GetPosition());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, const char* string, int len)
	{
		return [type, string, len](ParserManager& parser)
		{
			if (parser.GetCurrentValue().GetCommonType() == type && parser.GetCurrentValue().EqualString(string, len))
				return parser.Success(parser.GetCurrentValue(), parser.GetPosition().Advance());
			return parser.Failed<ValueT>(parser.GetPosition());	// TODO: メッセージあるとよい
		};
	}

	// 0回以上の繰り返し
	template<typename T>
	static Parser<ln::Array<T>> Many(const Parser<T>& internalParser)
	{
		return [internalParser](ParserManager& parser)
		{
			ln::Array<T> list;
			auto r = parser.DoParser(internalParser);
			while (r.IsSucceed())
			{
				list.Add(r.GetValue());
				//ParserManager next(r.GetRemainder());
				r = parser.DoParser(internalParser);
			}
			return parser.Success(list, parser.GetPosition());
		};
	}

	template<typename T>
	static Parser<T> Or(const Parser<T>& first, const Parser<T>& second)
	{
		return [first, second](ParserManager& parser)
		{
			auto fr = parser.DoParser(first);
			if (fr.IsFailed())
			{
				return parser.DoParser(second);
			}
			return fr;
		};
	}

};




struct Data
{
	ln::String	left;
	ln::String	right;
};

Result<ln::parser::Token> Parser_texture_variable(ParserManager& parser)
{
	auto t1 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Operator, '<'));
	auto t2 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Identifier));
	auto t3 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Operator, '>'));
	return parser.Success(t2, parser.GetPosition());
}

Result<Data> Statement(ParserManager& parser)
{
	// 本来ならt1とかはTokenを返す関数オブジェクトになるのが良い。
	// が、それだと
	//		auto result1 = i1.Parse();
	//		if (result1.IsFailed()) return result1;
	//		・・・
	//		return parser.Success(Data{ t1.GetValue.ToString(), ・・・ });
	// みたいに書く必要があり非常に冗長。
	// Sprache は LINQ で呼び出される Where や Select に細工がしてあり、
	// パーサ失敗は throw 使わなくても表現できるのだが、C++ には LINQ みたいなのが無い。
	// そうでなければ boost Spirit に手を出すか…
	// http://sssslide.com/www.slideshare.net/yak1ex/impractical-introduction-ofboostspiritqi-pdf
	auto t1 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Identifier));
	auto t2 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Operator));
	auto t3 = parser.Eval(ParseLib::Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
	auto t4 = parser.Eval(ParseLib::Token(ln::parser::CommonTokenType::Operator));
	return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
}

void SamplerLinker::Parse(const ln::parser::TokenListPtr& tokenList)
{
	ln::String input =
		"Texture=<g_MeshTexture>;"
		"MinFilter=LINEAR;"
		"MagFilter=NONE;"
		"MipFilter=NONE;"
		"AddressU=WRAP;"
		"AddressV=WRAP;";

	ln::parser::CppLexer lex;
	ln::parser::DiagnosticsItemSet diag;
	ln::parser::TokenListPtr tokens = lex.Tokenize(input.c_str(), &diag);

	//auto left = ParseLib::Token(ln::parser::CommonTokenType::Identifier, "MinFilter", 9);
	auto stmt = Parser<Data>(Statement);

	ParserManager parser;
	auto manyStmt = ParseLib::Many<Data>(Statement);
	auto result = parser.TryParse(manyStmt, tokens);

	//
	//auto result = stmt.TryParse(tokens);

	printf("");
}



//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void SamplerLinker::analyzeSampler( D3DXHANDLE handle, const char* name )
{
	printf("▼ %s の検索 ----------------\n", name);
	// TODO 重複の排除

	for ( UINT iTech = 0; ; ++iTech )
	{
		D3DXHANDLE tech = mDxEffect->GetTechnique( iTech );
		if ( tech ) 
		{
			mDxEffect->SetTechnique( tech );
			UINT passes = 0;
			mDxEffect->Begin( &passes, 0 );
			for ( UINT iPass = 0; iPass < passes; ++iPass )
			{
				mDxEffect->BeginPass( iPass );

				//printf("pass %d ----\n", iPass);

				IDirect3DPixelShader9* ps = NULL;
				mDxDevice->GetPixelShader( &ps );
				if ( ps )
				{
					UINT size;
					ps->GetFunction( NULL, &size );

					ln::byte_t* funcBuf = new ln::byte_t[size];
					ps->GetFunction( funcBuf, &size );

					ID3DXConstantTable* ct;
					D3DXGetShaderConstantTable( (const DWORD*)funcBuf, &ct );

					D3DXHANDLE sampler = ct->GetConstantByName( NULL, name );
					if ( sampler )
					{
						UINT index = ct->GetSamplerIndex( sampler );

						// サンプラの index にセットされているテクスチャ
						IDirect3DBaseTexture9* basetex;
						mDxDevice->GetTexture( index, &basetex );
							
						if ( basetex )
						{
							IDirect3DBaseTexture9* tex = basetex;//dynamic_cast<IDirect3DTexture9*>( basetex );
							if ( tex )
							{
								TextureVar* var = findTextureVar( tex );
								if ( var )
								{
									SamplerPair pair;
									pair.SamplerVarName = name;
									pair.TextureVarName = var->Name;
									addSamplerPair( pair );
									//printf("◆ %s <- %s\n", pair.SamplerVarName.c_str(), pair.TextureVarName.c_str() );
								}
							}
						}
					}


					delete[] funcBuf;
				}


				mDxEffect->EndPass();
			}
			mDxEffect->End();
		}
		else
		{
			break;
		}
	}
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
const char* SamplerLinker::getTextureNameBySampler( const char* name )
{
	for ( SamplerPair& var : mSamplerPairArray )
	{
		if ( var.SamplerVarName == name ) {
			return var.TextureVarName.c_str();
		}
	}
	return NULL;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
SamplerLinker::TextureVar* SamplerLinker::findTextureVar( IDirect3DBaseTexture9* texture )
{
	for ( TextureVar& var : mTextureVarArray )
	{
		if ( var.Texture == texture ) {
			return &var;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void SamplerLinker::addSamplerPair( const SamplerPair& var )
{
	SamplerPairArray::iterator itr = std::find( mSamplerPairArray.begin(), mSamplerPairArray.end(), var );
	if ( itr == mSamplerPairArray.end() )
	{
		printf("%s < %s\n", var.SamplerVarName.c_str(), var.TextureVarName.c_str() );
		mSamplerPairArray.push_back( var );
	}
}


