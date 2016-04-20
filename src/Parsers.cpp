
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

	// �܂��̓e�N�X�`���^�ϐ��Ƀ_�~�[�e�N�X�`��������Đݒ�
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

	// �T���v���^�ϐ����Ƃɉ��
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
	��ԃV���v��

		static Result<Data> Statement(ParserContext& parser)
		{
			auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
			auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
			auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
		}

	���̂������Ƃ��� Parser ��Ԃ��悤�ɂ�������

		static Parser<Data> Statement(ParserContext& parser)
		{
			return [](ParserContext& parser)
			{
				auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
				auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
				auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
				auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
				return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
			};
		}

	�{���Ɋ֐��^���ۂ�����Ȃ炱���Ȃ� 1

		static Parser<Data> Statement(ParserContext& parser)
		{
			Parser<Token> p, t1, t2, t3, t4;
			p = (t1 = Token(ln::parser::CommonTokenType::Identifier))
			&&	(t2 = Token(ln::parser::CommonTokenType::Operator)))
			&&	(t3 = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			&&	(t4 = Token(ln::parser::CommonTokenType::Operator));
			return [p, t1, t3](ParserContext& parser) { return p(parser).Then( Data(t1.GetValue, t2.GetValue()) ) };
		}
	
	�{���Ɋ֐��^���ۂ�����Ȃ炱���Ȃ� 2

		static Parser<Data> Statement(ParserContext& parser)	// �l�n���ɂ��邩�A�Ăяo������ DoParse �Ƃ�1���܂��Ȃ��Ƃ���
		{
			return
				(parser[0] = Token(ln::parser::CommonTokenType::Identifier))
			|	(parser[1] = Token(ln::parser::CommonTokenType::Operator)))
			|	(parser[2] = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			|	(parser[3] = Token(ln::parser::CommonTokenType::Operator));
			->	[](ParserContext& parser) { return Data(parser[0].GetValue(), parser[1].GetValue()) };
		}
	


	��ԃV���v���Ȃ̂��}�N���g���悤�ɂ���ƁA��Olongjmp���Ȃ��čς�

		static Result<Data> Statement(ParserContext& parser)
		{
			LN_PARSE(t1, Token(ln::parser::CommonTokenType::Identifier));
			LN_PARSE(t2, Token(ln::parser::CommonTokenType::Operator));
			LN_PARSE(t3, Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
			LN_PARSE(t4, Token(ln::parser::CommonTokenType::Operator));
			return parser.Success(Data{ t1.ToString(), t3.ToString() });
		}

*/


#define LN_PARSE(result, parser) \
	auto result##_ = (parser).Call(input); \
	if (result##_.IsFailed()) return Fail(input); \
	input.Next(result##_.GetRemainder()); \
	auto result = result##_.GetValue();

#define LN_PARSE_RESULT(result, parser) \
	auto result = (parser).Call(input); \
	if (result.IsFailed()) return Fail(input); \
	input.Next(result.GetRemainder());

#define LN_PARSE_SUCCESS(value)	\
	Success(value, input);

namespace combinators
{

namespace detail
{

template<typename TCursor>
class ParserFailure
{
public:
	TCursor remainder;
};

} // namespace detail


// ParserResult �̌��܂育��
//	- ���� (true/false) ������
//	- �l������ (���s�̏ꍇ�͕s���l)
//	- ���̓ǂݎ��ʒu������ (remainder)
// T �̓p�[�T�֐��̖߂��l
template<typename T, typename TCursor>
class GenericParserResult
{
public:
	static GenericParserResult<T, TCursor> Success(const T& value, const TCursor& remainder)
	{
		return GenericParserResult<T, TCursor>(value, remainder, true);
	}
	static GenericParserResult<T, TCursor> Failed(const TCursor& remainder)
	{
		return GenericParserResult<T, TCursor>(T(), remainder, false);
	}

	const T& GetValue() const { return m_value; }
	const TCursor& GetRemainder() const { return m_remainder; }	// �]����̎��̓ǂݎ��ʒu
	bool IsSucceed() const { return m_isSuccess; }
	bool IsFailed() const { return !m_isSuccess; }


	GenericParserResult(const detail::ParserFailure<TCursor>& failer)
		: m_value()
		, m_remainder(failer.remainder)
		, m_isSuccess(false)
	{
	}
	
private:
	GenericParserResult(const T& value, const TCursor& remainder, bool isSuccess)
		: m_value(value)
		, m_remainder(remainder)
		, m_isSuccess(isSuccess)
	{
	}

	T			m_value;
	TCursor		m_remainder;
	bool		m_isSuccess;
};



struct ParserCursorConditional
{
	struct Always
	{
		template<typename T>
		bool operator()(const T& value)
		{
			return true;
		}
	};
};

template<typename TConditional = ParserCursorConditional::Always>
class ParserCursor
{
public:
	ParserCursor()
		: m_tokenList(nullptr)
		, m_position(0)
	{}

	// �p�[�X�J�n���̏������p
	ParserCursor(const ln::parser::TokenList* tokenList)
		: m_tokenList(tokenList)
		, m_position(0)
	{
	}

	// �p�[�X�J�n���̏������p
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

	ParserCursor& operator=(const ParserCursor& obj)
	{
		m_tokenList = obj.m_tokenList;
		m_position = obj.m_position;
		return *this;
	}

	const ln::parser::Token& GetCurrentValue() const
	{
		return m_tokenList->GetAt(m_position);
	}

	int GetPosition() const
	{
		return m_position;
	}

	ParserCursor Cuing() const
	{
		TConditional cond;
		int pos = m_position;
		while (!cond(m_tokenList->GetAt(pos)))
		{
			++pos;
		};
		return ParserCursor(m_tokenList, pos);
	}

	ParserCursor Advance() const
	{
		if (m_position == m_tokenList->GetCount())
		{
			LN_THROW(0, ln::InvalidOperationException, "end of source.");
		}

		TConditional cond;
		int pos = m_position;
		do
		{
			++pos;
		} while (pos < m_tokenList->GetCount() && !cond(m_tokenList->GetAt(pos)));

		return ParserCursor(m_tokenList, pos);

		//return ParserCursor(m_tokenList, m_position + 1);
	}

private:
	const ln::parser::TokenList*	m_tokenList;
	int								m_position;
};


template<typename TCursor>
class GenericParserContext
{
public:
	GenericParserContext(const TCursor& input)
		: m_start(input)
		, m_current(input)
	{
	}

	const TCursor& Advance()
	{
		m_current = m_current.Advance();
		return m_current;
	}

	const parser::Token& GetCurrentValue() const
	{
		return m_current.GetCurrentValue();
	}

	const TCursor& GetStartursor() const
	{
		return m_start;
	}

	const TCursor& GetCurrentCursor() const
	{
		return m_current;
	}

	void Next(const TCursor& newPos)
	{
		m_current = newPos;
	}

	template<typename T>
	GenericParserResult<T, TCursor> Success(const T& value)
	{
		return GenericParserResult<T, TCursor>::Success(value, m_current);
	}

private:
	TCursor		m_start;
	TCursor		m_current;
};


template<typename TValue, typename TCursor, typename TContext>
class GenericParser : public ln::Delegate<GenericParserResult<TValue, TCursor>(TContext)>
{
public:
	template<typename TParserFunc>
	GenericParser(TParserFunc func)
		: ln::Delegate<GenericParserResult<TValue, TCursor>(TContext)>(func)
	{
	}
};


template<typename TParserCursor>
class ParseLib
{
public:
	using ValueT = ln::parser::Token;

	using ParserContext = GenericParserContext<TParserCursor>;

	template<typename TValue>
	using Parser = GenericParser<TValue, TParserCursor, ParserContext>;

	template<typename TValue>
	using ParserResult = GenericParserResult<TValue, TParserCursor>;

	using ParserCursor = TParserCursor;


	
	// TODO: context ��
	static detail::ParserFailure<TParserCursor> Fail(const ParserContext& input)
	{
		detail::ParserFailure<TParserCursor> failer;
		failer.remainder = input.GetCurrentCursor();
		return failer;
	}

	static Parser<ValueT> TokenChar(char ch)
	{
		return [ch](ParserContext input)
		{
			if (input.GetCurrentValue().EqualChar(ch))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.Advance());
			return ParserResult<ValueT>::Failed(input.GetCurrentCursor());	// TODO: ���b�Z�[�W����Ƃ悢
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type)
	{
		return [type](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type)
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.Advance());
			return ParserResult<ValueT>::Failed(input.GetCurrentCursor());	// TODO: ���b�Z�[�W����Ƃ悢
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, char ch)
	{
		return [type, ch](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type && input.GetCurrentValue().EqualChar(ch))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.Advance());
			return ParserResult<ValueT>::Failed(input.GetCurrentCursor());
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, const char* string, int len)
	{
		return [type, string, len](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type && input.GetCurrentValue().EqualString(string, len))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.Advance());
			return ParserResult<ValueT>::Failed(input.GetCurrentCursor());
		};
	}

	// 0��ȏ�̌J��Ԃ�
	template<typename T>
	static Parser<ln::Array<T>> Many(const Parser<T>& parser)
	{
		return [parser](ParserContext input)
		{
			ln::Array<T> list;
			auto r = parser.Call(input);
			//input = r.GetRemainder();

			while (r.IsSucceed())
			{
				list.Add(r.GetValue());
				r = parser.Call(r.GetRemainder());
				//input = r.GetRemainder();
			}
			return ParserResult<ln::Array<T>>::Success(list, r.GetRemainder());
		};
	}

	template<typename T, typename TParserFunc>
	static Parser<T> Or(const Parser<T>& first, TParserFunc second_/*const Parser<T>& second*/)
	{
		Parser<T> second(second_);
		return [first, second](ParserContext input)
		{
			auto fr = first.Call(input);
			if (fr.IsFailed())
			{
				return second.Call(input);
			}
			return fr;
		};
	}



	template<typename T>
	static ParserResult<T> TryParse(const Parser<T>& parser, const ln::parser::TokenList* tokenList)
	{
		TParserCursor input(tokenList);
		ParserResult<T> result = parser.Call(input.Cuing());
		if (result.IsFailed())
			LN_THROW(0, ln::InvalidFormatException);
		return result;
	}
};

} // namespace combinators

