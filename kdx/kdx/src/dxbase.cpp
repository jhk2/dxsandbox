#include "dxbase.h"
#include <tchar.h>

DxBase::DxBase(HINSTANCE hInstance, unsigned int width, unsigned int height) : hInstance_(hInstance), width_(width), height_(height)
{
	hInstance_ = hInstance;
    holdcursor_ = false;
    frontKeyBuffer_ = 0;

    // clear key and mouse buffers
    memset(keys_[0], 0, sizeof(bool)*256);
    memset(keys_[1], 0, sizeof(bool)*256);
    memset(mouse_buttons_[0], 0, sizeof(bool)*3);
    memset(mouse_buttons_[1], 0, sizeof(bool)*3);

    addMessageHandler(WM_CLOSE, OnClose);
    addMessageHandler(WM_DESTROY, OnDestroy);
	addMessageHandler(WM_SIZE, OnResize);
    addMessageHandler(WM_KEYDOWN, OnKeyDown);
    addMessageHandler(WM_KEYUP, OnKeyUp);
    addMessageHandler(WM_LBUTTONDOWN, OnMouseDownL);
    addMessageHandler(WM_LBUTTONUP, OnMouseUpL);
    addMessageHandler(WM_MBUTTONDOWN, OnMouseDownM);
    addMessageHandler(WM_MBUTTONUP, OnMouseUpM);
    addMessageHandler(WM_RBUTTONDOWN, OnMouseDownR);
    addMessageHandler(WM_RBUTTONUP, OnMouseUpR);
    init();
	initD3D();
}

DxBase::~DxBase()
{
	finishD3D();
}

/*static*/ void DxBase::ThrowError(const LPCWSTR &message)
{
	MessageBox(NULL, message, L"Error", MB_ICONEXCLAMATION | MB_OK);
}

void DxBase::update()
{
	handleMessages();
	updatedt();

    // calculate mouse motion
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd_, &pt);
    mouse_dxdy_.x = pt.x - mouse_pos_.x;
    mouse_dxdy_.y = pt.y - mouse_pos_.y;
    mouse_pos_.x = pt.x;
    mouse_pos_.y = pt.y;

	// if hold cursor is on, recenter
    if(holdcursor_) {
        pt.x = hold_pos_.x;
        pt.y = hold_pos_.y;
        ClientToScreen(hwnd_, &pt);
        SetCursorPos(pt.x, pt.y);
        mouse_pos_ = hold_pos_;
    }
}

/*static*/ LRESULT CALLBACK DxBase::RouteMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	DxBase *wnd = 0;
	if(message == WM_NCCREATE) {
        // when the window is created, we want to save its unique ID to GWL_USERDATA so we can retreive this particular window
        // in a static context later when more messages come in

        // get the individual window instance from Windows
        wnd = reinterpret_cast<DxBase *>(((LPCREATESTRUCT) lparam)->lpCreateParams);
        SetWindowLong(hwnd, GWL_USERDATA, reinterpret_cast<long>(wnd));
        // save the hwnd
        printf("got nccreate, set hwnd to %x\n", hwnd);
        wnd->setHwnd(hwnd);
    } else {
        // get the individual window from GWL_USERDATA
        wnd = reinterpret_cast<DxBase *>(GetWindowLong(hwnd, GWL_USERDATA));
    }
    if(wnd) {
        DxBase::MessageIterator it;
        // get the message handler from our window's message map
        bool found = wnd->getMessageHandler(message, it);
        if(found) {
            // call the function that the message map points to
            return (it->second)((*wnd), hwnd, wparam, lparam);
        }
    }
    // otherwise return the default window proc
    return DefWindowProc(hwnd, message, wparam, lparam);
}

void DxBase::handleMessages()
{
	MSG msg;
	if (!hwnd_) ThrowError(L"handle messages called on null hwnd");
	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        int temp = GetMessage(&msg, NULL, 0, 0);
        if(temp > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            //throwError("getmessage returned nonpositive");  not an error if it's a quit
            running_ = false;
            exitCode_ = (long) msg.lParam;
            break;
        }
    }
}


DxBase::MessageHandler DxBase::addMessageHandler(long message, MessageHandler handler)
{
    // insert the message handler into the map and return the old one that was replaced, if it exists
    MessageHandler m = NULL;
    MessageIterator it = messagemap_.find(message);
    if(it != messagemap_.end())
        m = it->second;
	// insert doesn't replace if element already exists
    //messagemap_.insert(std::pair<long, MessageHandler>(message, handler));
	messagemap_[message] = handler;
    return m;
}

