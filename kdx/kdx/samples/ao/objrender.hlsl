// basic shader to render an obj mesh with textures and material parameters
// matrices
cbuffer Matrices : register(b0)
{
	matrix model;
	matrix view;
	matrix proj;
};

cbuffer ObjMaterial : register(b1)
{
	float Ns : packoffset(c0);
	float Ni : packoffset(c0.y);
	float Tr : packoffset(c0.z);
	float3 Tf : packoffset(c1);
	uint illum : packoffset(c1.w);
	float3 Ka : packoffset(c2);
	float3 Kd : packoffset(c3);
	float3 Ks : packoffset(c4);
	float3 Ke : packoffset(c5);
};

// textures
Texture2D map_Ka : register(t0);
Texture2D map_Kd : register(t1);
Texture2D map_Ks : register(t2);
// samplers
SamplerState sampler_default : register(s0);

// vertex shader input
struct VSInput
{
	float4 pos : POSITION;
	float4 tex : TEXCOORD;
	float4 norm : NORMAL;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 tex : TEXCOORD;
};

PSInput VertexMain(VSInput input)
{
	PSInput output;
	
	input.pos.w = 1.0f;

	// multiply matrices
	output.pos = mul(input.pos, model);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, proj);

	//output.col = float4(input.norm.x, input.norm.y, input.norm.z, 1.0f);
	//output.col = map_Ka.Sample(sampler_Ka, input.tex.xy); // VS does not support Sample method
	output.tex = input.tex;

	return output;
}

float4 PixelMain(PSInput input) : SV_TARGET
{
	float4 final = float4(0, 0, 0, 0);
	final += float4(Ka.r, Ka.g, Ka.b, 1.0) * map_Ka.Sample(sampler_default, input.tex.xy);
	final += float4(Kd.r, Kd.g, Kd.b, 1.0) * map_Kd.Sample(sampler_default, input.tex.xy);
	final += float4(Ks.r, Ks.g, Ks.b, 1.0) * map_Ks.Sample(sampler_default, input.tex.xy);
	return final;
}