struct ParserCursorCondition_SkipSpace
{
	bool operator()(const ln::parser::Token& token)
	{
		// space �ȊO������
		return token.GetCommonType() != parser::CommonTokenType::SpaceSequence;
	}
};

using ParserCursor_SkipSpace = combinators::ParserCursor<ParserCursorCondition_SkipSpace>;


class TokenParser : public combinators::ParseLib<ParserCursor_SkipSpace>
{
public:

	struct Data
	{
		String	left;
		String	right;

		Data() {}
		Data(String l, String r) : left(l), right(r) {}
	};

	static ParserResult<ln::parser::Token> Parse_texture_variable(ParserContext input)
	{
		LN_PARSE(t1, Token(ln::parser::CommonTokenType::Operator, '<'));
		LN_PARSE(t2, Token(ln::parser::CommonTokenType::Identifier));
		LN_PARSE(t3, Token(ln::parser::CommonTokenType::Operator, '>'));
		return input.Success(t2);
	}

	static ParserResult<Data> Statement(ParserContext input)
	{
		LN_PARSE(t1, Token(ln::parser::CommonTokenType::Identifier));
		LN_PARSE(t2, Token(ln::parser::CommonTokenType::Operator));
		LN_PARSE(t3, Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parse_texture_variable));
		LN_PARSE(t4, Token(ln::parser::CommonTokenType::Operator));
		return input.Success(Data(t1.ToString(), t3.ToString()));
	}
};





