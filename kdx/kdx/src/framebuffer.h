#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <D3D11.h>

class DxBase;

// initialization parameters
struct FramebufferParams {
	UINT width, height;
	UINT numMrts; // number of color targets
	UINT numSamples;
	bool depthEnable;
	DXGI_FORMAT colorFormat, depthFormat;
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

	// copying to another framebuffer
	void blit(ID3D11DeviceContext &devcon, Framebuffer &other);
	// copying to default framebuffer
	void blit(DxBase &base);

	void resize(ID3D11Device &dev, UINT width, UINT height);
private:
	bool init(ID3D11Device &dev);
	void free();

	static DXGI_FORMAT GetDepthResourceFormat(DXGI_FORMAT depthformat);
	static DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT depthformat);

	ID3D11Texture2D **colortextures_;
	ID3D11ShaderResourceView **colorviews_;
	ID3D11RenderTargetView **colortargets_;

	ID3D11Texture2D *depthtexture_;
	ID3D11ShaderResourceView *depthview_;
	ID3D11DepthStencilView *depthtarget_;

	FramebufferParams &params_;
};
#endif // FRAMEBUFFER_H
