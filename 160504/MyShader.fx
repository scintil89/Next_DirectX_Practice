///////////////////////////////////////////////
//	0420.
///////////////////////////////////////////////

struct VertexIn
{
	float3 pos:POSITION;
	float4 color:COLOR;
};

struct VertexOut
{
	float4 pos:SV_POSITION;
	float4 color:COLOR;
};

cbuffer ConstantBuffer
{
	//	matrix    world;
	//	float4x4  view;

	float4x4 	wvp;
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