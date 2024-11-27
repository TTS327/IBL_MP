#include "DX11ExampleApp.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <tuple>
#include <vector>

#include "GeometryGenerator.h"

namespace FEFE 
{
        
using namespace std;
using namespace DirectX;


ExampleApp::ExampleApp() : AppBase(), m_BasicPixelConstantBufferData() {}


void ExampleApp::InitializeCubeMapping() 
{
    auto atribumDiffuseFilename = L"./CubemapTextures/Atrium_diffuseIBL.dds";
    auto atribumSpecularFilename = L"./CubemapTextures/Atrium_specularIBL.dds";

    auto stonewallDiffuseFilename = L"./CubemapTextures/Stonewall_diffuseIBL.dds";
    auto stonewallSpecularFilename = L"./CubemapTextures/Stonewall_specularIBL.dds";
    
    auto CloudDiffuseFilename = L"./CubemapTextures/CloudCommon_diffuseIBL.dds";
    auto CloudSpecularFilename = L"./CubemapTextures/CloudCommons_specularIBL.dds"; 
    
    auto saintDiffuseFilename = L"./CubemapTextures/saint_diffuse.dds";
    auto saintSpecularFilename = L"./CubemapTextures/saint_specular.dds";
   
    auto GarageDiffuseFilename = L"./CubemapTextures/Garage_diffuseIBL.dds";
    auto GarageSpecularFilename = L"./CubemapTextures/Garage_specularIBL.dds";

    auto MSPathDiffuseFilename = L"./CubemapTextures/MSPath_diffuseIBL.dds";
    auto MSPathSpecularFilename = L"./CubemapTextures/MSPath_specularIBL.dds";

    // .dds 파일 읽어들여서 초기화 
    CreateCubemapTexture(stonewallDiffuseFilename, m_cubeMapping.diffuseResView);
    CreateCubemapTexture(stonewallSpecularFilename, m_cubeMapping.specularResView);

    m_cubeMapping.cubeMesh = std::make_shared<Mesh>();

    m_BasicVertexConstantBufferData.model = Matrix();
    m_BasicVertexConstantBufferData.view = Matrix();
    m_BasicVertexConstantBufferData.projection = Matrix();
    ComPtr<ID3D11Buffer> vertexConstantBuffer;
    ComPtr<ID3D11Buffer> pixelConstantBuffer;

    AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
                                  m_cubeMapping.cubeMesh->vertexConstantBuffer);
    AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
                                  m_cubeMapping.cubeMesh->pixelConstantBuffer);  

    // 커다란 박스 초기화
    // 세상이 커다란 박스 안에 갇혀 있는 구조
    // D3D11_CULL_MODE::D3D11_CULL_NONE 또는 삼각형 뒤집기
    MeshData cubeMeshData = GeometryGenerator::MakeBox(20.0f);
    std::reverse(cubeMeshData.indices.begin(), cubeMeshData.indices.end());

    AppBase::CreateVertexBuffer(cubeMeshData.vertices,
                                m_cubeMapping.cubeMesh->vertexBuffer);
    m_cubeMapping.cubeMesh->m_indexCount = UINT(cubeMeshData.indices.size());
    AppBase::CreateIndexBuffer(cubeMeshData.indices,
                               m_cubeMapping.cubeMesh->indexBuffer);


    // 쉐이더 초기화
    vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,     
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    

    // 정의한 InputElements배열로 부터 Inputlayout 만듦
    AppBase::CreateVertexShaderAndInputLayout(
        L"CubeMappingVertexShader.hlsl", basicInputElements,
        m_cubeMapping.vertexShader, m_cubeMapping.inputLayout);

    AppBase::CreatePixelShader(L"CubeMappingPixelShader.hlsl",
                               m_cubeMapping.pixelShader);

}