bool DxBase::getMessageHandler(long message, DxBase::MessageIterator &it)
{
    // look in the message map for the handler
    DxBase::MessageIterator findit = messagemap_.find(message);
    if(findit == messagemap_.end())
        return false;
    it = findit;
    return true;
}


const LPCTSTR g_windowClass = L"DxBaseWindowClass";

bool DxBase::init()
{
	WNDCLASSEX wc;
	HWND hwnd;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	
	// register window class with default settings
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RouteMessage;
	wc.hInstance = hInstance_;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW; // tutorial has (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = g_windowClass;

	if(!RegisterClassEx(&wc)) {
		ThrowError(L"Window Registration Failed");
		return false;
	}

	// adjust window size to account for bars and borders
	RECT windowRect = { 0, 0, width_, height_ };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// create window
	hwnd = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		g_windowClass,
		L"Title Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
		NULL, NULL, hInstance_, this);

	// 
	hwnd_ = hwnd;
	hdc_ = GetDC(hwnd);
	running_ = true;
	return true;
}

void DxBase::showWindow(int nCmdShow)
{
	ShowWindow(hwnd_, nCmdShow);
	SetForegroundWindow(hwnd_);
	SetFocus(hwnd_);
	UpdateWindow(hwnd_);

	// set initial mouse position
	POINT p;
	if (GetCursorPos(&p)) {
		if (ScreenToClient(hwnd_, &p)) {
			
		}
	}
}

long double DxBase::currentMillis()
{
    LARGE_INTEGER freq; // ticks of performance counter per second (2862568 on my machine)
    BOOL use_qpc = QueryPerformanceFrequency(&freq);

    if (use_qpc) {
        LARGE_INTEGER ticks;
        QueryPerformanceCounter(&ticks);
        current_ticks_ = ticks.QuadPart;
        return (1000.0L * current_ticks_) / freq.QuadPart;
    } else {
        return static_cast<long double>(GetTickCount());
    }
}

void DxBase::updatedt()
{
    LARGE_INTEGER freq; // ticks of performance counter per second (2862568 on my machine)
    BOOL use_qpc = QueryPerformanceFrequency(&freq);

    if (use_qpc) {
        LARGE_INTEGER ticks;
        QueryPerformanceCounter(&ticks);
        unsigned long long newticks = ticks.QuadPart;
        unsigned long long dticks = newticks - current_ticks_;
        current_ticks_ = newticks;
        dt_ = (1000.0L * dticks) / freq.QuadPart; // ticks / (ticks/sec) = seconds * 1000 = milliseconds
        current_millis_ = (1000.L * current_ticks_) / freq.QuadPart;
    } else {
        long double newt = static_cast<long double>(GetTickCount());
        dt_ = newt - current_millis_;
        current_millis_ = newt;
    }
}

void DxBase::resize(unsigned int width, unsigned int height)
{
	width_ = width;
	height_ = height;
	resizeD3D();
}

void DxBase::swapBuffers()
{
	swapchain_->Present(0,0);
}

void DxBase::setHwnd(HWND hwnd)
{
	hwnd_ = hwnd;
}

void DxBase::close()
{
	SendMessage(hwnd_, WM_CLOSE, 0, 0);
}

// IO stuff
bool DxBase::isKeyDown(const unsigned int code)
{
    return keys_[frontKeyBuffer_][code];
}

bool DxBase::isKeyPress(const unsigned int code)
{
    return keys_[frontKeyBuffer_][code] && !keys_[1-frontKeyBuffer_][code];
}

bool DxBase::isKeyRelease(const unsigned int code)
{
    return !keys_[frontKeyBuffer_][code] && keys_[1-frontKeyBuffer_][code];
}

bool DxBase::isMouseDown(const MOUSE_BUTTON code)
{
    return mouse_buttons_[frontKeyBuffer_][code];
}

bool DxBase::isMousePress(const MOUSE_BUTTON code)
{
    return mouse_buttons_[frontKeyBuffer_][code] && !mouse_buttons_[1-frontKeyBuffer_][code];
}

bool DxBase::isMouseRelease(const MOUSE_BUTTON code)
{
    return !mouse_buttons_[frontKeyBuffer_][code] && mouse_buttons_[1-frontKeyBuffer_][code];
}

void DxBase::swapIODeviceBuffers()
{
    frontKeyBuffer_ = 1 - frontKeyBuffer_;
    memcpy(&keys_[frontKeyBuffer_][0], &keys_[1-frontKeyBuffer_][0], sizeof(keys_[0]));
    memcpy(&mouse_buttons_[frontKeyBuffer_][0], &mouse_buttons_[1-frontKeyBuffer_][0], sizeof(mouse_buttons_[0]));
}

