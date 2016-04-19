
#include "Common.h"
#include "Parsers.h"

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






/*
	一番シンプル

		static Result<Data> Statement(ParserManager& parser)
		{
			auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
			auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
			auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
		}

	↑のをちゃんとした Parser を返すようにしたもの

		static Parser<Data> Statement(ParserManager& parser)
		{
			return [](ParserManager& parser)
			{
				auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
				auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
				auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
				auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
				return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
			};
		}

	本当に関数型っぽくするならこうなる 1

		static Parser<Data> Statement(ParserManager& parser)
		{
			Parser<Token> p, t1, t2, t3, t4;
			p = (t1 = Token(ln::parser::CommonTokenType::Identifier))
			&&	(t2 = Token(ln::parser::CommonTokenType::Operator)))
			&&	(t3 = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			&&	(t4 = Token(ln::parser::CommonTokenType::Operator));
			return [p, t1, t3](ParserManager& parser) { return p(parser).Then( Data(t1.GetValue, t2.GetValue()) ) };
		}
	
	本当に関数型っぽくするならこうなる 2

		static Parser<Data> Statement(ParserManager& parser)	// 値渡しにするか、呼び出し元を DoParse とか1つかませないとだめ
		{
			return
				(parser[0] = Token(ln::parser::CommonTokenType::Identifier))
			|	(parser[1] = Token(ln::parser::CommonTokenType::Operator)))
			|	(parser[2] = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			|	(parser[3] = Token(ln::parser::CommonTokenType::Operator));
			->	[](ParserManager& parser) { return Data(parser[0].GetValue(), parser[1].GetValue()) };
		}
	

*/



class ParserCursor;

template<typename TCursor>
class ParserManager;

// ParserResult の決まりごと
//	- 成否 (true/false) を持つ
//	- 値を持つ (失敗の場合は不正値)
//	- 次の読み取り位置を持つ (remainder)
// T はパーサ関数の戻す値
template<typename T, typename TCursor>
class ParserResult
{
public:
	static ParserResult<T, TCursor> Success(const T& value, const TCursor& remainder)
	{
		return ParserResult<T, TCursor>(value, remainder, true);
	}
	static ParserResult<T, TCursor> Failed(const TCursor& remainder)
	{
		return ParserResult<T, TCursor>(T(), remainder, false);
	}

	const T& GetValue() const { return m_value; }
	const TCursor& GetRemainder() const { return m_remainder; }	// 評価後の次の読み取り位置
	bool IsSucceed() const { return m_isSuccess; }
	bool IsFailed() const { return !m_isSuccess; }

	
private:
	ParserResult(const T& value, const TCursor& remainder, bool isSuccess)
		: m_value(value)
		, m_remainder(remainder)
		, m_isSuccess(isSuccess)
	{
	}

	T			m_value;
	TCursor		m_remainder;
	bool		m_isSuccess;
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


template<typename TValue, typename TCursor>
class Parser : public ln::Delegate<ParserResult<TValue, TCursor>(ParserManager<TCursor>& parser)>
{
public:
	template<typename TParserFunc>
	Parser(TParserFunc func)
		: ln::Delegate<ParserResult<TValue, TCursor>(ParserManager<TCursor>& parser)>(func)
	{
	}

	using FuncPtr = ParserResult<TValue, TCursor>(*)(ParserManager<TCursor>& parser);

	Parser(FuncPtr func)
		: ln::Delegate<ParserResult<TValue, TCursor>(ParserManager<TCursor>& parser)>(func)
	{
	}

	//ParserResult<T> TryParse(const ln::parser::TokenList* tokenList)
	//{
	//	return this->Call(ParserCursor(tokenList));
	//}
};

// 現在位置と実行関数(ユーティリティ)
template<typename TCursor = ParserCursor>
class ParserManager
{
public:
	//using TCursor = ParserCursor;

	ParserManager() {}
	ParserManager(const TCursor& pos) : m_currentinEval(pos) {}

	template<typename T>
	ParserResult<T, TCursor> TryParse(const Parser<T, TCursor>& parser, const ln::parser::TokenList* tokenList)
	{
		m_currentinEval = TCursor(tokenList);
		ParserResult<T, TCursor> result = parser.Call(*this);
		if (result.IsFailed())
			LN_THROW(0, ln::InvalidFormatException);
		return result;
	}


	template<typename T>
	T Eval(const Parser<T, TCursor>& parser)
	{
		TCursor oldPos = m_currentinEval;
		ParserResult<T, TCursor> result = parser.Call(*this);
		if (result.IsFailed())
		{
			m_currentinEval = oldPos;
			LN_THROW(0, ln::InvalidFormatException);
		}
		m_currentinEval = result.GetRemainder();
		return result.GetValue();
	}