bool ExampleApp::Initialize() 
{

    if (!AppBase::Initialize()) // direct 초기화 코드는 부모 클래스의 Initialize에 구현
        return false;

    // 큐브매핑 준비
    InitializeCubeMapping();

    // Texture sampler 만들기
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the Sample State
    m_d3dDevice->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());

    // Geometry 정의
       
    // Sphere
    vector<MeshData> meshes = {GeometryGenerator::MakeSphere(0.3f, 100, 100)};
    meshes[0].textureFilename = "ojwD8.jpg";

    // 3D model 사용 시 파일 위치 지정
    /* auto meshes =
         GeometryGenerator::ReadFromFile("C:/Users/.../.../IBL_MediaProject_FEFE/MODEL/", "gear.gltf");*/

     /*auto meshes =
         GeometryGenerator::ReadFromFile("C:/Users/.../.../IBL_MediaProject_FEFE/MODEL/dota/", "scene.gltf");*/

    /*auto meshes =
        GeometryGenerator::ReadFromFile("C:/Users/.../.../IBL_MediaProject_FEFE/MODEL/shd/", "High.fbx");*/

    // ConstantBuffer 만들기 (하나 만들어서 공유)
    m_BasicVertexConstantBufferData.model = Matrix();
    m_BasicVertexConstantBufferData.view = Matrix();
    m_BasicVertexConstantBufferData.projection = Matrix();
    ComPtr<ID3D11Buffer> vertexConstantBuffer;
    ComPtr<ID3D11Buffer> pixelConstantBuffer;
    AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
                                  vertexConstantBuffer);
    AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
                                  pixelConstantBuffer);

    for (const auto &meshData : meshes) 
    {
        auto newMesh = std::make_shared<Mesh>();
        AppBase::CreateVertexBuffer(meshData.vertices, newMesh->vertexBuffer);
        newMesh->m_indexCount = UINT(meshData.indices.size());
        AppBase::CreateIndexBuffer(meshData.indices, newMesh->indexBuffer);

        if (!meshData.textureFilename.empty()) 
        {

            cout << meshData.textureFilename << endl;
            AppBase::CreateTexture(meshData.textureFilename, newMesh->texture,
                                   newMesh->textureResourceView);
        }

        newMesh->vertexConstantBuffer = vertexConstantBuffer;
        newMesh->pixelConstantBuffer = pixelConstantBuffer;

        this->m_meshes.push_back(newMesh);
    }

    // POSITION에 float3를 보낼 경우 내부적으로 마지막에 1을 덧붙여서 float4를 만듦
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/display/supplying-default-values-for-texture-coordinates-in-vertex-declaration
    vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    AppBase::CreateVertexShaderAndInputLayout(
        L"BasicVertexShader.hlsl", basicInputElements, m_basicVertexShader,
        m_basicInputLayout);

    AppBase::CreatePixelShader(L"BasicPixelShader.hlsl", m_basicPixelShader);

    // 노멀 벡터 그리기
    // InputLayout은 BasicVertexShader와 같이 사용
    m_normalLines = std::make_shared<Mesh>();

    std::vector<Vertex> normalVertices;
    std::vector<uint32_t> normalIndices;

    // 여러 메쉬의 normal 들을 하나로 합치기
    size_t offset = 0;
    for (const auto &meshData : meshes) 
    {
        for (size_t i = 0; i < meshData.vertices.size(); i++) 
        {

            auto v = meshData.vertices[i];

            v.texcoord.x = 0.0f; // 시작점 표시
            normalVertices.push_back(v);

            v.texcoord.x = 1.0f; // 끝점 표시
            normalVertices.push_back(v);

            normalIndices.push_back(uint32_t(2 * (i + offset)));
            normalIndices.push_back(uint32_t(2 * (i + offset) + 1));
        }
        offset += meshData.vertices.size();
    }

    AppBase::CreateVertexBuffer(normalVertices, m_normalLines->vertexBuffer);
    m_normalLines->m_indexCount = UINT(normalIndices.size());
    AppBase::CreateIndexBuffer(normalIndices, m_normalLines->indexBuffer);
    AppBase::CreateConstantBuffer(m_normalVertexConstantBufferData,
                                  m_normalLines->vertexConstantBuffer);

    AppBase::CreateVertexShaderAndInputLayout(
        L"NormalVertexShader.hlsl", basicInputElements, m_normalVertexShader,
        m_basicInputLayout);
    AppBase::CreatePixelShader(L"NormalPixelShader.hlsl", m_normalPixelShader);

    return true;
}