int2 DxBase::getMousePixelPos()
{
    return mouse_pos_;
}

int2 DxBase::getMousePixel_dxdy()
{
    return mouse_dxdy_;
}

fl2 DxBase::getMouseNormPos()
{
    return fl2(((float) mouse_pos_.x) / width_, ((float) mouse_pos_.y) / height_);
}

fl2 DxBase::getMouseNorm_dxdy()
{
    return fl2(((float) mouse_dxdy_.x) / width_, ((float) mouse_dxdy_.y) / height_);
}

void DxBase::showCursor()
{
    ShowCursor(true);
}

void DxBase::hideCursor()
{
    ShowCursor(false);
}

void DxBase::holdCursor(const bool setting)
{
    holdcursor_ = setting;
    if(holdcursor_) {
        hold_pos_ = mouse_pos_;
    }
}

float DxBase::getdtBetweenUpdates()
{
	return static_cast<float>(dt_);
}

// default message handler for closing
long DxBase::OnClose(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    DestroyWindow(hwnd);
    return 0;
}

// default message handler for destroy
long DxBase::OnDestroy(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    PostQuitMessage(0);
    return 0;
}

// default message handler for resize (will probably be replaced by app)
long DxBase::OnResize(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wnd.resize(LOWORD(lparam), HIWORD(lparam));
	return 0;
}

long DxBase::OnKeyDown(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.keys_[wnd.frontKeyBuffer_][wparam] = true;
    return true;
}

long DxBase::OnKeyUp(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.keys_[wnd.frontKeyBuffer_][wparam] = false;
    return false;
}

long DxBase::OnMouseDownL(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][0] = true;
    return true;
}

long DxBase::OnMouseUpL(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][0] = false;
    return true;
}

long DxBase::OnMouseDownM(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][1] = true;
    return true;
}

long DxBase::OnMouseUpM(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][1] = false;
    return true;
}

long DxBase::OnMouseDownR(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][2] = true;
    return true;
}

long DxBase::OnMouseUpR(DxBase &wnd, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wnd.mouse_buttons_[wnd.frontKeyBuffer_][2] = false;
    return true;
}

void DxBase::createRenderTargets()
{
	// assuming that swap chain is properly initialized, create the color and depth render targets
	// get the address of the texture
	swapchain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *) &backbuffertex_);
	// use the address to make the render target object
	device_->CreateRenderTargetView(backbuffertex_, NULL, &backbuffer_);

	// create depth target
	D3D11_TEXTURE2D_DESC depthDesc;
	ZeroMemory(&depthDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthDesc.Width = width_;
	depthDesc.Height = height_;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D16_UNORM;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	device_->CreateTexture2D(&depthDesc, NULL, &depthbuffertex_);
	// WORKNOTE: look into depth/stencil state for more complex usage
	device_->CreateDepthStencilView(depthbuffertex_, NULL, &depthbuffer_);
	// for now, just permanently bind this default depth buffer to output merger
	// set this render target as the back buffer
	devicecontext_->OMSetRenderTargets(1, &backbuffer_, depthbuffer_);
}

void DxBase::initD3D()
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd_;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	// create the device, context, and swap chain with this struct data
	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, 
		D3D_DRIVER_TYPE_HARDWARE,
		NULL, NULL, // this can be D3D11_CREATE_DEVICE_DEBUG, but debugging needs to be installed properly
		NULL, NULL, 
		D3D11_SDK_VERSION, 
		&swapChainDesc, 
		&swapchain_, 
		&device_,
		NULL, 
		&devicecontext_);

	createRenderTargets();
}

void DxBase::resizeD3D()
{
	// release all existing buffers in the swap chain + the depth buffer
	// TODO: resizing is messing up the depth buffer
	devicecontext_->OMSetRenderTargets(0, 0, 0);
	backbuffer_->Release();
	backbuffertex_->Release();
	depthbuffer_->Release();
	depthbuffertex_->Release();
	// resize the buffers
	swapchain_->ResizeBuffers(0, width_, height_, DXGI_FORMAT_UNKNOWN, 0); // just resizes and keeps all else the same
	// get the handles to the buffers again
	createRenderTargets();
}

void DxBase::finishD3D()
{
	swapchain_->Release();
	backbuffer_->Release();
	backbuffertex_->Release();
	depthbuffer_->Release();
	depthbuffertex_->Release();
	device_->Release();
	devicecontext_->Release();
}