
#include <set>
#include "Generator.h"

void Generator::Generate(Effect* effect, const PathName& output)
{
	m_effect = effect;

	StreamWriter writer(output);
	writer.WriteLine("#ifdef LN_FXC_METADATA");
	writer.WriteLine();

	JsonWriter json(&writer);

	json.WriteStartObject();

	// EntryPoint
	// まずは重複を排除
	std::set<String> entryPointsVS;
	std::set<String> entryPointsPS;
	for (auto& tech : effect->m_techniqueInfoList)
	{
		for (auto& pass : tech.passes)
		{
			if (!pass.vertexShader.IsEmpty())
			{
				entryPointsVS.insert(pass.vertexShader);
			}
			if (!pass.pixelShader.IsEmpty())
			{
				entryPointsPS.insert(pass.pixelShader);
			}
		}
	}
	// vertexShaders
	json.WritePropertyName("vertexShaders");
	json.WriteStartArray();
	for (auto& entry : entryPointsVS)
	{
		json.WriteString(entry.c_str());
	}
	json.WriteEndArray();

	// pixelShaders
	json.WritePropertyName("pixelShaders");
	json.WriteStartArray();
	for (auto& entry : entryPointsPS)
	{
		json.WriteString(entry.c_str());
	}
	json.WriteEndArray();

	// techniques
	json.WritePropertyName("techniques");
	json.WriteStartArray();
	for (auto& tech : effect->m_techniqueInfoList)
	{
		tech.Save(&json);
	}
	json.WriteEndArray();

	// parameters
	json.WritePropertyName("parameters");
	json.WriteStartArray();
	for (auto& param : effect->parameterList)
	{
		if (!param.isSampler)
		{
			param.Save(&json);
		}
	}
	json.WriteEndArray();

	json.WriteEndObject();

	writer.WriteLine();
	writer.WriteLine("#endif");


	
	// 次にシェーダコードを作る
	for (auto& e : entryPointsVS)
	{
		writer.WriteLine("#ifdef LN_GLSL_VERTEX_{0}", e);
		writer.WriteLine(Convert(effect->convertableCode, e, EShLangVertex));
		writer.WriteLine("#endif");
	}
	for (auto& e : entryPointsPS)
	{
		writer.WriteLine("#ifdef LN_GLSL_FRAGMENT_{0}", e);
		writer.WriteLine(Convert(effect->convertableCode, e, EShLangFragment));
		writer.WriteLine("#endif");
	}

	// オリジナルのコード
	writer.WriteLine("#ifdef LN_HLSL_DX9");
	writer.WriteLine(effect->preprocessedCode);
	writer.WriteLine();
	writer.WriteLine("#endif");

	
}

StringA Generator::Convert(const StringA& input, const StringA& entryPoint, EShLanguage codeType)
{
	StringA code = Hlsl2Glsl(input,entryPoint, codeType);

	// hlsl の sampler 型変数が、glsl の sampler 型変数に変換されているが、
	// hlsl の texture 型変数を使いたい。ここで全部置換してみる
	for (auto& param : m_effect->parameterList)
	{
		if (!param.samplerName.IsEmpty())
		{
			code = code.Replace(param.samplerName, param.name);
		}
	}

	return code;
}

