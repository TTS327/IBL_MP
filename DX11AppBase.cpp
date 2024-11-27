// 참고: 헤더 include 순서
// https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes

#include "DX11AppBase.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <dxgi.h>                       // DXGIFactory
#include <dxgi1_4.h>                    // DXGIFactory4

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace FEFE 
{

using namespace std;
using namespace DirectX;

// RegisterClassEx()에서 멤버 함수를 직접 등록할 수가 없음
// 클래스의 멤버 함수에서 간접적으로 메시지를 처리할 수 있도록 도와줍니다.
AppBase *g_appBase = nullptr;

// RegisterClassEx()에서 실제로 등록될 콜백 함수
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    // g_appBase를 이용해서 간접적으로 멤버 함수 호출
    return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}

// 생성자, 자식생성자에서 호출
// 해상도 1280 x 960
// window, 포인터 초기화
AppBase::AppBase()
    : m_screenWidth(1280), m_screenHeight(960), m_mainWindow(0),
      m_d3dScreenViewPort(D3D11_VIEWPORT()) // 어디에 그릴지 뷰포트 지정, 일단 기본 값으로
{

    g_appBase = this;  
}

AppBase::~AppBase()
{
    g_appBase = nullptr;

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyWindow(m_mainWindow);
 
}

float AppBase::GetAspectRatio() const
{
    return float(m_screenWidth - m_guiWidth) / m_screenHeight;
}

int AppBase::Run()
{
    // Main loop
    MSG msg = {0};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } 
        else
         {
            ImGui_ImplDX11_NewFrame(); // GUI 프레임 시작
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame(); // 어떤 것들을 렌더링 할지 기록 시작
            ImGui::Begin("Scene Control");

            // ImGui가 측정해주는 Framerate 출력
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);

            UpdateGUI(); // 추가적으로 사용할 GUI

            m_guiWidth = 0;
            // 화면을 크게 쓰기 위해 기능 정지
            // ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
            // m_guiWidth = int(ImGui::GetWindowWidth());

            ImGui::End();
            ImGui::Render(); // 렌더링할 것들 기록 끝

            Update(ImGui::GetIO().DeltaTime); // 애니메이션 같은 변화

            Render(); // 우리가 구현한 렌더링

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI 렌더링

            // Switch the back buffer and the front buffer
            // 주의: ImGui RenderDrawData() 다음에 Present() 호출
            m_d3dSwapChain->Present(1, 0);
        }
    }

    return 0;
}

bool AppBase::Initialize() 
{

    if (!InitMainWindow())  // window 생성
        return false;

    if (!InitDirect3D())    // window에 어떻게 그릴지
        return false;
        
    if (!InitGUI())         // GUI 초기화
        return false;

    return true;
}

LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // Reset and resize swapchain
        // std::cout << (UINT)LOWORD(lParam) << " " << (UINT)HIWORD(lParam)
        //          << std::endl;

        if (m_d3dSwapChain)
        { // 처음 실행이 아닌지 확인

            m_screenWidth = int(LOWORD(lParam));
            m_screenHeight = int(HIWORD(lParam));
            m_guiWidth = 0;

            m_d3dRenderTargetView.Reset();
            m_d3dSwapChain->ResizeBuffers(0, // 현재 개수 유지
                (UINT)LOWORD(lParam),        // 해상도 변경
                (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN,         // 현재 포맷 유지
                0);
            CreateRenderTargetView();
            CreateDepthBuffer();
            SetViewport();
        }

        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_MOUSEMOVE:
        // cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << endl;
        break;
    case WM_LBUTTONUP:
        // cout << "WM_LBUTTONUP Left mouse button" << endl;
        break;
    case WM_RBUTTONUP:
        // cout << "WM_RBUTTONUP Right mouse button" << endl;
        break;
    case WM_KEYDOWN:
        // cout << "WM_KEYDOWN " << (int)wParam << endl;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

bool AppBase::InitMainWindow() 
{
    // 구조체
    WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                     CS_CLASSDC,
                     WndProc,       // 콜백함수 등록
                     0L,
                     0L,
                     GetModuleHandle(NULL),
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     L"FEFE_IBL", // lpszClassName, L-string
                     NULL};

    // 생성실패확인
    if (!RegisterClassEx(&wc)) 
    {
        cout << "RegisterClassEx() failed./// DX11AppBase.cpp" << endl;
        return false;
    }

    // 툴바까지 포함한 윈도우 전체 해상도가 아니라
    // 우리가 실제로 그리는 해상도가 width x height가 되도록
    // 윈도우를 만들 해상도를 다시 계산해서 CreateWindow()에서 사용

    // 우리가 원하는 그림이 그려질 부분의 해상도 1280 x 960
    RECT wr = {0, 0, m_screenWidth, m_screenHeight};

    // 필요한 윈도우 크기(해상도) 계산
    // wr의 값이 바뀜
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    // 윈도우를 만들때 위에서 계산한 wr 사용
    // 실제 윈도우를 생성, 툴바 크기 제외하고
    m_mainWindow = CreateWindow(wc.lpszClassName, L"FEFE_IBL MEDIAPROJECT",
                                WS_OVERLAPPEDWINDOW,
                                50, // 윈도우 좌측 상단의 x 좌표
                                50, // 윈도우 좌측 상단의 y 좌표
                                wr.right - wr.left, // 윈도우 가로 방향 해상도
                                wr.bottom - wr.top, // 윈도우 세로 방향 해상도
                                NULL, NULL, wc.hInstance, NULL);

    if (!m_mainWindow) 
    {
        cout << "CreateWindow() failed." << endl;
        return false;
    }

    ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
    UpdateWindow(m_mainWindow);

    return true;
}

