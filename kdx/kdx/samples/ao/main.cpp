#include <Windows.h>
#include "dxbase.h"
#include "shader.h"
#include "camera.h"

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

	devcon.RSSetViewports(1, &viewport);

	cam.init(45, wnd.getAspect(), 1.f, 500.f);
	cam.setPos(fl3(0, 0, 10));

	// shaders
	VertexShader vs (dev, L"ssao.hlsl");
	PixelShader ps (dev, L"ssao.hlsl");

	// temporarily putting my vertex buffer init stuff here
	VERTEX vertices[] =
	{
		{0.0f, 0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f)},
		{0.45f, -0.5, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)},
		{-0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)}
	};
	ID3D11Buffer *pVBuffer;
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev.CreateBuffer(&bd, NULL, &pVBuffer);

	D3D11_MAPPED_SUBRESOURCE ms;
	devcon.Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, vertices, sizeof(vertices));
	devcon.Unmap(pVBuffer, NULL);

	// create input layout
	D3D11_INPUT_ELEMENT_DESC ied[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	ID3D11InputLayout *pLayout = vs.setInputLayout(dev, ied, 2);
	devcon.IASetInputLayout(pLayout);

	MVPMatrices matrices;

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
            fl2 dxdy = window->getMouseNorm_dxdy();
            cam.rotate(fl2(-MOUSE_SENSITIVITY * dxdy.y, -MOUSE_SENSITIVITY * dxdy.x));
        }
        fl3 tomove;
        if(window->isKeyDown(KEY_W)) {
            tomove.z -= 1.f;
        }
        if(window->isKeyDown(KEY_S)) {
            tomove.z += 1.f;
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
		devcon.ClearRenderTargetView(&wnd.getBackBuffer(), D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

		// matrix stuff
		cam.toMatrixView(matrices.view);
		cam.toMatrixProj(matrices.proj);
		// send matrices to constant buffer
		ID3D11Buffer *matrixBuffer = 0;
		D3D11_BUFFER_DESC matrixBufferDesc;
		D3D11_MAPPED_SUBRESOURCE cbufresource;

		// transpose necessary for D3D11 (and OpenGL)
		//D3DXMatrixTranspose(&matrices.model, &matrices.model);
		D3DXMatrixTranspose(&matrices.view, &matrices.view);
		D3DXMatrixTranspose(&matrices.proj, &matrices.proj);

		//ZeroMemory(&matrices.proj, sizeof(D3DXMATRIX));
		//matrices.proj._11 = 1;
		//matrices.proj._22 = 1;
		//matrices.proj._33 = 1;
		//matrices.proj._44 = 1;

		// descriptor for constant buffer object
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(MVPMatrices);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		matrixBufferDesc.MiscFlags = 0;
		matrixBufferDesc.StructureByteStride = 0;

		HRESULT bufresult;
		// map the buffer and copy over the data
		bufresult = dev.CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);
		if (FAILED(bufresult)) {
			DxBase::ThrowError(L"createbuffer failed");
			return 1;
		}
		bufresult = devcon.Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbufresource);
		if (FAILED(bufresult)) {
			DxBase::ThrowError(L"map failed");
			return 1;
		}
		MVPMatrices *matrixBufferPtr = (MVPMatrices *) cbufresource.pData;
		matrixBufferPtr->model = matrices.model;
		matrixBufferPtr->view = matrices.view;
		matrixBufferPtr->proj = matrices.proj;
		devcon.Unmap(matrixBuffer, 0);

		// set the constant buffer in the shader itself (slot 0 for now)
		devcon.VSSetConstantBuffers(0, 1, &matrixBuffer);

		// WORKNOTE: not setting shaders caused driver crash
		devcon.VSSetShader(vs.get(), 0, 0);
		devcon.PSSetShader(ps.get(), 0, 0);

		// drawing
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;
		devcon.IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
		devcon.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		devcon.Draw(3, 0);

		wnd.finishFrame();
		Sleep(1);
	}
	delete window;
	return 0;
}