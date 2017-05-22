
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
	float4	normal:NORMAL;
};

cbuffer ConstantBuffer
{
	float4x4 	wvp;
	float4x4	world;
	float4		lightDir;
	float4		lightColor;
}

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;

	vOut.pos = mul(float4(vIn.pos, 1.0f), wvp);
	vOut.color = vIn.color;

	vOut.normal = mul(float4(vIn.normal, 0.0f), world);

	return vOut;
}

float4 PS(VertexOut vOut) : SV_TARGET
{
	float4 finalColor = 0;

	finalColor = (dot(-lightDir, vOut.normal)  * 0.5 + 0.5) * lightColor; //half lambert
	//finalColor = saturate(dot(-lightDir, vOut.normal) * lightColor); //lambert
	finalColor.a = 1.0f;

	//float4 texColor = texDiffuse.Sample(samLinear, vOut.tex) * finalColor;
	//texColor.a = 1.0f;

	return  finalColor;
}
