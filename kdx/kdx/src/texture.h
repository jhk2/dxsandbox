#ifndef TEXTURE_H
#define TEXTURE_H

#include <D3D11.h>
#include "utils.h"

class Texture {
public:
	Texture(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename, bool mipmap = true);
	Texture(const Texture &other);
	virtual ~Texture();
	void use(ID3D11DeviceContext &devcon, UINT resourceSlot, UINT samplerSlot);
	void useResource(ID3D11DeviceContext &devcon, UINT slot); // if we want to use the same resource with a different sampler

private:
	bool init(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename);
	
	ID3D11ShaderResourceView *resource_;
	ID3D11SamplerState *sampler_;
};

#endif // TEXTURE_H