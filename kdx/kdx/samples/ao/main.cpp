#include <Windows.h>
#include "dxbase.h"
#include "shader.h"
#include "camera.h"
#include "obj.h"
#include "framebuffer.h"
#include "sampler.h"

#define MOUSE_SENSITIVITY 20.f
#define MOVESPEED 0.05f

DxBase *window;
FirstPersonCamera cam;

struct VERTEX
{
	FLOAT x, y, z;
	D3DXCOLOR color;
};

struct MVPMatrices
{
	D3DXMATRIX model, view, proj;
};

long OnResize(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wnd.resize(LOWORD(lparam), HIWORD(lparam));
	cam.init(45, wnd.getAspect(), 1.f, 500.f);
	
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = FLOAT(window->getWidth());
	viewport.Height = FLOAT(window->getHeight());
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.0f; // WORKNOTE: if you don't set these on resize, things mess up; what are the defaults?

	wnd.getDeviceContext().RSSetViewports(1, &viewport);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	window = new DxBase(hInstance, 1024, 768);
	DxBase &wnd = *window;
	wnd.addMessageHandler(WM_SIZE, OnResize);
	wnd.showWindow(nShowCmd);
	
	ID3D11Device &dev = wnd.getDevice();
	ID3D11DeviceContext &devcon = wnd.getDeviceContext();

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = FLOAT(window->getWidth());
	viewport.Height = FLOAT(window->getHeight());
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.0f;

	devcon.RSSetViewports(1, &viewport);

	cam.init(45, wnd.getAspect(), 1.f, 500.f);
	cam.setPos(fl3(0, 0, -10)); // negative starting position for LH coordinate system

	// shaders
	VertexShader vs (dev, L"ssao.hlsl");
	PixelShader ps (dev, L"ssao.hlsl");
	VertexShader prepassvs (dev, L"prepass.hlsl");
	PixelShader prepassps (dev, L"prepass.hlsl");

	// framebuffers
	FramebufferParams params;
	params.width = wnd.getWidth();
	params.height = wnd.getHeight();
	params.numSamples = 1;
	params.numMrts = 1;
	params.colorFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	params.depthEnable = true;
	params.depthFormat = DXGI_FORMAT_D16_UNORM;
	Framebuffer prepassfb (dev, params);

	// temporarily putting my vertex buffer init stuff here
	VERTEX vertices[] =
	{
		{0.0f, 0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f)},
		{0.45f, -0.5, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)},
		{-0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)}
	};
	// try doing it with the mesh class instead
	InterleavedMesh<VERTEX, UINT8> mesh (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mesh.addVert(vertices[0])
		.addVert(vertices[1])
		.addVert(vertices[2]);
	mesh.addInd(0).addInd(1).addInd(2);
	mesh.finalize(dev);

	Obj servbot (dev, devcon, L"../assets/ServerBot1.obj");

	// full screen quad
	InterleavedMesh<PTvert, UINT8> quad (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	PTvert v;
	v.pos = fl3(0, 0, 0); v.tex = fl3(0, 1, 0); quad.addVert(v);
	v.pos = fl3(1, 0, 0); v.tex = fl3(1, 1, 0); quad.addVert(v);
	v.pos = fl3(0, 1, 0); v.tex = fl3(0, 0, 0); quad.addVert(v);
	v.pos = fl3(1, 1, 0); v.tex = fl3(1, 0, 0); quad.addVert(v);
	quad.addInd(0).addInd(2).addInd(1).addInd(1).addInd(2).addInd(3);
	quad.finalize(dev);
	// matrices for othrographic projection (constant)
	D3DXMATRIX fsorthoMat;
	D3DXMatrixOrthoOffCenterLH(&fsorthoMat, 0, 1, 0, 1, 0, 1);
	D3DXMatrixTranspose(&fsorthoMat, &fsorthoMat);
	ID3D11Buffer *fsorthoBuffer = 0;
	D3D11_BUFFER_DESC fsorthoDesc;
	D3D11_SUBRESOURCE_DATA fsorthoSubr;
	fsorthoDesc.Usage = D3D11_USAGE_IMMUTABLE;
	fsorthoDesc.ByteWidth = sizeof(D3DXMATRIX); // only need projection
	fsorthoDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fsorthoDesc.CPUAccessFlags = 0;
	fsorthoDesc.MiscFlags = 0;
	fsorthoDesc.StructureByteStride = 0;
	ZeroMemory(&fsorthoSubr, sizeof(D3D11_SUBRESOURCE_DATA));
	fsorthoSubr.pSysMem = &fsorthoMat;
	dev.CreateBuffer(&fsorthoDesc, &fsorthoSubr, &fsorthoBuffer);

	// create input layout
	// TODO: should this be a property of the mesh or its own separate thing?
	D3D11_INPUT_ELEMENT_DESC ied[] = 
	{
		//{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	// WORKNOTE: input element descriptor must exactly match fields in the shader source
	// and setInputLayout must be called with the correct number of items or else
	// input layout creation will fail
	ID3D11InputLayout *pFsLayout = vs.setInputLayout(dev, ied, 2);
	ID3D11InputLayout *pPrepassLayout = prepassvs.setInputLayout(dev, ied, 3);
	//devcon.IASetInputLayout(pLayout);

	// struct and buffer for camera matrices
	MVPMatrices matrices;
	ID3D11Buffer *matrixBuffer = 0;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_MAPPED_SUBRESOURCE cbufresource;
	// descriptor for constant buffer object
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MVPMatrices);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	dev.CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// extras for inverse projection matrix (used for reconstruction)
	D3DXMATRIX invCamPj;
	ID3D11Buffer *invCamPjBuffer = 0;
	D3D11_BUFFER_DESC invCamPjBufferDesc;
	D3D11_MAPPED_SUBRESOURCE invCamPjResource;
	invCamPjBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	invCamPjBufferDesc.ByteWidth = sizeof(D3DXMATRIX);
	invCamPjBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	invCamPjBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	invCamPjBufferDesc.MiscFlags = 0;
	invCamPjBufferDesc.StructureByteStride = 0;
	dev.CreateBuffer(&invCamPjBufferDesc, NULL, &invCamPjBuffer);

	while (window->isActive()) {
		window->update();

		if (window->isKeyPress(VK_ESCAPE)) {
			window->close();
			break;
		}
		if (window->isMousePress(MOUSE_RIGHT)) {
            window->hideCursor();
            window->holdCursor(true);
        } else if (window->isMouseRelease(MOUSE_RIGHT)) {
            window->showCursor();
            window->holdCursor(false);
        }
        if (window->isMouseDown(MOUSE_RIGHT)) {
            fl2 dxdy = window->getMouseNorm_dxdy(); // dxdy is relative to top left, positive y is down, positive x is right
			// looking right is negative rotation around y axis
			// looking down is negative rotation around x axis
            cam.rotate(fl2(-MOUSE_SENSITIVITY * dxdy.y, -MOUSE_SENSITIVITY * dxdy.x));
        }
        fl3 tomove;
        if(window->isKeyDown(KEY_W)) {
            tomove.z += 1.f; // positive Z is forward in LH coordinate system
        }
        if(window->isKeyDown(KEY_S)) {
            tomove.z -= 1.f;
        }
        if(window->isKeyDown(KEY_A)) {
            tomove.x -= 1.f;
        }
        if(window->isKeyDown(KEY_D)) {
            tomove.x += 1.f;
        }
        if(window->isKeyDown(KEY_SPACE)) {
            tomove.y += 1.f;
        }
        if(window->isKeyDown(KEY_CTRL)) {
            tomove.y -= 1.f;
        }
        normalize(tomove);
        tomove *= MOVESPEED * window->getdtBetweenUpdates();
        cam.move(tomove);

		// do graphics stuff
		prepassfb.use(devcon);
		prepassfb.clear(devcon, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f));

		// matrix stuff
		cam.toMatrixView(matrices.view);
		cam.toMatrixProj(matrices.proj);
		D3DXMatrixIdentity(&matrices.model);
		D3DXMatrixInverse(&invCamPj, NULL, &matrices.proj);

		// transpose necessary for D3D11 (and OpenGL)
		D3DXMatrixTranspose(&matrices.model, &matrices.model);
		D3DXMatrixTranspose(&matrices.view, &matrices.view);
		D3DXMatrixTranspose(&matrices.proj, &matrices.proj);

		// update the constant buffers
		devcon.Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbufresource);
		memcpy(cbufresource.pData, &matrices, sizeof(MVPMatrices));
		devcon.Unmap(matrixBuffer, 0);
		devcon.Map(invCamPjBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &invCamPjResource);
		memcpy(invCamPjResource.pData, &invCamPj, sizeof(D3DXMATRIX));
		devcon.Unmap(invCamPjBuffer, 0);

		// set the constant buffer in the shader itself (slot 0 for now)
		devcon.VSSetConstantBuffers(0, 1, &matrixBuffer);

		// WORKNOTE: not setting shaders caused driver crash
		devcon.VSSetShader(prepassvs.get(), 0, 0);
		devcon.PSSetShader(prepassps.get(), 0, 0);

		devcon.IASetInputLayout(pPrepassLayout);

		// drawing
		servbot.draw(dev, devcon);

		wnd.useDefaultFramebuffer();
		devcon.ClearRenderTargetView(&wnd.getBackBuffer(), D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f));
		devcon.ClearDepthStencilView(&wnd.getDepthBuffer(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// draw full screen quad for post process
		devcon.VSSetConstantBuffers(0, 1, &fsorthoBuffer);
		devcon.PSSetConstantBuffers(1, 1, &invCamPjBuffer);
		devcon.VSSetShader(vs.get(), 0, 0);
		devcon.PSSetShader(ps.get(), 0, 0);
		devcon.IASetInputLayout(pFsLayout);

		prepassfb.useColorResources(devcon, 0, 1);
		prepassfb.useDepthResource(devcon, 1);
		Sampler::GetDefaultSampler(dev).use(devcon, 0);

		quad.draw(dev, devcon);

		wnd.finishFrame();
		Sleep(1);
	}
	delete window;
	return 0;
}