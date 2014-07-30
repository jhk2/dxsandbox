// matrices
cbuffer Matrices : register(b0)
{
	matrix proj;
};

// textures
Texture2D source_texture : register(t0);

// samplers
SamplerState sampler_default : register(s0);

// vertex shader input
struct VSInput
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

PSInput VertexMain(VSInput input)
{
	PSInput output;
	
	input.pos.w = 1.0f;
	output.pos = mul(input.pos, proj);
	output.tex = input.tex;

	return output;
}

float4 PixelMain(PSInput input) : SV_TARGET
{
	return source_texture.Sample(sampler_default, input.tex);
}