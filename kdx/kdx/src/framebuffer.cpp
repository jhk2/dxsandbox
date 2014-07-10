#include "framebuffer.h"
#include "dxbase.h"

Framebuffer::Framebuffer(ID3D11Device &dev, FramebufferParams &params) : params_(params)
{
	init(dev);
}

Framebuffer::~Framebuffer()
{
	free();
}

void Framebuffer::use(ID3D11DeviceContext &devcon)
{
	devcon.OMSetRenderTargets(params_.numMrts, colortargets_, depthtarget_);
}

void Framebuffer::useColorResources(ID3D11DeviceContext &devcon, UINT slot, UINT count)
{
	devcon.PSSetShaderResources(slot, count, colorviews_);
}

void Framebuffer::useDepthResource(ID3D11DeviceContext &devcon, UINT slot)
{
	devcon.PSSetShaderResources(slot, 1, &depthview_);
}

void Framebuffer::blit(ID3D11DeviceContext &devcon, Framebuffer &other)
{
	for (UINT i = 0; i < min(params_.numMrts, other.params_.numMrts); i++) {
		devcon.CopyResource(other.colortextures_[i], colortextures_[i]);
	}
	if (params_.depthEnable && other.params_.depthEnable) {
		devcon.CopyResource(other.depthtexture_, depthtexture_);
	}
}

void Framebuffer::blit(DxBase &base)
{
	if (params_.numMrts > 0) {
		base.devicecontext_->CopyResource(base.backbuffertex_, colortextures_[0]);
	}

	if (params_.depthEnable) {
		base.devicecontext_->CopyResource(base.depthbuffertex_, depthtexture_);
	}
}

void Framebuffer::resize(ID3D11Device &dev, UINT width, UINT height)
{
	params_.width = width;
	params_.height = height;
	free();
	init(dev);
}

void Framebuffer::clear(ID3D11DeviceContext &devcon, const D3DXCOLOR &color)
{
	for (UINT i = 0; i < params_.numMrts; i++) {
		devcon.ClearRenderTargetView(colortargets_[i], color);
	}
	if (params_.depthEnable) {
		// WORKNOTE: only clearing depth for now, need to add a stencil clear flag if necessary
		devcon.ClearDepthStencilView(depthtarget_, D3D11_CLEAR_DEPTH, 1.0, 0);
	}
}

bool Framebuffer::init(ID3D11Device &dev)
{
	if (params_.numMrts > 0) {
		// create the color buffers
		D3D11_TEXTURE2D_DESC colorDesc;
		ZeroMemory(&colorDesc, sizeof(D3D11_TEXTURE2D_DESC));
		colorDesc.Width = params_.width;
		colorDesc.Height = params_.height;
		colorDesc.MipLevels = 1;
		colorDesc.ArraySize = 1;
		colorDesc.Format = params_.colorFormat;
		colorDesc.SampleDesc.Count = params_.numSamples;
		colorDesc.SampleDesc.Quality = 0;
		colorDesc.Usage = D3D11_USAGE_DEFAULT;
		colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		colorDesc.CPUAccessFlags = 0;
		colorDesc.MiscFlags = 0;

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = params_.colorFormat;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = params_.colorFormat;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		// WORKNOTE: we will do individual texture 2Ds with separate render target views for now
		// but a texture 2D array in a single render target view is another option
		colortextures_ = new ID3D11Texture2D*[params_.numMrts];
		colorviews_ = new ID3D11ShaderResourceView*[params_.numMrts];
		colortargets_ = new ID3D11RenderTargetView*[params_.numMrts];

		for (UINT i = 0; i < params_.numMrts; i++) {
			dev.CreateTexture2D(&colorDesc, NULL, &colortextures_[i]);
			dev.CreateShaderResourceView(colortextures_[i], &srvDesc, &colorviews_[i]);
			dev.CreateRenderTargetView(colortextures_[i], &rtvDesc, &colortargets_[i]);
		}
	}

	if (params_.depthEnable) {
		D3D11_TEXTURE2D_DESC depthDesc;
		ZeroMemory(&depthDesc, sizeof(D3D11_TEXTURE2D_DESC));
		depthDesc.Width = params_.width;
		depthDesc.Height = params_.height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		// WORKNOTE: for depth buffer to be accessible both as depth buffer and shader resource,
		// it needs to be typeless, with the shader resource view and depth stencil view
		// each setting their own type params to determine how to interpret the data
		depthDesc.Format = GetDepthResourceFormat(params_.depthFormat);
		depthDesc.SampleDesc.Count = params_.numSamples;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;

		HRESULT result;
		result = dev.CreateTexture2D(&depthDesc, NULL, &depthtexture_);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.Format = GetShaderResourceViewFormat(params_.depthFormat);
		srvDesc.ViewDimension = params_.numSamples > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		result = dev.CreateShaderResourceView(depthtexture_, &srvDesc, &depthview_);

		D3D11_DEPTH_STENCIL_VIEW_DESC targetDesc;
		ZeroMemory(&targetDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		targetDesc.Format = params_.depthFormat;
		targetDesc.ViewDimension = params_.numSamples > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		targetDesc.Texture2D.MipSlice = 0;
		result = dev.CreateDepthStencilView(depthtexture_, &targetDesc, &depthtarget_);
	}
	return true;
}

void Framebuffer::free()
{
	for (UINT i = 0; i < params_.numMrts; i++) {
		colortargets_[i]->Release();
		colorviews_[i]->Release();
		colortextures_[i]->Release();
	}
	if (params_.numMrts > 0) {
		delete[] colortargets_;
		delete[] colorviews_;
		delete[] colortextures_;
	}

	if (params_.depthEnable) {
		depthtarget_->Release();
		depthview_->Release();
		depthtexture_->Release();
	}
}

// functions to convert the depth format into the specific formats for the base resource
// and the shader resource view (the depth render target just uses the normal depth format)
// see https://stackoverflow.com/questions/20256815/how-to-check-the-content-of-the-depth-stencil-buffer
/*static*/ DXGI_FORMAT Framebuffer::GetDepthResourceFormat(DXGI_FORMAT depthformat)
{
	DXGI_FORMAT resformat;
	switch (depthformat)
	{
	case DXGI_FORMAT_D16_UNORM:
			resformat = DXGI_FORMAT_R16_TYPELESS;
			break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
			resformat = DXGI_FORMAT_R24G8_TYPELESS;
			break;
	case DXGI_FORMAT_D32_FLOAT:
			resformat = DXGI_FORMAT_R32_TYPELESS;
			break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			resformat = DXGI_FORMAT_R32G8X24_TYPELESS;
			break;
	}

	return resformat;
}

/*static*/ DXGI_FORMAT Framebuffer::GetShaderResourceViewFormat(DXGI_FORMAT depthformat)
{
	DXGI_FORMAT srvformat;
    switch (depthformat)
    {
    case DXGI_FORMAT_D16_UNORM:
            srvformat = DXGI_FORMAT_R16_FLOAT;
            break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
            srvformat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            break;
    case DXGI_FORMAT_D32_FLOAT:
            srvformat = DXGI_FORMAT_R32_FLOAT;
            break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            srvformat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            break;
    }
    return srvformat;
}