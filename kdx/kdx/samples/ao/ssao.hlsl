// matrices
cbuffer Matrices
{
	matrix model;
	matrix view;
	matrix proj;
};

// vertex shader input
struct VSInput
{
	float4 pos : POSITION;
	float4 tex : TEXTURE;
	float4 norm : NORMAL;
};
/*
struct PSInput
{
	float4 posn : SV_POSITION;
	float4 col : COLOR;
};

PSInput VertexMain(VSInput input)
{
	PSInput output;
	
	input.pos.w = 1.0f;

	// multiply matrices
	output.pos = mul(input.pos, model);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.posn, proj);

	output.col = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return output;
}

PSInput PixelMain(PSInput input) : SV_TARGET
{
	return input.col;
}
*/


struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VOut VertexMain(VSInput input)
{
	float4 position = input.pos;
    VOut output;
	//position.w = 1.0f;
    //output.position = position;
	output.position = mul(position, model);
	output.position = mul(position, view);
	output.position = mul(output.position, proj);
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	
	//matrix projcopy = proj;
	//float4 multed = mul(position, projcopy);
	//output.color.r = multed.w;
	//output.color.g = 1.0f;

    return output;
}


float4 PixelMain(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
    return color;
}