bool AppBase::InitDirect3D()
{
    const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;   
#endif

    // shared_ptr 처럼 ComPtr 사용. 지역변수로.
    // device는 주체, context는 문법? 이라 생각하면 될듯하다
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;


    const D3D_FEATURE_LEVEL featureLevels[2] = 
    {
        D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정해야 한다고 함
        D3D_FEATURE_LEVEL_9_3};
    D3D_FEATURE_LEVEL featureLevel;
       

    
    /*HRESULT D3D11CreateDevice(
        [in, optional]  IDXGIAdapter * pAdapter,
        D3D_DRIVER_TYPE         DriverType,
        HMODULE                 Software,
        UINT                    Flags,
        [in, optional]  const D3D_FEATURE_LEVEL * pFeatureLevels,
        UINT                    FeatureLevels,
        UINT                    SDKVersion,
        [out, optional] ID3D11Device * *ppDevice,
        [out, optional] D3D_FEATURE_LEVEL * pFeatureLevel,
        [out, optional] ID3D11DeviceContext * *ppImmediateContext
    );*/
    // CreateDevice 반환값 hresult, 이걸 FAILED 라는 매크로로 확인
    if (FAILED(D3D11CreateDevice(
            nullptr,                    // Specify nullptr to use the default adapter.
            driverType,                 // Create a device using the hardware graphics driver.
            0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            createDeviceFlags,          // Set debug and Direct2D compatibility flags.
            featureLevels,              // List of feature levels this app can support.
            ARRAYSIZE(featureLevels),   // Size of the list above.
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for
                                        // Microsoft Store apps.
            device.GetAddressOf(),      // Returns the Direct3D device created.
            &featureLevel,              // Returns feature level of device created.
            context.GetAddressOf()      // Returns the device immediate context.
            )))
    {
        cout << "D3D11CreateDevice() failed. DX11AppBase.cpp" << endl;
        return false;
    }

    

    if (featureLevel != D3D_FEATURE_LEVEL_11_0) 
    {
        cout << "D3D Feature Level 11 unsupported." << endl;
        return false;
    }


    // 4X MSAA 지원하는지 확인,  멀티샘플링
    // UINT numQualityLevels; // Resize 처리를 위해 멤버 변수로 변경
    // 샘플 개수 4로 지정
    device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4,
                                          &numQualityLevels);
    if (numQualityLevels <= 0) 
    {
        cout << "MSAA not supported." << endl;
    }

    // numQualityLevels = 0; // MSAA 강제로 끄기

    if (FAILED(device.As(&m_d3dDevice)))
    {
        cout << "device.AS() failed." << endl;
        return false;
    }

    if (FAILED(context.As(&m_d3dContext)))
    {
        cout << "context.As() failed." << endl;
        return false;
    }

    // 구조체
    // 더블버퍼링 생각. 백버퍼 <-> 프론트버퍼
    DXGI_SWAP_CHAIN_DESC sd;        // DESC = description, 어떻게 스왑체인만들것인지
    ZeroMemory(&sd, sizeof(sd));    // 0으로 초기화 해줌.  ZeroMemory로 
    sd.BufferDesc.Width = m_screenWidth;               // 렌더링할 해상도
    sd.BufferDesc.Height = m_screenHeight;             // 렌더링할 해상도
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 컬러하나표현시 32비트 사용, 데이터형식 unsigned normalized
    sd.BufferCount = 2;                                // Double-buffering, 그 이상도 가능
    sd.BufferDesc.RefreshRate.Numerator = 60;          // 초당 60 fps
    sd.BufferDesc.RefreshRate.Denominator = 1;         // ..
    sd.BufferUsage =
        DXGI_USAGE_RENDER_TARGET_OUTPUT;                // 어디에 쓰일지
    sd.OutputWindow = m_mainWindow;                     // 만든 윈도우에
    sd.Windowed = TRUE;                                 // 테두리 창으로
    sd.Flags =
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 전체, 창모드 가능하도록// flag에 따라
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (numQualityLevels > 0) 
    {
        sd.SampleDesc.Count = 4; // 4번 멀디샘플링
        sd.SampleDesc.Quality = numQualityLevels - 1;
    } else
    {
        sd.SampleDesc.Count = 1; // or 1번
        sd.SampleDesc.Quality = 0;
    }

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            0, // 기본값
            driverType,
            0, // 소프트웨어 디바이스 X
            createDeviceFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd,
            m_d3dSwapChain.GetAddressOf(), m_d3dDevice.GetAddressOf(), &featureLevel,
            m_d3dContext.GetAddressOf()))) 

    {
        cout << "D3D11CreateDeviceAndSwapChain() failed." << endl;
        return false;
    }


    CreateRenderTargetView();       // gpu에 있는 메모리를 렌더타켓으로

    SetViewport();

    // Create a rasterizer state
    D3D11_RASTERIZER_DESC rastDesc; // DESC 생성 후
    ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC)); // 기본값으로 채워주고
    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
   
    rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;           // backface 컬링 안하게 함
    rastDesc.FrontCounterClockwise = false;
    rastDesc.DepthClipEnable = true; // <- zNear, zFar 확인에 필요

    // rasterizerstate 만듦, 렌더링할때 사용
    m_d3dDevice->CreateRasterizerState(&rastDesc,
                                    m_d3dSolidRasterizerSate.GetAddressOf());

    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;      // wire로 그려줌

    m_d3dDevice->CreateRasterizerState(&rastDesc,
                                    m_d3dWireRasterizerSate.GetAddressOf());


    CreateDepthBuffer();

    // Create depth stencil state
    // DESC로 만들고, 설정하는 단계
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask =
        D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc =
        D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
    if (FAILED(m_d3dDevice->CreateDepthStencilState(
            &depthStencilDesc, m_d3dDepthStencilState.GetAddressOf())))
    {
        cout << "CreateDepthStencilState() failed." << endl;
    }

    return true;
}

bool AppBase::InitGUI()
{
    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplDX11_Init(m_d3dDevice.Get(), m_d3dContext.Get())) 
    {
        return false;
    }

    if (!ImGui_ImplWin32_Init(m_mainWindow)) 
    {
        return false;
    }

    return true;
}