	template<typename T>
	ParserResult<T, TCursor> DoParser(const Parser<T, TCursor>& parser)
	{
		TCursor oldPos = m_currentinEval;	// エラー時の復帰用に覚えておく
		try
		{
			ParserResult<T, TCursor> result = parser.Call(*this);
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
		catch (ln::Exception&)
		{
			return ParserResult<T, TCursor>::Failed(m_currentinEval);
		}
		return ParserResult<T, TCursor>::Failed(m_currentinEval);
	}

	template<typename T>
	ParserResult<T, TCursor> Success(const T& value, const TCursor& remainder)
	{
		return ParserResult<T, TCursor>::Success(value, remainder/*, m_currentinEval*/);
	}
	template<typename T>
	ParserResult<T, TCursor> Failed(const TCursor& remainder)
	{
		return ParserResult<T, TCursor>::Failed(remainder);
	}

	const TCursor& GetPosition() const { return m_currentinEval; }	// パーサ関数成功後、次の読み取り位置をいくつ進めるかはパーサ関数の役目なので公開する

	const ln::parser::Token& GetCurrentValue() const
	{
		return m_currentinEval.GetCurrentValue();
	}

private:
	TCursor	m_currentinEval;
};









template<typename TParserCursor>
class ParseLib
{
public:
	using ValueT = ln::parser::Token;
	using TParserManager = ParserManager<TParserCursor>;

	template<typename TValue>
	using Parser = ::Parser<TValue, TParserCursor>;

	static Parser<ValueT> Token(ln::parser::CommonTokenType type)
	{
		return [type](TParserManager& parser)
		{
			if (parser.GetCurrentValue().GetCommonType() == type)
				return parser.Success(parser.GetCurrentValue(), parser.GetPosition().Advance());//ParserResult<ln::parser::Token>::Success(parser.GetCurrentValue(), parser.Advance());
			return parser.Failed<ValueT>(parser.GetPosition());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, char ch)
	{
		return [type, ch](TParserManager& parser)
		{
			if (parser.GetCurrentValue().GetCommonType() == type && parser.GetCurrentValue().EqualChar(ch))
				return parser.Success(parser.GetCurrentValue(), parser.GetPosition().Advance());
			return parser.Failed<ValueT>(parser.GetPosition());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, const char* string, int len)
	{
		return [type, string, len](TParserManager& parser)
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
		return [internalParser](TParserManager& parser)
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

	template<typename T, typename TParserFunc>
	static Parser<T> Or(const Parser<T>& first, TParserFunc second_/*const Parser<T>& second*/)
	{
		Parser<T> second(second_);
		return [first, second](TParserManager& parser)
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




class ParserCursor_SkipSpace
{
public:
	ParserCursor_SkipSpace()
		: m_tokenList(nullptr)
		, m_position(0)
	{}

	// パース開始時の初期化用
	ParserCursor_SkipSpace(const ln::parser::TokenList* tokenList)
		: m_tokenList(tokenList)
		, m_position(0)
	{
	}

	// パース開始時の初期化用
	ParserCursor_SkipSpace(const ln::parser::TokenList* tokenList, int position)
		: m_tokenList(tokenList)
		, m_position(position)
	{
	}

	ParserCursor_SkipSpace(const ParserCursor_SkipSpace& obj)
		: m_tokenList(obj.m_tokenList)
		, m_position(obj.m_position)
	{
	}

	const ln::parser::Token& GetCurrentValue() const
	{
		return m_tokenList->GetAt(m_position);
	}

	ParserCursor_SkipSpace Advance() const
	{
		if (m_position == m_tokenList->GetCount())
		{
			LN_THROW(0, ln::InvalidOperationException, "end of source.");
		}

		int pos = m_position;
		do
		{
			++pos;
		} while (m_tokenList->GetAt(pos).GetCommonType() == parser::CommonTokenType::SpaceSequence);

		return ParserCursor_SkipSpace(m_tokenList, pos);
	}

private:
	const ln::parser::TokenList*	m_tokenList;
	int								m_position;
};






class TokenParser : public ParseLib<ParserCursor_SkipSpace>
{
public:

	using ParserManager = ::ParserManager<ParserCursor_SkipSpace>;

	template<typename TValue>
	using Result = ParserResult<TValue, ParserCursor_SkipSpace>;

	struct Data
	{
		String	left;
		String	right;
	};

	static Result<ln::parser::Token> Parse_texture_variable(ParserManager& parser)
	{
		auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Operator, '<'));
		auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
		auto t3 = parser.Eval(Token(ln::parser::CommonTokenType::Operator, '>'));
		return parser.Success(t2, parser.GetPosition());
	}

	static Result<Data> Statement(ParserManager& parser)
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
		auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
		auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
		auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parse_texture_variable));
		auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
		return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
	}


};

void SamplerLinker::Parse(const ln::parser::TokenListPtr& tokenList)
{
	ln::String input =
		"Texture = <g_MeshTexture>;"
		"MinFilter=LINEAR;"
		"MagFilter=NONE;"
		"MipFilter=NONE;"
		"AddressU=WRAP;"
		"AddressV=WRAP;";

	ln::parser::CppLexer lex;
	ln::parser::DiagnosticsItemSet diag;
	ln::parser::TokenListPtr tokens = lex.Tokenize(input.c_str(), &diag);

	//auto left = ParseLib::Token(ln::parser::CommonTokenType::Identifier, "MinFilter", 9);
	//auto stmt = Parser<TokenParser::Data>(TokenParser::Statement);

	TokenParser::ParserManager parser;
	auto manyStmt = TokenParser::Many<TokenParser::Data>(TokenParser::Statement);
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