void ExampleApp::Update(float dt) 
{
    
    using namespace DirectX;

    // 모델의 변환
    m_BasicVertexConstantBufferData.model =
        Matrix::CreateScale(m_modelScaling) *
        Matrix::CreateRotationY(m_modelRotation.y) *
        Matrix::CreateRotationX(m_modelRotation.x) *
        Matrix::CreateRotationZ(m_modelRotation.z) *
        Matrix::CreateTranslation(m_modelTranslation);

    m_BasicVertexConstantBufferData.model =
        m_BasicVertexConstantBufferData.model.Transpose();

    m_BasicVertexConstantBufferData.invTranspose =
        m_BasicVertexConstantBufferData.model;

    m_BasicVertexConstantBufferData.invTranspose.Translation(Vector3(0.0f));

    m_BasicVertexConstantBufferData.invTranspose =
        m_BasicVertexConstantBufferData.invTranspose.Transpose().Invert();

    // 시점 변환
    m_BasicVertexConstantBufferData.view =
        Matrix::CreateRotationY(m_viewRot.y) *
        Matrix::CreateRotationX(m_viewRot.x) *
        Matrix::CreateTranslation(0.0f, 0.0f, 2.0f);

    m_BasicPixelConstantBufferData.eyeWorld = Vector3::Transform(
        Vector3(0.0f), m_BasicVertexConstantBufferData.view.Invert());

    m_BasicVertexConstantBufferData.view =
        m_BasicVertexConstantBufferData.view.Transpose();

    // 프로젝션
    const float aspect = AppBase::GetAspectRatio(); // <- GUI에서 조절
    if (m_usePerspectiveProjection) 
    {
        m_BasicVertexConstantBufferData.projection = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(m_projFovAngleY), aspect, m_nearZ, m_farZ);
    } 
    else
    {
        m_BasicVertexConstantBufferData.projection =
            XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f,
                m_nearZ, m_farZ);
    }
    m_BasicVertexConstantBufferData.projection =
        m_BasicVertexConstantBufferData.projection.Transpose();

    // Constant를 CPU에서 GPU로 복사
    // buffer를 공유하기 때문에 하나만 복사
    if (m_meshes[0]) 
    {
        AppBase::UpdateBuffer(m_BasicVertexConstantBufferData,
                              m_meshes[0]->vertexConstantBuffer);
    }

    // buffer를 공유하기 때문에 하나만 복사
    if (m_meshes[0]) 
    {
        AppBase::UpdateBuffer(m_BasicPixelConstantBufferData,
                              m_meshes[0]->pixelConstantBuffer);
    } 

    // 노멀 벡터 그리기
    if (m_drawNormals && m_drawNormalsDirtyFlag)
    {

        AppBase::UpdateBuffer(m_normalVertexConstantBufferData,
                              m_normalLines->vertexConstantBuffer);

        m_drawNormalsDirtyFlag = false;
    }

    // 큐브매핑을 위한 ConstantBuffers
    m_BasicVertexConstantBufferData.model = Matrix();
    // Transpose()도 생략 가능

    AppBase::UpdateBuffer(m_BasicVertexConstantBufferData,
                          m_cubeMapping.cubeMesh->vertexConstantBuffer);

    m_BasicPixelConstantBufferData.material.diffuse =
        Vector3(m_materialDiffuse);
    m_BasicPixelConstantBufferData.material.specular =
        Vector3(m_materialSpecular);
}

