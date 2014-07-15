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

static const float3 taps[16] = { float3(-0.364452, -0.014985, -0.513535),
                        float3(0.004669, -0.445692, -0.165899),
                        float3(0.607166, -0.571184, 0.377880),
                        float3(-0.607685, -0.352123, -0.663045),
                        float3(-0.235328, -0.142338, 0.925718),
                        float3(-0.023743, -0.297281, -0.392438),
                        float3(0.918790, 0.056215, 0.092624),
                        float3(0.608966, -0.385235, -0.108280),
                        float3(-0.802881, 0.225105, 0.361339),
                        float3(-0.070376, 0.303049, -0.905118),
                        float3(-0.503922, -0.475265, 0.177892),
                        float3(0.035096, -0.367809, -0.475295),
                        float3(-0.316874, -0.374981, -0.345988),
                        float3(-0.567278, -0.297800, -0.271889),
                        float3(-0.123325, 0.197851, 0.626759),
                        float3(0.852626, -0.061007, -0.144475) };

#define TAP_SIZE 0.02
#define NUM_TAPS 16
#define THRESHOLD 0.1
#define SCALE 1.0

float4 PixelMain(PSInput input) : SV_TARGET
{
	// reconstruct the view space position from the depth map
	float start_Z = depth_map.Sample(sampler_default, input.tex).r;
	float3 start_Pos = float3(input.tex.x, 1.0 - input.tex.y, start_Z);
	float3 ndc_Pos = (2.0 * start_Pos) - 1.0;
	// TODO: unproject has w of zero, that doesn't sound right
	float4 unproject = mul(float4(ndc_Pos.x, ndc_Pos.y, ndc_Pos.z, 1.0), invCamPj);
	float3 viewPos = unproject.xyz / unproject.w;
	float3 viewNorm = normal_map.Sample(sampler_default, input.tex).xyz;

	/*
	if (viewPos.z > 0) {
		return float4(0, 1.0, 0, 1.0);
	} else {
		return float4(1.0, 0, 0, 1.0);
	}
	*/
	//return float4(unproject.x, unproject.y, unproject.z, 1.0);

	float total = 0.0;
	for (uint i = 0; i < NUM_TAPS; i++) {
		float3 offset = TAP_SIZE * taps[i];
		float2 offTex = input.tex + offset.xy;

		float off_start_Z = depth_map.Sample(sampler_default, offTex).r;
		float3 off_start_Pos = float3(offTex.x, offTex.y, off_start_Z);
		float3 off_ndc_Pos = (2.0 * off_start_Pos) - 1.0;
		float4 off_unproject = mul(float4(off_ndc_Pos.x, off_ndc_Pos.y, off_ndc_Pos.z, 1.0), invCamPj);
		float3 off_viewPos = off_unproject.xyz / off_unproject.w;
		//return float4(off_viewPos.x, off_viewPos.y, off_viewPos.z, 1.0);
		/*
		if (off_unproject.w > 1000) {
			return float4(0, 1.0, 0, 1.0);
		} else {
			return float4(1.0, 0, 0, 1.0);
		}
		*/
		
		float3 diff = off_viewPos.xyz - viewPos.xyz;
		float distance = length(diff);
		float3 diffnorm = normalize(diff);

		float occlusion = max(0.0, dot(viewNorm, diffnorm)) * SCALE / (1.0 + distance);
		total += (1.0 - occlusion);
	}

	total /= NUM_TAPS;
	return float4(total, total, total, 1.0);

	/*
	float4 final = float4(0, 0, 0, 0);
	final = normal_map.Sample(sampler_default, input.tex);
	if (unproject.w > 0) {
		final = float4(unproject.w, unproject.w, unproject.w, 1.0);
	} else {
		final = float4(1.0, 0.0, 1.0, 1.0);
	}
	return final;
	*/
}
