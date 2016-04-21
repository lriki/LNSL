
#pragma once
#include <map>

struct AnnotationInfo
{
	String		type;
	String		name;
	String		value;
};

struct ParameterInfo
{
	String		name;
	String		semantic;
	String		samplerName;
	Array<AnnotationInfo>	annotations;
	bool		shared;

	/*
		hlsl2glsl は sampler 型を出力し、texture 型は出力しない。
		HLSL と同じような感覚でユーザーに使わせたいとすると、Shader::SetTexture() でサンプラ名を指定するのは不自然。
		ParameterInfo::name は texture 変数。
		ParameterInfo::samplerName は sampler 変数。
	*/
};

struct SamplerInfo
{
	String		samplerName;
	String		textureName;
	std::map<String, String>	samplerStatus;
};

struct StateInfo
{
	String name;
	String value;
};

struct PassInfo
{
	String name;
	String vertexShader;
	String pixelShader;
	Array<StateInfo>	status;
};

struct TechniqueInfo
{
	String name;
	Array<PassInfo>	passes;
};

class Effect
{
public:
	Array<ParameterInfo>	parameterList;
	Array<SamplerInfo>		m_samplerInfoList;
	Array<TechniqueInfo>	m_techniqueInfoList;
	String					convertableCode;

private:
};