void AppBase::SetViewport()
{

    static int previousGuiWidth = -1;

    if (previousGuiWidth != m_guiWidth) 
    {

        previousGuiWidth = m_guiWidth;

        // Set the viewport
        ZeroMemory(&m_d3dScreenViewPort, sizeof(D3D11_VIEWPORT));
        // 범위 설정
        m_d3dScreenViewPort.TopLeftX = float(m_guiWidth);
        m_d3dScreenViewPort.TopLeftY = 0;
        m_d3dScreenViewPort.Width = float(m_screenWidth - m_guiWidth);
        m_d3dScreenViewPort.Height = float(m_screenHeight);
        // depth버퍼 생각
        m_d3dScreenViewPort.MinDepth = 0.0f;
        m_d3dScreenViewPort.MaxDepth = 1.0f; 
        // 레스터단계에서 
        // 3차원 -> 2차원 projection, NDC 등등
        m_d3dContext->RSSetViewports(1, &m_d3dScreenViewPort);
    }
}

// gpu에 있는 메모리를 어떻게 사용하냐를 View라고 한다.
// 스왑체인이 가지고 있는 버퍼를 가져다가 렌더링 대상으로 사용하겠다.
bool AppBase::CreateRenderTargetView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    m_d3dSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (backBuffer)
    {
        m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), NULL,
            m_d3dRenderTargetView.GetAddressOf());
    } 
    else 
    {
        std::cout << "CreateRenderTargetView() failed." << std::endl;
        return false;
    }

    return true;
}

bool AppBase::CreateDepthBuffer()
{
    // Depth값을 저장하는 메모리이기 때문에 2D 사용
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    depthStencilBufferDesc.Width = m_screenWidth;
    depthStencilBufferDesc.Height = m_screenHeight;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.ArraySize = 1;
    // 스텐실저장
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (numQualityLevels > 0)
    {
        depthStencilBufferDesc.SampleDesc.Count = 4; // 4번 멀티샘플
        depthStencilBufferDesc.SampleDesc.Quality = numQualityLevels - 1;
    } 
    else 
    {
        depthStencilBufferDesc.SampleDesc.Count = 1; // 1번만 샘플
        depthStencilBufferDesc.SampleDesc.Quality = 0;
    }
    // Default: 이미지를 읽고 쓸 수 있게 설정
    // IMMUTABLE: 읽기만 할 수 있음
    // DYNAMIC:  cpu가 쓸 수 있다고 함.
    // STAGING: gpu -> cpu ?
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0;
    depthStencilBufferDesc.MiscFlags = 0;

    // device 인터페이스를 통해서 텍스처2D 메모리 만드는것
    if (FAILED(m_d3dDevice->CreateTexture2D(
            &depthStencilBufferDesc, 0, m_d3dDepthStencilBuffer.GetAddressOf()))) 
    {
        std::cout << "CreateTexture2D() failed." << std::endl;
    }
    if (FAILED(m_d3dDevice->CreateDepthStencilView(m_d3dDepthStencilBuffer.Get(), 0,
                                                &m_d3dDepthStencilView)))
    {
        std::cout << "CreateDepthStencilView() failed." << std::endl;
    }
    return true;
}



void CheckResult(HRESULT hr, ID3DBlob *errorBlob)
{
    if (FAILED(hr)) 
    {
        // 파일이 없을 경우
        if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)   
        {
            cout << "File not found." << endl;
        }

        // 에러 메시지가 있으면 출력
        if (errorBlob) 
        {
            cout << "Shader compile error\n" << (char*)errorBlob->GetBufferPointer() << endl;
        }
    }
}

