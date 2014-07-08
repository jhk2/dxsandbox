#include "sampler.h"
#include <map>
#include <assert.h>

// reference counter for resource management
static std::map<ID3D11SamplerState*, unsigned short> RefCount;

Sampler::Sampler(ID3D11Device &dev, D3D11_SAMPLER_DESC &desc) : sampler_(0)
{
	if (init(dev, desc)) {
		assert(RefCount.count(sampler_) == 0);
		RefCount[sampler_] = 1;
		assert(RefCount[sampler_] == 1);
		assert(RefCount.count(sampler_) == 1);
	}
}

Sampler::Sampler(const Sampler &other)
{
	assert(RefCount.count(sampler_) == 1);
	RefCount[sampler_]++;
}

Sampler::~Sampler()
{
	assert(RefCount.count(sampler_) == 1);
	assert(RefCount[sampler_] > 0);
	if (--RefCount[sampler_] == 0) {
		sampler_->Release();
	}
}

void Sampler::use(ID3D11DeviceContext &devcon, UINT slot)
{
	devcon.PSSetSamplers(slot, 1, &sampler_);
}

bool Sampler::init(ID3D11Device &dev, D3D11_SAMPLER_DESC &desc)
{
	HRESULT result = dev.CreateSamplerState(&desc, &sampler_);
	return !FAILED(result);
}

static Sampler *DefaultSampler = 0;

/*static*/ Sampler& Sampler::GetDefaultSampler(ID3D11Device &dev)
{
	if (!DefaultSampler) {
		D3D11_SAMPLER_DESC samplerdesc;
		ZeroMemory(&samplerdesc, sizeof(D3D11_SAMPLER_DESC));
		samplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerdesc.MinLOD = 0;
		samplerdesc.MaxLOD = D3D11_FLOAT32_MAX;

		DefaultSampler = new Sampler(dev, samplerdesc);
	}
	return *DefaultSampler;
}