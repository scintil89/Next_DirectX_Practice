///////////////////////////////////////////////
//	0420.
///////////////////////////////////////////////

cbuffer ConstantBuffer
{
	//	matrix    world;
	//	float4x4  view;
	float4x4 	wvp;
	float4x4	world;	//������ ������ ��������� ��ȯ�ϸ� �ȴ�.

	float4	lightDir;
	float4	lightColor;
};

struct VertexIn
{
	float3 pos:POSITION;
	float4 color:COLOR;

	float3 normal:NORMAL;
};

struct VertexOut
{
	float4 pos:SV_POSITION;
	float4 color:COLOR;

	float4 normal:NORMAL;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;

	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);

	vOut.color = vIn.color;

	//���� �������� �븻
	//vOut.normal = mul(float4(vIn.normal, 0.0f), world);

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	float4 finalColor = 0;

	//saturate : 0~1 ���̷� �����ִ� �Լ�
	//finalColor = saturate(dot((float3) - lightDir, vOut.normal) * //lightColor);
	//
	//finalColor.a = 1.0f;

	return float(1,0,0,0);
}

/*
if (vOut.pos.x < 300.0f)
return float4(1, 0, 1, 0);
else if (vOut.pos.x > 500.0f)
return float4(0, 1, 1, 0);

float width = 600.0f;
float height = 300.0f;

float red = -(vOut.pos.x / width - 1.0f) * 2;
float green = -(vOut.pos.y / height - 1.0f) * 2;
float blue	= 0.1f;

vOut.color = (red, green, blue, 1.0f);
*/