void AppBase::CreateVertexShaderAndInputLayout(
    const wstring &filename,
    const vector<D3D11_INPUT_ELEMENT_DESC> &inputElements,
    ComPtr<ID3D11VertexShader> &vertexShader,
    ComPtr<ID3D11InputLayout> &inputLayout) 
{

    // 임시로 데이터 저장할 공간
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 쉐이더의 시작점의 이름이 "main"인 함수로 지정 // 엔트리 지점
    // 버전(모델)은 5
    // D3D_COMPILE_STANDARD_FILE_INCLUDE 추가: 쉐이더에서 include 사용
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
        "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

    CheckResult(hr, errorBlob.Get());

    // device 통해서 버퍼 생성
    m_d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(),
                                 shaderBlob->GetBufferSize(), NULL,
                                 &vertexShader);

    // 어떤 데이터가 들어갈지 알려줘야함 IA 단계?
    m_d3dDevice->CreateInputLayout(inputElements.data(),
                                UINT(inputElements.size()),
                                shaderBlob->GetBufferPointer(),
                                shaderBlob->GetBufferSize(), &inputLayout);
}

void AppBase::CreatePixelShader(const wstring &filename,
                                ComPtr<ID3D11PixelShader> &pixelShader) {
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 쉐이더의 시작점의 이름이 "main"인 함수로 지정
    // D3D_COMPILE_STANDARD_FILE_INCLUDE 추가: 쉐이더에서 include 사용
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
        "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

    CheckResult(hr, errorBlob.Get());

    m_d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(),
                                shaderBlob->GetBufferSize(), NULL,
                                &pixelShader);
}

void AppBase::CreateIndexBuffer(const std::vector<uint32_t> &indices,
                                ComPtr<ID3D11Buffer> &indexBuffer) 
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
    bufferDesc.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
    bufferDesc.StructureByteStride = sizeof(uint32_t);

    D3D11_SUBRESOURCE_DATA indexBufferData = {0};
    indexBufferData.pSysMem = indices.data();
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    m_d3dDevice->CreateBuffer(&bufferDesc, &indexBufferData,
                           indexBuffer.GetAddressOf());
}

void AppBase::CreateTexture(
    const std::string filename, ComPtr<ID3D11Texture2D> &texture,
    ComPtr<ID3D11ShaderResourceView> &textureResourceView)
{

    int width, height, channels;

    unsigned char *img =
        stbi_load(filename.c_str(), &width, &height, &channels, 0);

    // assert(channels == 4);

    // 4채널로 만들어서 복사
    std::vector<uint8_t> image;
    image.resize(width * height * 4);
    for (size_t i = 0; i < width * height; i++) {
        for (size_t c = 0; c < 3; c++) {
            image[4 * i + c] = img[i * channels + c];
        }
        image[4 * i + 3] = 255;
    }

    // Create texture.
    D3D11_TEXTURE2D_DESC txtDesc = {};
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.MipLevels = txtDesc.ArraySize = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.Usage = D3D11_USAGE_IMMUTABLE;
    txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = image.data();
    initData.SysMemPitch = txtDesc.Width * sizeof(uint8_t) * 4;
    // initData.SysMemSlicePitch = 0;

    m_d3dDevice->CreateTexture2D(&txtDesc, &initData, texture.GetAddressOf());
    m_d3dDevice->CreateShaderResourceView(texture.Get(), nullptr,
                                       textureResourceView.GetAddressOf());
}

void AppBase::CreateCubemapTexture(
    const wchar_t *filename,
    ComPtr<ID3D11ShaderResourceView> &textureResourceView) 
{

    ComPtr<ID3D11Texture2D> texture;

    // https://github.com/microsoft/DirectXTK/wiki/DDSTextureLoader
    auto hr = CreateDDSTextureFromFileEx(
        m_d3dDevice.Get(), filename, 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE, // 큐브맵용 텍스춰
        DDS_LOADER_FLAGS(false), (ID3D11Resource **)texture.GetAddressOf(),
        textureResourceView.GetAddressOf(), nullptr);

    if (FAILED(hr)) {
        std::cout << "CreateDDSTextureFromFileEx() failed" << std::endl;
    }
}

} // namespace FEFE