#if 0

namespace ParameterAnnotationParser
{
	struct Range
	{
		int start;
		int last;
	};

	struct ParserCursorCondition_SkipSpace
	{
		bool operator()(const parser::Token& token)
		{
			return token.EqualChar('<') || token.EqualChar('>') || token.EqualChar('{') || token.EqualChar('}');
		}
	};

	using ParserCursor_SkipSpace = combinators::ParserCursor<ParserCursorCondition_SkipSpace>;


	struct TokenParser : public combinators::ParseLib<ParserCursor_SkipSpace>
	{
		static ParserResult<Range> Parse_GlobalAnnotation(ParserContext input)
		{
			LN_PARSE_RESULT(r1, Token('<'));
			LN_PARSE_RESULT(r2, Token('>'));
			Range r{ input.GetPosition(), r1.GetRemainder().GetPosition() };	// TODO: ����ς� input �� runner �Ƃ��ɂ��āA�J�n�_���ق����B���ƁA�}�N������ {} �������q�͏����Ȃ��̂ŁALN_PARSE_SUCCESS �͂�߂� runner.Success() �ŕԂ�����
			return LN_PARSE_SUCCESS(r);
		}

		static ParserResult<Range> Parse_Block(ParserContext input)
		{
			LN_PARSE_RESULT(r1, Token('{'));
			LN_PARSE_RESULT(r3, Many<Range>(Parse_Block));	// �l�X�g	TODO: �ł���� <Range> �͂�߂���
			LN_PARSE_RESULT(r2, Token('}'));
			Range r{ input.GetPosition(), r1.GetRemainder().GetPosition() };	// TODO: ����ς� input �� runner �Ƃ��ɂ��āA�J�n�_���ق����B���ƁA�}�N������ {} �������q�͏����Ȃ��̂ŁALN_PARSE_SUCCESS �͂�߂� runner.Success() �ŕԂ�����
			return LN_PARSE_SUCCESS(r);
		}