void ExampleApp::Render() 
{

    // RS: Rasterizer stage
    // OM: Output-Merger stage
    // VS: Vertex Shader
    // PS: Pixel Shader
    // IA: Input-Assembler stage

    SetViewport();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_d3dContext->ClearRenderTargetView(m_d3dRenderTargetView.Get(), clearColor);
    m_d3dContext->ClearDepthStencilView(m_d3dDepthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                     1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, m_d3dRenderTargetView.GetAddressOf(),
                                  m_d3dDepthStencilView.Get());
    m_d3dContext->OMSetDepthStencilState(m_d3dDepthStencilState.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    // 큐브매핑
    m_d3dContext->IASetInputLayout(m_cubeMapping.inputLayout.Get());
    m_d3dContext->IASetVertexBuffers(
        0, 1, m_cubeMapping.cubeMesh->vertexBuffer.GetAddressOf(), &stride,
        &offset);
    m_d3dContext->IASetIndexBuffer(m_cubeMapping.cubeMesh->indexBuffer.Get(),
                                DXGI_FORMAT_R32_UINT, 0);
    m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_d3dContext->VSSetShader(m_cubeMapping.vertexShader.Get(), 0, 0);
    m_d3dContext->VSSetConstantBuffers(
        0, 1, m_cubeMapping.cubeMesh->vertexConstantBuffer.GetAddressOf());
    ID3D11ShaderResourceView *views[2] = {m_cubeMapping.diffuseResView.Get(),
                                          m_cubeMapping.specularResView.Get()};
    m_d3dContext->PSSetShaderResources(0, 2, views);
    m_d3dContext->PSSetShader(m_cubeMapping.pixelShader.Get(), 0, 0);
    m_d3dContext->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    m_d3dContext->DrawIndexed(m_cubeMapping.cubeMesh->m_indexCount, 0, 0);

    // 물체들
    m_d3dContext->VSSetShader(m_basicVertexShader.Get(), 0, 0);
    m_d3dContext->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    m_d3dContext->PSSetShader(m_basicPixelShader.Get(), 0, 0);

    if (m_drawAsWire) 
    {
        m_d3dContext->RSSetState(m_d3dWireRasterizerSate.Get());
    } 
    else 
    {
        m_d3dContext->RSSetState(m_d3dSolidRasterizerSate.Get());
    }

    // 버텍스/인덱스 버퍼 설정
    for (const auto &mesh : m_meshes)
    {
        m_d3dContext->VSSetConstantBuffers(
            0, 1, mesh->vertexConstantBuffer.GetAddressOf());

        // 물체 렌더링할 때 큐브맵도 같이 사용
        ID3D11ShaderResourceView *resViews[3] = 
        {
            mesh->textureResourceView.Get(), m_cubeMapping.diffuseResView.Get(),
            m_cubeMapping.specularResView.Get()
        };
        m_d3dContext->PSSetShaderResources(0, 3, resViews);

        m_d3dContext->PSSetConstantBuffers(
            0, 1, mesh->pixelConstantBuffer.GetAddressOf());

        m_d3dContext->IASetInputLayout(m_basicInputLayout.Get());
        m_d3dContext->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(),
                                      &stride, &offset);
        m_d3dContext->IASetIndexBuffer(mesh->indexBuffer.Get(),
                                    DXGI_FORMAT_R32_UINT, 0);
        m_d3dContext->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_d3dContext->DrawIndexed(mesh->m_indexCount, 0, 0);
    }

    // 노멀 벡터 그리기
    if (m_drawNormals) 
    {
        m_d3dContext->VSSetShader(m_normalVertexShader.Get(), 0, 0);

        ID3D11Buffer *pptr[2] = {m_meshes[0]->vertexConstantBuffer.Get(),
                                 m_normalLines->vertexConstantBuffer.Get()};
        m_d3dContext->VSSetConstantBuffers(0, 2, pptr);

        m_d3dContext->PSSetShader(m_normalPixelShader.Get(), 0, 0);
       
        m_d3dContext->IASetVertexBuffers(
            0, 1, m_normalLines->vertexBuffer.GetAddressOf(), &stride, &offset);
        m_d3dContext->IASetIndexBuffer(m_normalLines->indexBuffer.Get(),
                                    DXGI_FORMAT_R32_UINT, 0);
        m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_d3dContext->DrawIndexed(m_normalLines->m_indexCount, 0, 0);
    }
}

void ExampleApp::UpdateGUI() 
{

    ImGui::Checkbox("Use Texture", &m_BasicPixelConstantBufferData.useTexture);
    ImGui::Checkbox("Wireframe", &m_drawAsWire);
    ImGui::Checkbox("Draw Normals", &m_drawNormals);
    if (ImGui::SliderFloat("Normal scale",
                           &m_normalVertexConstantBufferData.scale, 0.0f,
                           1.0f))
    {
        m_drawNormalsDirtyFlag = true;
    }

    ImGui::SliderFloat3("m_modelRotation", &m_modelRotation.x, -3.14f, 3.14f);
    ImGui::SliderFloat3("m_viewRot", &m_viewRot.x, -3.14f, 3.14f);
    ImGui::SliderFloat3("Material FresnelR0",
                        &m_BasicPixelConstantBufferData.material.fresnelR0.x,
                        0.0f, 1.0f);

    ImGui::SliderFloat("Material Diffuse", &m_materialDiffuse, 0.0f, 3.0f);
    ImGui::SliderFloat("Material Specular", &m_materialSpecular, 0.0f, 3.0f);
    ImGui::SliderFloat("Material Shininess",
                       &m_BasicPixelConstantBufferData.material.shininess,
                       0.01f, 20.0f);
}

} // namespace FEFE
 