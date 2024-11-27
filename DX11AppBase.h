#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <wrl.h> // ComPtr

namespace FEFE 
{

using Microsoft::WRL::ComPtr;
using std::shared_ptr;
using std::vector;
using std::wstring;

// 모든 예제들이 공통적으로 사용할 기능들을 가지고 있는
// 부모 클래스
class AppBase 
{
  public:
    AppBase();
    virtual ~AppBase();

    float GetAspectRatio() const;

    int Run();

    // 자식에서 사용할 것들 
    virtual bool Initialize();
    virtual void UpdateGUI() = 0;
    virtual void Update(float dt) = 0;
    virtual void Render() = 0;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y){};
    virtual void OnMouseUp(WPARAM btnState, int x, int y){};
    virtual void OnMouseMove(WPARAM btnState, int x, int y){};

  protected: // 상속 받은 클래스에서도 접근 가능
    bool InitMainWindow();
    bool InitDirect3D();
    bool InitGUI();

    void SetViewport();
    bool CreateRenderTargetView();
    bool CreateDepthBuffer();
    void CreateVertexShaderAndInputLayout(
        const wstring &filename,
        const vector<D3D11_INPUT_ELEMENT_DESC> &inputElements,
        ComPtr<ID3D11VertexShader> &vertexShader,
        ComPtr<ID3D11InputLayout> &inputLayout);
    void CreatePixelShader(const wstring &filename,
                           ComPtr<ID3D11PixelShader> &pixelShader);
    void CreateIndexBuffer(const vector<uint32_t> &indices,
                           ComPtr<ID3D11Buffer> &indexBuffer);


    // 템플릿 // 
    template <typename T_VERTEX>
    void CreateVertexBuffer(const vector<T_VERTEX> &vertices,
                            ComPtr<ID3D11Buffer> &vertexBuffer) 
    {
        // vertices를 gpu에 옮겨서 버퍼를 만듦
        // D3D11_USAGE enumeration (d3d11.h)
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(bufferDesc));
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
        bufferDesc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size());
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0; 
        bufferDesc.StructureByteStride = sizeof(T_VERTEX); // 버텍스하나가 가지고있는 사이즈 넣어줌

        D3D11_SUBRESOURCE_DATA vertexBufferData = {0};  // MS 예제에서 초기화하는 방식
        vertexBufferData.pSysMem = vertices.data();     // cpu데이터의 첫포인터, 가리키는 곳부터 보내라
        vertexBufferData.SysMemPitch = 0;           
        vertexBufferData.SysMemSlicePitch = 0;


        // 버퍼 만듦
        const HRESULT hr = m_d3dDevice->CreateBuffer(&bufferDesc, &vertexBufferData, vertexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            std::cout << "CreateBuffer() failed. " << std::hex << hr << std::endl;
        };
    }

    template <typename T_CONSTANT>
    void CreateConstantBuffer(const T_CONSTANT &constantBufferData,
                              ComPtr<ID3D11Buffer> &constantBuffer) 
    {
        D3D11_BUFFER_DESC cbDesc;
        cbDesc.ByteWidth = sizeof(constantBufferData);
        // 물체를 매프레임당 회전시킨다고 할 때 회전행렬이 계속 바뀜
        // 매 프레임당 한번씩 gpu로보내줘야할때 한번만들어놓은 버퍼에 보내기만 하면 됨 
        // 매번 버퍼를 새로 만들 필요 없음. 데이터만 다시 보내주자
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;                 
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;     // CPU가 GPU메모리에 쓰기를 할 수 있다.
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;

        // Fill in the subresource data.
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &constantBufferData;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;

        auto hr = m_d3dDevice->CreateBuffer(&cbDesc, &initData,
                                         constantBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            std::cout << "CreateConstantBuffer() CreateBuffer failed()."
                      << std::endl;
        }
    }

    template <typename T_DATA>
    void UpdateBuffer(const T_DATA &bufferData, ComPtr<ID3D11Buffer> &buffer) 
    {

        if (!buffer) 
        {
            std::cout << "UpdateBuffer() buffer was not initialized."
                      << std::endl;
        }

        D3D11_MAPPED_SUBRESOURCE ms;
        m_d3dContext->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
        memcpy(ms.pData, &bufferData, sizeof(bufferData));
        m_d3dContext->Unmap(buffer.Get(), NULL);
    }

    void CreateTexture(const std::string filename,
                       ComPtr<ID3D11Texture2D> &texture,
                       ComPtr<ID3D11ShaderResourceView> &textureResourceView);
    void CreateCubemapTexture(const wchar_t *filename,
                              ComPtr<ID3D11ShaderResourceView> &texResView);

  public:
    // 변수 이름 붙이는 규칙은 VS DX11/12 기본 템플릿을 따릅니다.
    // 다만 변수 이름을 줄이기 위해 d3d는 생략했습니다.
    // 예: m_d3dDevice -> m_d3dDevice
    int m_screenWidth; // 렌더링할 최종 화면의 해상도
    int m_screenHeight;
    int m_guiWidth = 0;
    HWND m_mainWindow;
    UINT numQualityLevels = 0;

    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<ID3D11DeviceContext> m_d3dContext;
    ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;
    ComPtr<IDXGISwapChain> m_d3dSwapChain;

    ComPtr<ID3D11RasterizerState> m_d3dSolidRasterizerSate;
    ComPtr<ID3D11RasterizerState> m_d3dWireRasterizerSate;
    bool m_drawAsWire = false;

    // Depth buffer 관련
    ComPtr<ID3D11Texture2D> m_d3dDepthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
    ComPtr<ID3D11DepthStencilState> m_d3dDepthStencilState;

    D3D11_VIEWPORT m_d3dScreenViewPort;
};
} // namespace FEFE