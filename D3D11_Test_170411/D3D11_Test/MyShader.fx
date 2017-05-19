///////////////////////////////////////////////
//	070411. texture
///////////////////////////////////////////////

Texture2D texDiffuse; //texture
SamplerState samLinear; //sample state

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
	float4 color:COLOR0;

	float4 normal:NORMAL;

	float2 tex:TEXCOORD;
};

cbuffer ConstantBuffer
{
	//	matrix    world;
	//	float4x4  view;

	float4x4 	wvp;
	float4x4	world;

	float4 lightDir;
	float4 lightColor;
}

VertexOut VS(VertexIn vIn)
{
	VertexOut	vOut;

	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);

	vOut.color = vIn.color;

	vOut.normal = mul(float4(vIn.normal, 0.0f), world);

	vOut.tex = vIn.tex;

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	float4 finalColor = 0;
	
	finalColor = (dot(-lightDir, vOut.normal)  * 0.5 + 0.5) * lightColor; //half lambert
	//finalColor = saturate(dot(-lightDir, vOut.normal) * lightColor); //lambert
	//finalColor.a = 1.0f;

	float4 texColor = texDiffuse.Sample(samLinear, vOut.tex) * finalColor;
	texColor.a = 1.0f;

	return  texColor;
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

/*
///////////////////////////////////////////////
//	0411.
///////////////////////////////////////////////

struct VertexIn
{
	float3 pos:POSITION;
	float4 color:COLOR;

	float3 normal:NORMAL;
};

struct VertexOut
{
	float4 pos:SV_POSITION;
	float4 color:COLOR0;

	float4 normal:NORMAL;
};

cbuffer ConstantBuffer
{
	//	matrix    world;
	//	float4x4  view;

	float4x4 	wvp;
	float4x4	world;

	float4 lightDir;
	float4 lightColor;
}

VertexOut VS(VertexIn vIn)
{
	VertexOut	vOut;

	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);

	vOut.color = vIn.color;

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	return vOut.color;
}

/**
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
***/