		static ParserResult<Array<Range>> Parse_File(ParserContext input)
		{
			LN_PARSE(r1, Parser<Range>(Parse_GlobalAnnotation));
			LN_PARSE(r3, Parser<Range>(Parse_Block));
			Array<Range> list{ r1, r3 };
			return LN_PARSE_SUCCESS(list);
		}
	};
};
#endif





void SamplerLinker::Parse(const ln::parser::TokenListPtr& tokenList)
{
#if 0
	{

		String input =
			"aa<aa>;"
			"f"
			"{"
			"  {"
			"  }"
			"}";


		ln::parser::CppLexer lex;
		ln::parser::DiagnosticsItemSet diag;
		ln::parser::TokenListPtr tokens = lex.Tokenize(input.c_str(), &diag);


		auto result = ParameterAnnotationParser::TokenParser::TryParse(
			ParameterAnnotationParser::TokenParser::Parser<Array<ParameterAnnotationParser::Range>>(ParameterAnnotationParser::TokenParser::Parse_File), tokens);

		printf("");

	}
#endif






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

	auto manyStmt = TokenParser::Many<TokenParser::Data>(TokenParser::Statement);
	auto result = TokenParser::TryParse(manyStmt, tokens);

	//
	//auto result = stmt.TryParse(tokens);

	printf("");
}



//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void SamplerLinker::analyzeSampler( D3DXHANDLE handle, const char* name )
{
	printf("�� %s �̌��� ----------------\n", name);
	// TODO �d���̔r��

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

						// �T���v���� index �ɃZ�b�g����Ă���e�N�X�`��
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
									//printf("�� %s <- %s\n", pair.SamplerVarName.c_str(), pair.TextureVarName.c_str() );
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


