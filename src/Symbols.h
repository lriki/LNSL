
#pragma once
#include <map>

struct AnnotationInfo
{
	String		type;
	String		name;
	String		value;

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		json->WritePropertyName("type");
		json->WriteString(type.c_str());
		json->WritePropertyName("name");
		json->WriteString(name.c_str());
		json->WritePropertyName("value");
		json->WriteString(value.c_str());
		json->WriteEndObject();
	}
};

struct SamplerInfo
{
	String		samplerName;
	String		textureName;
	std::map<String, String>	samplerStatus;

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		for (auto& pair : samplerStatus)
		{
			json->WritePropertyName(pair.first.c_str());
			json->WriteString(pair.second.c_str());
		}
		json->WriteEndObject();
	}
};


struct ParameterInfo
{
	String		name;
	String		semantic;
	String		samplerName;	// �����L�[
	bool		shared;
	SamplerInfo	samplerInfo;
	Array<AnnotationInfo>	annotations;
	/*
		hlsl2glsl �� sampler �^���o�͂��Atexture �^�͏o�͂��Ȃ��B
		HLSL �Ɠ����悤�Ȋ��o�Ń��[�U�[�Ɏg�킹�����Ƃ���ƁAShader::SetTexture() �ŃT���v�������w�肷��͕̂s���R�B
		ParameterInfo::name �� texture �ϐ��B
		ParameterInfo::samplerName �� sampler �ϐ��B
	*/

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		json->WritePropertyName("name");
		json->WriteString(name.c_str());
		if (!semantic.IsEmpty())
		{
			json->WritePropertyName("semantic");
			json->WriteString(semantic.c_str());
		}
		if (!samplerName.IsEmpty())
		{
			json->WritePropertyName("samplerName");
			json->WriteString(samplerName.c_str());
		}
		if (shared)
		{
			json->WritePropertyName("shared");
			json->WriteBool(true);
		}
		if (!samplerName.IsEmpty())
		{
			json->WritePropertyName("samplerStatus");
			samplerInfo.Save(json);
		}
		if (!annotations.IsEmpty())
		{
			json->WritePropertyName("annotations");
			json->WriteStartArray();
			for (auto& anno : annotations)
			{
				anno.Save(json);
			}
			json->WriteEndArray();
		}
		json->WriteEndObject();
	}
};
struct StateInfo
{
	String name;
	String value;

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		json->WritePropertyName("name");
		json->WriteString(name.c_str());
		json->WritePropertyName("value");
		json->WriteString(value.c_str());
		json->WriteEndObject();
	}
};

struct PassInfo
{
	String name;
	String vertexShader;
	String pixelShader;
	Array<StateInfo>	status;
	Array<AnnotationInfo>	annotations;

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		json->WritePropertyName("name");
		json->WriteString(name.c_str());
		json->WritePropertyName("vertexShader");
		json->WriteString(vertexShader.c_str());
		json->WritePropertyName("pixelShader");
		json->WriteString(pixelShader.c_str());
		if (!status.IsEmpty())
		{
			json->WritePropertyName("status");
			json->WriteStartArray();
			for (auto& state : status)
			{
				state.Save(json);
			}
			json->WriteEndArray();
		}
		if (!annotations.IsEmpty())
		{
			json->WritePropertyName("annotations");
			json->WriteStartArray();
			for (auto& anno : annotations)
			{
				anno.Save(json);
			}
			json->WriteEndArray();
		}
		json->WriteEndObject();
	}
};

struct TechniqueInfo
{
	String name;
	Array<PassInfo>	passes;
	Array<AnnotationInfo>	annotations;

	PassInfo* GetPassInfo(const char* name)
	{
		PassInfo* pass = passes.Find([name](const  PassInfo& info) {return info.name == name; });
		LN_THROW(pass != nullptr, InvalidOperationException);
		return pass;
	}

	void Save(JsonWriter* json)
	{
		json->WriteStartObject();
		json->WritePropertyName("name");
		json->WriteString(name.c_str());
		if (!passes.IsEmpty())
		{
			json->WritePropertyName("passes");
			json->WriteStartArray();
			for (auto& pass : passes)
			{
				pass.Save(json);
			}
			json->WriteEndArray();
		}
		if (!annotations.IsEmpty())
		{
			json->WritePropertyName("annotations");
			json->WriteStartArray();
			for (auto& anno : annotations)
			{
				anno.Save(json);
			}
			json->WriteEndArray();
		}
		json->WriteEndObject();
	}
};

class Effect
{
public:
	Array<ParameterInfo>	parameterList;
	Array<SamplerInfo>		m_samplerInfoList;
	Array<TechniqueInfo>	m_techniqueInfoList;
	String					convertableCode;

	TechniqueInfo* GetTechniqueInfo(const char* name)
	{
		TechniqueInfo* tech = m_techniqueInfoList.Find([name](const  TechniqueInfo& info) {return info.name == name; });
		LN_THROW(tech != nullptr, InvalidOperationException);
		return tech;
	}

	SamplerInfo* GetSamplerInfo(const char* name)
	{
		SamplerInfo* s = m_samplerInfoList.Find([name](const  SamplerInfo& info) {return info.samplerName == name; });
		LN_THROW(s != nullptr, InvalidOperationException);
		return s;
	}

private:
};
