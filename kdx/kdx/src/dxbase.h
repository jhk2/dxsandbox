#ifndef DXBASE_H
#define DXBASE_H

#include <Windows.h>
#include <map>
#include "utils.h"

#include <D3D11.h>
#include <D3DX11.h>
#include <D3DX10.h>

enum MOUSE_BUTTON {
	MOUSE_LEFT = 0, MOUSE_MIDDLE = 1, MOUSE_RIGHT = 2
};

class DxBase {
public:
	DxBase(HINSTANCE hInstance, unsigned int width, unsigned int height);
	virtual ~DxBase();
	static void ThrowError(const LPCWSTR &message);
	void resize(unsigned int width, unsigned int height);
	void update();

	void showWindow(int nCmdShow);
	void close();

	long double currentMillis();
	float getdtBetweenUpdates();

	unsigned int getWidth() { return width_; }
	unsigned int getHeight() { return height_; }
	float getAspect() { return ((float) width_) / ((float) height_); }
	bool isActive() { return running_; }

	// IO stuff
	bool isKeyDown(const unsigned int code);
	bool isKeyPress(const unsigned int code);
	bool isKeyRelease(const unsigned int code);
	bool isMouseDown(const MOUSE_BUTTON code);
	bool isMousePress(const MOUSE_BUTTON code);
	bool isMouseRelease(const MOUSE_BUTTON code);

	// mouse position
	int2 getMousePixelPos();
	int2 getMousePixel_dxdy();
	fl2 getMouseNormPos();
	fl2 getMouseNorm_dxdy();

	// mouse cursor hide/show
	void showCursor();
	void hideCursor();
	void holdCursor(const bool setting);

	// adding additional message handlers
	typedef long (* MessageHandler)(DxBase &, HWND, WPARAM, LPARAM);
	MessageHandler addMessageHandler(long message, MessageHandler handler);

	void finishFrame() { swapBuffers(); swapIODeviceBuffers(); }
private:
	void swapBuffers();
	void swapIODeviceBuffers();

	// map for storing message handlers
	typedef std::map<long, MessageHandler> MessageMap;
	typedef MessageMap::iterator MessageIterator;
	MessageMap messagemap_;
	bool getMessageHandler(long message, MessageIterator &it);

	// some default message handlers
	static long OnClose(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnDestroy(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnResize(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	// keyboard message handlers
	static long OnKeyDown(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnKeyUp(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	// mouse button handlers
	static long OnMouseDownL(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnMouseUpL(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnMouseDownM(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnMouseUpM(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnMouseDownR(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);
	static long OnMouseUpR(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam);

	bool init();

	// static windows message handler function
	static LRESULT CALLBACK RouteMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	void handleMessages();
	void setHwnd(HWND hwnd);
	HINSTANCE hInstance_;
	long exitCode_;
	HWND hwnd_;
	HDC hdc_;

	unsigned int width_, height_;
	bool running_;

	// mouse and keyboard related stuff
	unsigned int frontKeyBuffer_;
	bool keys_[2][256];
	bool mouse_buttons_[2][3];
	int2 mouse_pos_;
	int2 mouse_dxdy_;
	int2 hold_pos_;
	bool holdcursor_;

	void updatedt();
	long double current_millis_;
	unsigned long long current_ticks_;
	long double dt_;

private:
	// dx specific objects
	IDXGISwapChain *swapchain_;
	ID3D11Device *device_;
	ID3D11DeviceContext *devicecontext_;
	ID3D11RenderTargetView *backbuffer_;

public:
	void initD3D();
	void finishD3D();
	ID3D11Device& getDevice() { return *device_; }
	ID3D11DeviceContext& getDeviceContext() { return *devicecontext_; }
	ID3D11RenderTargetView& getBackBuffer() { return *backbuffer_; }
};

// enum for virtual key codes
enum WIN_KEYS {
    // numbers
    KEY_0=0x30,KEY_1=0x31,KEY_2=0x32,KEY_3=0x33,KEY_4=0x34,KEY_5=0x35,KEY_6=0x36,KEY_7=0x38,
    KEY_8=0x38,KEY_9=0x39,
    // alphabet
    KEY_A=0x41,KEY_B=0x42,KEY_C=0x43,KEY_D=0x44,KEY_E=0x45,KEY_F=0x46,KEY_G=0x47,KEY_H=0x48,
    KEY_I=0x49,KEY_J=0x4A,KEY_K=0x4B,KEY_L=0x4C,KEY_M=0x4D,KEY_N=0x4E,KEY_O=0x4F,KEY_P=0x50,
    KEY_Q=0x51,KEY_R=0x52,KEY_S=0x53,KEY_T=0x54,KEY_U=0x55,KEY_V=0x56,KEY_W=0x57,KEY_X=0x58,
    KEY_Y=0x59,KEY_Z=0x5A,
    // todo: add other keys
    KEY_ESC=0x1B, KEY_SPACE=0x20, KEY_CTRL=0x11
};
#endif // DXBASE_H