StringA Generator::Hlsl2Glsl(const StringA& input, const StringA& entryPoint, EShLanguage codeType)
{
	ShHandle shParser = nullptr;
	StringA output;

	if (Hlsl2Glsl_Initialize() == 0) {
		LN_THROW(0, InvalidOperationException, "failed Hlsl2Glsl_Initialize.");
	}

	try
	{


		shParser = Hlsl2Glsl_ConstructCompiler(codeType);



		static EAttribSemantic kAttribSemantic[50] = {
			EAttrSemPosition,
			EAttrSemPosition1,
			EAttrSemPosition2,
			EAttrSemPosition3,
			EAttrSemVPos,
			EAttrSemVFace,
			EAttrSemNormal,
			EAttrSemNormal1,
			EAttrSemNormal2,
			EAttrSemNormal3,
			EAttrSemColor0,
			EAttrSemColor1,
			EAttrSemColor2,
			EAttrSemColor3,
			EAttrSemTex0,
			EAttrSemTex1,
			EAttrSemTex2,
			EAttrSemTex3,
			EAttrSemTex4,
			EAttrSemTex5,
			EAttrSemTex6,
			EAttrSemTex7,
			EAttrSemTex8,
			EAttrSemTex9,
			EAttrSemTangent,
			EAttrSemTangent1,
			EAttrSemTangent2,
			EAttrSemTangent3,
			EAttrSemBinormal,
			EAttrSemBinormal1,
			EAttrSemBinormal2,
			EAttrSemBinormal3,
			EAttrSemBlendWeight,
			EAttrSemBlendWeight1,
			EAttrSemBlendWeight2,
			EAttrSemBlendWeight3,
			EAttrSemBlendIndices,
			EAttrSemBlendIndices1,
			EAttrSemBlendIndices2,
			EAttrSemBlendIndices3,
			EAttrSemPSize,
			EAttrSemPSize1,
			EAttrSemPSize2,
			EAttrSemPSize3,
			EAttrSemDepth,
			//EAttrSemUnknown,
			//EAttrSemVertexID,
			//EAttrSemInstanceID,
			//EAttrSemPrimitiveID,
			//EAttrSemCoverage,
		};
		static const char* kAttribString[50] = {	// TODO: このへん、使える attr かどうかチェックしたい
			"ln_Vertex", //EAttrSemPosition,
			"ln_Vertex1", //EAttrSemPosition1,
			"ln_Vertex2", //EAttrSemPosition2,
			"ln_Vertex3", //EAttrSemPosition3,
			"ln_VPos",	//EAttrSemVPos,
			"ln_VFace", //EAttrSemVFace,
			"ln_Normal",	//EAttrSemNormal,
			"ln_Normal1",	//EAttrSemNormal1,
			"ln_Normal2",	//EAttrSemNormal2,
			"ln_Normal3",	//EAttrSemNormal3,
			"ln_Color0",	//EAttrSemColor0,
			"ln_Color1",	//EAttrSemColor1,
			"ln_Color2",	//EAttrSemColor2,
			"ln_Color3",	//EAttrSemColor3,
			"ln_MultiTexCoord0",	//EAttrSemTex0,
			"ln_MultiTexCoord1",	//EAttrSemTex1,
			"ln_MultiTexCoord2",	//EAttrSemTex2,
			"ln_MultiTexCoord3",	//EAttrSemTex3,
			"ln_MultiTexCoord4",	//EAttrSemTex4,
			"ln_MultiTexCoord5",	//EAttrSemTex5,
			"ln_MultiTexCoord6",	//EAttrSemTex6,
			"ln_MultiTexCoord7",	//EAttrSemTex7,
			"ln_MultiTexCoord8",	//EAttrSemTex8,
			"ln_MultiTexCoord9",	//EAttrSemTex9,
			"ln_Tangent",	//EAttrSemTangent,
			"ln_Tangent1",	//EAttrSemTangent1,
			"ln_Tangent2",	//EAttrSemTangent2,
			"ln_Tangent3",	//EAttrSemTangent3,
			"ln_Binormal",	//EAttrSemBinormal,
			"ln_Binormal1",	//EAttrSemBinormal1,
			"ln_Binormal2",	//EAttrSemBinormal2,
			"ln_Binormal3",	//EAttrSemBinormal3,
			"ln_BlendWeight",	//EAttrSemBlendWeight,
			"ln_BlendWeight1",	//EAttrSemBlendWeight1,
			"ln_BlendWeight2",	//EAttrSemBlendWeight2,
			"ln_BlendWeight3",	//EAttrSemBlendWeight3,
			"ln_BlendIndices",	//EAttrSemBlendIndices,
			"ln_BlendIndices1",	//EAttrSemBlendIndices1,
			"ln_BlendIndices2",	//EAttrSemBlendIndices2,
			"ln_BlendIndices3",	//EAttrSemBlendIndices3,
			"ln_PSize",	//EAttrSemPSize,
			"ln_PSize1",	//EAttrSemPSize1,
			"ln_PSize2",	//EAttrSemPSize2,
			"ln_PSize3",	//EAttrSemPSize3,
			"ln_Depth",	//EAttrSemDepth,
			//"ln_Unknown",	//EAttrSemUnknown,
			//"ln_VertexId",	//EAttrSemVertexID,
			//"ln_InstanceId",	//EAttrSemInstanceID,
			//"ln_PrimitiveId",	//EAttrSemPrimitiveID,
			//"ln_Coverage",	//EAttrSemCoverage,
		};

		int setUserAttrOk = Hlsl2Glsl_SetUserAttributeNames(shParser, kAttribSemantic, kAttribString, 45);
		LN_THROW(setUserAttrOk != 0, InvalidFormatException, "Hlsl2Glsl_SetUserAttributeNames fail.");


		unsigned options = 0;
		if (false)
			options |= ETranslateOpIntermediate;
		ETargetVersion version = ETargetGLSL_120;//ETargetGLSL_ES_300;//
		int parseOk = Hlsl2Glsl_Parse(shParser, input.c_str(), version, nullptr, options);
		printf(Hlsl2Glsl_GetInfoLog(shParser));
		LN_THROW(parseOk != 0, InvalidFormatException, "Hlsl2Glsl_Parse fail.");

		int translateOk = Hlsl2Glsl_Translate(shParser, entryPoint.c_str(), version, options);
		LN_THROW(translateOk != 0, InvalidFormatException, "Hlsl2Glsl_Translate fail.");

		output = Hlsl2Glsl_GetShader(shParser);


		Hlsl2Glsl_DestructCompiler(shParser);
		Hlsl2Glsl_Shutdown();
	}
	catch (...)
	{
		if (shParser != 0)
		{
			Hlsl2Glsl_DestructCompiler(shParser);
		}
		Hlsl2Glsl_Shutdown();
		throw;
	}

	return output;
}
