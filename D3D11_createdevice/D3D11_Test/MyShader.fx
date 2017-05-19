///////////////////////////////////////////////
//	0420.
///////////////////////////////////////////////
Texture2D texDiffuse;			//texture
SamplerState samLinear;			//sample state	(filtering / mode)

cbuffer ConstantBuffer
{
	//	matrix    world;
	//	float4x4  view;
	float4x4 	wvp;
	float4x4	world;			//상자의 법선은 월드까지만 변환하면 된다.

	float4	lightDir;
	float4	lightColor;
}

struct VertexIn
{
	float3 pos:POSITION;
	float4 color:COLOR;

	float3 normal:NORMAL;

	float2 tex:TEXCOORD;
};

struct VertexOut
{
	float4 pos:SV_POSITION;
	float4 color:COLOR;

	float4 normal:NORMAL;

	float2 tex:TEXCOORD;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;

	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);

	vOut.color = vIn.color;

	//월드 공간산의 노말
	vOut.normal = mul(float4(vIn.normal, 0.0f), world);

	vOut.tex = vIn.tex;

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	float4 finalColor = 0;

	//saturate : 0~1 사이로 묶어주는 함수
	finalColor = saturate( ( (dot((float3) -lightDir, vOut.normal) * 0.5f) + 0.5f) * lightColor);

	float4 texColor = texDiffuse.Sample(samLinear, vOut.tex) * finalColor;
	texColor.a = 1.0f;

	return texColor;
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