
#include "Common.h"
//#include "Utils.h"
#include "DXDevice.h"
#include "Loader.h"
#include "Generator.h"

//
//#include <fstream>
/*
�����c
�EVertexShader �^�͎g�p�s�B(hsls2glslfork ���Ή����Ă��Ȃ�)
�Epass �� VertexShader �ɕϐ��͐ݒ�ł��Ȃ��B
��)VertexShader = (vsArray[CurNumBones]);
�E�A�m�e�[�V�����O�̕�����͋֎~
(��������� shared ��������\��������)

HLSL ����\��
http://msdn.microsoft.com/ja-jp/library/bb509615%28v=vs.85%29.aspx



*/

int main(int argc, char* argv[])
{
	Effect effect;

	Loader loader;
	//loader.Load(argv[1]);
	loader.Load(&effect, "D:/Proj/LNSL/src/test/MirrorWater_low.fxm");

	Generator gen;
	gen.Generate(&effect, "D:/Proj/LNSL/src/test/MirrorWater_low.fxc");

	//StringA code = Generator::Convert(loader.GetPreprocessedHLSLCode(), "VS_Standard", EShLangVertex);

#if 0
	std::string shaderSource;
	std::ofstream ofs("shader.txt");
	ofs << "test";


	if (argc < 2)
	{
		return 1;
	}

	DXDevice device;
	if (!device.initlaize())
	{
		printf("failed initialize DirectX.\n");
		device.finalize();
		return 1;
	}

	//Converter converter;
	//converter.convert( "D:/Proj/LightNote/Source/Core/Resource/Shader/SSBasic3D.fx", &device );

	/*if ( Hlsl2Glsl_Initialize() == 0 ) {
	printf( "failed Hlsl2Glsl_Initialize.\n" );
	return 1;
	}*/



	//converter.convert( "SSBasic2D.fx", &device );
	//converter.convert( "SSBasic3D.fx", &device );
	//converter.convert( "MirrorWater_low.fxm", &device );
	//converter.convert( "full.fx", &device );

	for (int i = 1; i < argc; ++i)
	{
		Converter converter;
		converter.convert(argv[i], &device);
	}

	//Hlsl2Glsl_Shutdown();
	device.finalize();
#endif
	return 0;
}

