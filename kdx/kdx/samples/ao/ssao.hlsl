// matrices
cbuffer Matrices : register(b0)
{
	matrix proj;
};
// inverse of projection matrix used to render the original model
cbuffer InvCamPj : register(b1)
{
	matrix invCamPj;
};

// textures
Texture2D normal_map : register(t0);
Texture2D depth_map : register(t1);

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
	// reconstruct the view space position from the depth map
	float start_Z = depth_map.Sample(sampler_default, input.tex).r;
	float3 start_Pos = float3(input.tex.x, 1.0 - input.tex.y, start_Z);
	float3 ndc_Pos = (2.0 * start_Pos) - 1.0;
	// TODO: unproject has w of zero, that doesn't sound right
	float4 unproject = mul(float4(ndc_Pos.x, ndc_Pos.y, ndc_Pos.z, 1.0), invCamPj);
	float3 viewPos = unproject.xyz / unproject.w;

	float4 final = float4(0, 0, 0, 0);
	final = normal_map.Sample(sampler_default, input.tex);
	if (unproject.w > 0) {
		final = float4(unproject.w, unproject.w, unproject.w, 1.0);
	} else {
		final = float4(1.0, 1.0, 1.0, 1.0);
	}
	return final;
}
