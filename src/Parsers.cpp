
#include <memory>
#include "Common.h"
#include "Parsers.h"

SamplerLinker::SamplerLinker(Effect* effect)
	: m_effect(effect)
{
}

SamplerLinker::~SamplerLinker()
{
}

/*
	一番シンプル

		static Result<Data> Statement(ParserContext& parser)
		{
			auto t1 = parser.Eval(Token(ln::parser::CommonTokenType::Identifier));
			auto t2 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			auto t3 = parser.Eval(Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)));
			auto t4 = parser.Eval(Token(ln::parser::CommonTokenType::Operator));
			return parser.Success(Data{ t1.ToString(), t3.ToString() }, parser.GetPosition());
		}

	↑のをちゃんとした Parser を返すようにしたもの

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

	本当に関数型っぽくするならこうなる 1

		static Parser<Data> Statement(ParserContext& parser)
		{
			Parser<Token> p, t1, t2, t3, t4;
			p = (t1 = Token(ln::parser::CommonTokenType::Identifier))
			&&	(t2 = Token(ln::parser::CommonTokenType::Operator)))
			&&	(t3 = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			&&	(t4 = Token(ln::parser::CommonTokenType::Operator));
			return [p, t1, t3](ParserContext& parser) { return p(parser).Then( Data(t1.GetValue, t2.GetValue()) ) };
		}
	
	本当に関数型っぽくするならこうなる 2

		static Parser<Data> Statement(ParserContext& parser)	// 値渡しにするか、呼び出し元を DoParse とか1つかませないとだめ
		{
			return
				(parser[0] = Token(ln::parser::CommonTokenType::Identifier))
			|	(parser[1] = Token(ln::parser::CommonTokenType::Operator)))
			|	(parser[2] = Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parser_texture_variable)))
			|	(parser[3] = Token(ln::parser::CommonTokenType::Operator));
			->	[](ParserContext& parser) { return Data(parser[0].GetValue(), parser[1].GetValue()) };
		}
	


	一番シンプルなのをマクロ使うようにすると、例外longjmpしなくて済む

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
	if (result##_.IsFailed()) return input.Fail(); \
	input.Next(result##_); \
	auto result = result##_.GetValue();

#define LN_PARSE_RESULT(result, parser) \
	auto result = (parser).Call(input); \
	if (result.IsFailed()) return input.Fail(); \
	input.Next(result);

//#define LN_PARSE_SUCCESS(value)	\
//	Success(value, input);

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


// ParserResult の決まりごと
//	- 成否 (true/false) を持つ
//	- 値を持つ (失敗の場合は不正値)
//	- 次の読み取り位置を持つ (remainder)
// T はパーサ関数の戻す値
template<typename T, typename TCursor>
class GenericParserResult
{
public:
	static GenericParserResult<T, TCursor> Success(const T& value, int matchBegin, int matchEnd, const TCursor& remainder)
	{
		return GenericParserResult<T, TCursor>(value, matchBegin, matchEnd, remainder, true);
	}
	static GenericParserResult<T, TCursor> Fail(const TCursor& remainder)
	{
		return GenericParserResult<T, TCursor>(T(), 0, 0, remainder, false);
	}

	const T& GetValue() const { return m_value; }
	const TCursor& GetRemainder() const { return m_remainder; }	// 評価後の次の読み取り位置
	int GetRemainderPosition() const { return m_remainder.GetPosition(); }
	bool IsSucceed() const { return m_isSuccess; }
	bool IsFailed() const { return !m_isSuccess; }
	int GetMatchBegin() const { return m_matchBegin; }
	int GetMatchEnd() const { return m_matchEnd; }


	GenericParserResult(const detail::ParserFailure<TCursor>& failer)
		: m_value()
		, m_matchBegin(0)
		, m_matchEnd(0)
		, m_remainder(failer.remainder)
		, m_isSuccess(false)
	{
	}
	
private:
	GenericParserResult(const T& value, int matchBegin, int matchEnd, const TCursor& remainder, bool isSuccess)
		: m_value(value)
		, m_matchBegin(matchBegin)
		, m_matchEnd(matchEnd)
		, m_remainder(remainder)
		, m_isSuccess(isSuccess)
	{
	}

	T			m_value;
	int			m_matchBegin;
	int			m_matchEnd;
	TCursor		m_remainder;
	bool		m_isSuccess;
};

template<typename T>
class Option
{
public:
	// TODO: move

	static Option Some(const T& value) { return Option(value); }
	static Option None() { return Option(); }

	bool IsEmpty() const { return m_empty; }
	const T& GetValue() const { return m_value; }

private:
	Option(const T& value)
		: m_vale(value)
		, m_empty(false)
	{}
	Option()
		: m_vale()
		, m_empty(true)
	{}

	T		m_vale;
	bool	m_empty;
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
class GenericParserCursor
{
public:
	GenericParserCursor()
		: m_tokenList(nullptr)
		, m_position(0)
	{}

	// パース開始時の初期化用
	GenericParserCursor(const ln::parser::TokenList* tokenList)
		: m_tokenList(tokenList)
		, m_position(0)
	{
	}

	// パース開始時の初期化用
	GenericParserCursor(const ln::parser::TokenList* tokenList, int position)
		: m_tokenList(tokenList)
		, m_position(position)
	{
	}

	GenericParserCursor(const GenericParserCursor& obj)
		: m_tokenList(obj.m_tokenList)
		, m_position(obj.m_position)
	{
	}

	GenericParserCursor& operator=(const GenericParserCursor& obj)
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

	GenericParserCursor Cuing() const
	{
		TConditional cond;
		int pos = m_position;
		while (!cond(m_tokenList->GetAt(pos)))
		{
			++pos;
		};
		return GenericParserCursor(m_tokenList, pos);
	}

	GenericParserCursor Advance() const
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

		return GenericParserCursor(m_tokenList, pos);
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

	GenericParserContext(const GenericParserContext& obj)
		: m_start(obj.m_current)
		, m_current(obj.m_current)
	{
	}

	TCursor GetNext() const
	{
		return m_current.Advance();
		//m_current = m_current.Advance();
		//return m_current;
	}

	const parser::Token& GetCurrentValue() const
	{
		return m_current.GetCurrentValue();
	}

	const TCursor& GetStartCursor() const
	{
		return m_start;
	}

	int GetStartPosition() const
	{
		return m_start.GetPosition();
	}

	int GetLastMatchEndPosition() const
	{
		return m_last.GetPosition();
	}

	const TCursor& GetCurrentCursor() const
	{
		return m_current;
	}

	template<typename T>
	void Next(const GenericParserResult<T, TCursor>& result/*const TCursor& newPos*/)
	{
		m_last = m_current;
		m_current = result.GetRemainder();
	}

	template<typename T>
	GenericParserResult<T, TCursor> Success(const T& value)
	{
		return GenericParserResult<T, TCursor>::Success(value, GetStartPosition(), GetLastMatchEndPosition(), m_current);
	}

	detail::ParserFailure<TCursor> Fail()
	{
		detail::ParserFailure<TCursor> failer;
		failer.remainder = GetCurrentCursor();
		return failer;
	}

