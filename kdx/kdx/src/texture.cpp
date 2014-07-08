#include "texture.h"
#include <map>
#include <assert.h>
#include <D3DX11.h>

// reference counter for resource management
static std::map<ID3D11ShaderResourceView*, unsigned short> RefCount;

Texture::Texture(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename, bool mipmap) : resource_(0)
{
	if (init(dev, devcon, filename)) {
		//printf("successfully loaded texture %s\n", filename); fflush(stdout);
		assert(RefCount.count(resource_) == 0);
		RefCount[resource_] = 1;
		assert(RefCount[resource_] == 1);
		assert(RefCount.count(resource_) == 1);
		if (mipmap) {
			devcon.GenerateMips(resource_);
		}
	} else {
        printf("failed to load texture %s\n", filename); fflush(stdout);
		assert(0);
	}
}

Texture::Texture(const Texture &other) : resource_(other.resource_)
{
	assert(RefCount.count(resource_) == 1);
	RefCount[resource_]++;
}

Texture::~Texture()
{
	assert(RefCount.count(resource_) == 1);
	assert(RefCount[resource_] > 0);
	if (--RefCount[resource_] == 0) {
		resource_->Release();
	}
}

void Texture::use(ID3D11DeviceContext &devcon, UINT slot)
{
	devcon.PSSetShaderResources(slot, 1, &resource_);
}

bool Texture::init(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename)
{
	HRESULT result = D3DX11CreateShaderResourceViewFromFile(&dev, filename, NULL, NULL, &resource_, NULL);
	return !FAILED(result);
}