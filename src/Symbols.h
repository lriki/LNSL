
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
		hlsl2glsl �� sampler �^���o�͂��Atexture �^�͏o�͂��Ȃ��B
		HLSL �Ɠ����悤�Ȋ��o�Ń��[�U�[�Ɏg�킹�����Ƃ���ƁAShader::SetTexture() �ŃT���v�������w�肷��͕̂s���R�B
		ParameterInfo::name �� texture �ϐ��B
		ParameterInfo::samplerName �� sampler �ϐ��B
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
