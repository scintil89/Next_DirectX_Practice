Texture2D   texDiffuse;       // texture
SamplerState   samLinear;   // sample State ( filtering/mode )

cbuffer ConstantBuffer
{
	float4x4 wvp;
}

struct VertexIn
{
	float3 pos : POSITION;
	float4 color : COLOR;

	float2 tex : TEXCOORD;
};

struct VertexOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;

	float2 tex : TEXCOORD;
};


VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;
	
	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);
	vOut.color = vIn.color;

	vOut.tex = vIn.tex;

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	float4     texColor = texDiffuse.Sample(samLinear, vOut.tex);

	return  texColor;
}