private:
	TCursor		m_start;
	TCursor		m_current;
	TCursor		m_last;
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

	GenericParser operator||(const GenericParser& second) const
	{
		return ParseLib<TCursor>::Or(*this, second);
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



	static Parser<ValueT> TokenChar(char ch)
	{
		return [ch](ParserContext input)
		{
			if (input.GetCurrentValue().EqualChar(ch))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.GetStartPosition(), input.GetStartPosition() + 1, input.GetNext());
			return ParserResult<ValueT>::Fail(input.GetCurrentCursor());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> TokenString(const char* str_)
	{
		String str = str_;
		return [str](ParserContext input)
		{
			if (input.GetCurrentValue().EqualString(str.c_str(), str.GetLength()))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.GetStartPosition(), input.GetStartPosition() + 1, input.GetNext());
			return ParserResult<ValueT>::Fail(input.GetCurrentCursor());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type)
	{
		return [type](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type)
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.GetStartPosition(), input.GetStartPosition() + 1, input.GetNext());
			return ParserResult<ValueT>::Fail(input.GetCurrentCursor());	// TODO: メッセージあるとよい
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, char ch)
	{
		return [type, ch](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type && input.GetCurrentValue().EqualChar(ch))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.GetStartPosition(), input.GetStartPosition() + 1, input.GetNext());
			return ParserResult<ValueT>::Fail(input.GetCurrentCursor());
		};
	}

	static Parser<ValueT> Token(ln::parser::CommonTokenType type, const char* string, int len)
	{
		return [type, string, len](ParserContext input)
		{
			if (input.GetCurrentValue().GetCommonType() == type && input.GetCurrentValue().EqualString(string, len))
				return ParserResult<ValueT>::Success(input.GetCurrentValue(), input.GetStartPosition(), input.GetStartPosition() + 1, input.GetNext());
			return ParserResult<ValueT>::Fail(input.GetCurrentCursor());
		};
	}

	// 0回以上の繰り返し
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
			return ParserResult<ln::Array<T>>::Success(list, input.GetStartPosition(), r.GetMatchEnd(), r.GetRemainder());
		};
	}

	template<typename T>
	static Parser<T> Or(const Parser<T>& first, const Parser<T>& second)
	{
		//Parser<T> second(second_);
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
	static Parser<Option<T>> Optional(const Parser<T>& parser)
	{
		return [parser](ParserContext input)
		{
			auto r = parser.Call(input);
			if (r.IsSucceed())
			{
				return ParserResult<Option<T>>::Success(Option<T>::Some(r.GetValue()), input.GetStartPosition(), r.GetMatchEnd(), r.GetRemainder());
			}
			return ParserResult<Option<T>>::Success(Option<T>::None(), input.GetStartPosition(), input.GetStartPosition(), input.GetStartCursor());
		};
	}

	// term までをマッチの範囲とし、
	// ターミネータを result に含む
	template<typename T>
	static Parser<ln::Array<T>> UntilMore(const Parser<T>& term)
	{
		return [term](ParserContext input)
		{
			ln::Array<T> list;
			auto r = term.Call(input);
			auto lastResult = r;

			while (r.IsFailed())
			{
				list.Add(r.GetValue());
				lastResult = r;
				r = term.Call(r.GetRemainder().Advance());
			}

			// TODO: ストリーム末尾までfailedだったらパース失敗

			return ParserResult<ln::Array<T>>::Success(list, input.GetStartPosition(), r.GetMatchEnd(), r.GetRemainder());
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
		// space 以外を許可
		return token.GetCommonType() != parser::CommonTokenType::SpaceSequence;
	}
};

using ParserCursor_SkipSpace = combinators::GenericParserCursor<ParserCursorCondition_SkipSpace>;


class TokenParser : public combinators::ParseLib<ParserCursor_SkipSpace>
{
public:

	struct Data
	{
		String	left;
		String	right;
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
		LN_PARSE(t3, Or(ParseLib::Token(ln::parser::CommonTokenType::Identifier), Parser<ln::parser::Token>(Parse_texture_variable)));
		LN_PARSE(t4, Token(ln::parser::CommonTokenType::Operator));
		return input.Success(Data{ t1.ToString(), t3.ToString() });
	}
};





namespace ParameterAnnotationParser
{
	enum class RangeType
	{
		Annotation,
		Block,
		SamplerStateBlock,
		TechniqueBlock,
		PassBlock,
	};

	struct Range
	{
		RangeType		type;
		int				headerBegin;
		int				begin;
		int				end;
		Array<Range>	childRanges;

		template<typename TFunc>
		void ForEach(TFunc func)
		{
			func(*this);
			for (auto& r : childRanges)
			{
				r.ForEach(func);
			}
		}
	};

	struct ParserCursorCondition_SkipSpace
	{
		bool operator()(const parser::Token& token)
		{
			return
				token.EqualChar('<') || token.EqualChar('>') || token.EqualChar('{') || token.EqualChar('}') ||
				token.EqualString("sampler_state", 13) ||
				token.EqualString("technique", 9) ||
				token.EqualString("pass", 4) ||
				token.IsEof();	// TODO: これが無くてもいいようにしたい。今はこれがないと、Many中にEOFしたときOutOfRangeする
		}
	};

	using ParserCursor_SkipSpace = combinators::GenericParserCursor<ParserCursorCondition_SkipSpace>;


	struct TokenParser : public combinators::ParseLib<ParserCursor_SkipSpace>
	{
		static ParserResult<Range> Parse_Annotation(ParserContext input)
		{
			LN_PARSE_RESULT(r1, TokenChar('<'));
			LN_PARSE_RESULT(r2, UntilMore(TokenChar('>')));
			return input.Success(Range{ RangeType::Annotation, 0, r1.GetMatchBegin(), r2.GetMatchEnd(), Array<Range>() });
		}

		static ParserResult<Range> Parse_sampler_state_block(ParserContext input)
		{
			LN_PARSE_RESULT(r1, TokenString("sampler_state"));
			LN_PARSE_RESULT(r2, TokenChar('{'));
			LN_PARSE_RESULT(r3, UntilMore(TokenChar('}')));
			return input.Success(Range{ RangeType::SamplerStateBlock, r1.GetMatchBegin(), r1.GetMatchBegin(), r3.GetMatchEnd(), Array<Range>() });
		}

		static ParserResult<Range> Parse_technique_block(ParserContext input)
		{
			LN_PARSE_RESULT(r1, TokenString("technique"));
			LN_PARSE_RESULT(r2, Optional(Parser<Range>(Parse_Annotation)));
			LN_PARSE_RESULT(r3, TokenChar('{'));
			LN_PARSE_RESULT(r4, Many<Range>(Parser<Range>(Parse_pass_block)));	// ネスト	TODO: できれば <Range> はやめたい
			LN_PARSE_RESULT(r5, TokenChar('}'));
			return input.Success(Range{ RangeType::TechniqueBlock, r1.GetMatchBegin(), r3.GetMatchBegin(), r5.GetMatchEnd(), r4.GetValue() });
		}

		static ParserResult<Range> Parse_pass_block(ParserContext input)
		{
			LN_PARSE_RESULT(r1, TokenString("pass"));
			LN_PARSE_RESULT(r2, Optional(Parser<Range>(Parse_Annotation)));
			LN_PARSE_RESULT(r3, TokenChar('{'));
			// この中に < > は無いはず
			LN_PARSE_RESULT(r5, TokenChar('}'));
			return input.Success(Range{ RangeType::PassBlock, r1.GetMatchBegin(), r3.GetMatchBegin(), r5.GetMatchEnd(), Array<Range>() });
		}

		static ParserResult<Range> Parse_Block(ParserContext input)
		{
			LN_PARSE_RESULT(r0, Optional(Many<parser::Token>(TokenChar('<') || TokenChar('>'))));
			LN_PARSE_RESULT(r1, TokenChar('{'));
			LN_PARSE_RESULT(r2, Many<Range>(Parser<Range>(Parse_Block)));	// ネスト	TODO: できれば <Range> はやめたい
			LN_PARSE_RESULT(r3, TokenChar('}'));
			return input.Success(Range{ RangeType::Block, 0, r1.GetMatchBegin(), r3.GetMatchEnd(), Array<Range>() });
		}

		static ParserResult<Array<Range>> Parse_File(ParserContext input)
		{
			LN_PARSE(r1, Many(Parser<Range>(Parse_Annotation) || Parser<Range>(Parse_sampler_state_block) || Parser<Range>(Parse_technique_block) || Parser<Range>(Parse_Block)));
			return input.Success(r1);
		}
	};
};





String SamplerLinker::Parse(ln::parser::TokenListPtr& tokenList)
{
	auto result = ParameterAnnotationParser::TokenParser::TryParse(
		ParameterAnnotationParser::TokenParser::Parser<Array<ParameterAnnotationParser::Range>>(ParameterAnnotationParser::TokenParser::Parse_File), tokenList);

	//Console::WriteLine(tokenList->GetAt(result.GetValue().GetAt(0).begin).ToString());
	//Console::WriteLine(tokenList->GetAt(result.GetValue().GetAt(0).end-1).ToString());
	//Console::WriteLine(tokenList->GetAt(result.GetValue().GetAt(1).begin).ToString());
	//Console::WriteLine(tokenList->GetAt(result.GetValue().GetAt(1).end-1).ToString());
	
	{
		//StreamWriter w("test2.txt");


		auto list = result.GetValue();
		for (auto& r : list)
		{
			r.ForEach([this, /*&w, */tokenList](const ParameterAnnotationParser::Range& r)
			{

				if (r.type == ParameterAnnotationParser::RangeType::SamplerStateBlock)
				{
					SamplerInfo info;
					info.samplerName = tokenList->GetAt(r.headerBegin - 4).ToString();	// 巻き戻して読むと名前

					// サンプラステートの読み取り
					String statusText = tokenList->ToString(r.begin + 1, r.end - 1);
					StringArray lines = statusText.Split(_T(";"), StringSplitOptions::RemoveEmptyEntries);
					for (String line : lines)
					{
						StringArray tokens = line.Split(_T("="), StringSplitOptions::RemoveEmptyEntries);
						if (tokens.GetCount() >= 2)
						{
							String l = tokens[0].Trim();
							String r = tokens[1].Trim();
							if (l == "texture")
							{
								info.textureName = r;
							}
							else
							{
								info.samplerStatus[l] = r;
							}
						}
					}

					m_effect->m_samplerInfoList.Add(info);
				}
				else if (r.type == ParameterAnnotationParser::RangeType::Annotation)
				{
					// Annotation の範囲を全て無効コードにする
					for (int i = r.begin; i < r.end; ++i)
					{
						parser::TokenList* tl = tokenList;
						tl->GetAt(i).SetValid(false);
					}
				}
				if (r.type == ParameterAnnotationParser::RangeType::TechniqueBlock)
				{
					TechniqueInfo info;
					info.name = tokenList->GetAt(r.headerBegin + 2).ToString();	// スペースを飛ばして読むと名前
					m_effect->m_techniqueInfoList.Add(info);
					//w.WriteLine("Technique {0}", info.name);

					// technique の範囲を全て無効コードにする
					for (int i = r.headerBegin; i < r.end; ++i)
					{
						parser::TokenList* tl = tokenList;
						tl->GetAt(i).SetValid(false);
					}
				}
				if (r.type == ParameterAnnotationParser::RangeType::PassBlock)
				{
					PassInfo info;
					info.name = tokenList->GetAt(r.headerBegin + 2).ToString();	// スペースを飛ばして読むと名前
					//w.WriteLine("  Pass {0}", info.name);

					// レンダーステートの読み取り
					String statusText = tokenList->ToString(r.begin + 1, r.end - 1);
					StringArray lines = statusText.Split(_T(";"), StringSplitOptions::RemoveEmptyEntries);
					for (String line : lines)
					{
						StringArray tokens = line.Split(_T("="), StringSplitOptions::RemoveEmptyEntries);
						if (tokens.GetCount() >= 2)
						{
							StateInfo si{ tokens[0].Trim(), tokens[1].Trim() };

							if (si.name == "VertexShader" || si.name == "PixelShader")
							{
								parser::CppLexer lex;
								parser::DiagnosticsItemSet diag;
								parser::TokenListPtr valueTokens = lex.Tokenize(si.value.c_str(), &diag);	// TODO: 範囲
								LN_THROW(!diag.HasError(), InvalidFormatException);

								if (si.name == "VertexShader")
								{
									info.vertexShader = valueTokens->GetAt(4).ToString();
								}
								else if (si.name == "PixelShader")
								{
									info.pixelShader = valueTokens->GetAt(4).ToString();
								}
							}
							else
							{
								info.status.Add(si);
							}

							//w.WriteLine("    {0}={1}", si.name, si.value);
						}
					}

					m_effect->m_techniqueInfoList.GetLast().passes.Add(info);

					// pass の範囲を全て無効コードにする
					for (int i = r.headerBegin; i < r.end; ++i)
					{
						parser::TokenList* tl = tokenList;
						tl->GetAt(i).SetValid(false);
					}
				}
			});
			

			//w.WriteLine("----");
			//w.WriteLine(tokenList->ToString(r.begin, r.end));
			//w.WriteLine("----");
		}
	}


	String out = tokenList->ToStringValidCode();
	out = out.Replace("shared", "");
	return out;
}


