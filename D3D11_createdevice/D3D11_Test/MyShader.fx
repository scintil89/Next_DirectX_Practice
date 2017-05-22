///////////////////////////////////////////////
//	0406.
///////////////////////////////////////////////

float4   VS(float4     pos : POSITION) : SV_POSITION
{
	return     pos;
}

float4   PS(float4    pos  : SV_POSITION) : SV_TARGET
{	
	//H.W. Gradation
	float width = 800.0f;
	float height = 400.0f;
	
	float red = (pos.x / width ) * 1.f;
	float green = -(pos.y / height - 1.0f) * 1.0f;
	float blue = 0.f;


	return float4(red, green, blue, 1.0f);

	//return    float4(0.0f, 1.0f, 0.0f, 1.0f);
}
