#ifndef SAMPLER_H
#define SAMPLER_H

#include <D3D11.h>

class Sampler {
public:
	Sampler(ID3D11Device &dev, D3D11_SAMPLER_DESC &desc);
	Sampler(const Sampler &other);
	virtual ~Sampler();

	void use(ID3D11DeviceContext &devcon, UINT slot);
private:
	bool init(ID3D11Device &dev, D3D11_SAMPLER_DESC &desc);

	ID3D11SamplerState *sampler_;

public:
	static Sampler& GetDefaultSampler(ID3D11Device &dev);
};
#endif // SAMPLER_H