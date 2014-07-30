#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <D3D11.h>
#include <vector>

class DxBase;
struct D3DXCOLOR;

// initialization parameters
struct FramebufferParams {
	UINT width, height;
	UINT numSamples;
	UINT numMrts;
	bool depthEnable;
	std::vector<DXGI_FORMAT> colorFormats;
	DXGI_FORMAT depthFormat;
};

class Framebuffer {
public:
	Framebuffer(ID3D11Device &dev, FramebufferParams &params);
	virtual ~Framebuffer();

	// for binding as an output framebuffer
	void use(ID3D11DeviceContext &devcon);
	// for binding as an input shader resource
	void useColorResources(ID3D11DeviceContext &devcon, UINT slot, UINT count);
	void useDepthResource(ID3D11DeviceContext &devcon, UINT slot);
	// for binding as an unordered access view
	void useColorUAVs(ID3D11DeviceContext &devcon, UINT slot, UINT count);

	// WORKNOTE: Blitting using CopyResource is much more limited than glBlitFramebuffer
	// in terms of compatibility in formats between the source and destination buffers
	// copying to another framebuffer
	void blit(ID3D11DeviceContext &devcon, Framebuffer &other);
	// copying to default framebuffer
	void blit(DxBase &base);

	void resize(ID3D11Device &dev, UINT width, UINT height);
	void clear(ID3D11DeviceContext &devcon, const D3DXCOLOR &color);
private:
	bool init(ID3D11Device &dev);
	void free();

	static DXGI_FORMAT GetDepthResourceFormat(DXGI_FORMAT depthformat);
	static DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT depthformat);

	ID3D11Texture2D **colortextures_;
	ID3D11ShaderResourceView **colorviews_;
	ID3D11RenderTargetView **colortargets_;
	ID3D11UnorderedAccessView **coloruavs_;

	ID3D11Texture2D *depthtexture_;
	ID3D11ShaderResourceView *depthview_;
	ID3D11DepthStencilView *depthtarget_;

	FramebufferParams params_;
};
#endif // FRAMEBUFFER_H
