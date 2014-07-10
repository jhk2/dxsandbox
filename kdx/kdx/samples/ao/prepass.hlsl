// prepass shader for generating depth and normal buffers for ssao

// matrices
cbuffer Matrices : register(b0)
{
	matrix model;
	matrix view;
	matrix proj;
};

// textures

// samplers
SamplerState sampler_default : register(s0);

// vertex shader input
struct VSInput
{
	float4 pos : POSITION;
	float4 tex : TEXCOORD;
	float3 norm : NORMAL;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
};

PSInput VertexMain(VSInput input)
{
	PSInput output;
	
	input.pos.w = 1.0f;
	output.pos = mul(input.pos, model);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, proj);
	output.norm = input.norm;

	return output;
}

float3 PixelMain(PSInput input) : SV_TARGET
{
	float3 final = input.norm.xyz;
	return final;
}
