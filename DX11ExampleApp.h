#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "DX11AppBase.h"
#include "GeometryGenerator.h"
#include "Material.h"
#include "CubeMapping.h"

namespace FEFE 
{

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

struct BasicVertexConstantBuffer 
{
    Matrix model;
    Matrix invTranspose;
    Matrix view;
    Matrix projection;
};

static_assert((sizeof(BasicVertexConstantBuffer) % 16) == 0,
              "Constant Buffer size must be 16-byte aligned");



struct BasicPixelConstantBuffer
{
    Vector3 eyeWorld;         // 12
    bool useTexture;          // 4
    Material material;        // 48
    bool useSmoothstep = false; // 4
    float dummy[3];     // 16씩 끝어야함
};

static_assert((sizeof(BasicPixelConstantBuffer) % 16) == 0,
              "Constant Buffer size must be 16-byte aligned");

struct NormalVertexConstantBuffer 
{
    float scale = 0.1f;
    float dummy[3];
};

class ExampleApp : public AppBase // 상속
{
  public:
    ExampleApp();

    virtual bool Initialize() override;
    virtual void UpdateGUI() override;
    virtual void Update(float dt) override;
    virtual void Render() override;

    void InitializeCubeMapping();

  protected:
    ComPtr<ID3D11VertexShader> m_basicVertexShader;
    ComPtr<ID3D11PixelShader> m_basicPixelShader;
    ComPtr<ID3D11InputLayout> m_basicInputLayout;

    // 하나의 3D 모델이 내부적으로 여러개의 메쉬로 구성
    std::vector<shared_ptr<Mesh>> m_meshes;

    ComPtr<ID3D11SamplerState> m_samplerState;

    BasicVertexConstantBuffer m_BasicVertexConstantBufferData;
    BasicPixelConstantBuffer m_BasicPixelConstantBufferData;

    bool m_usePerspectiveProjection = true;
    Vector3 m_modelTranslation = Vector3(0.0f);
    Vector3 m_modelRotation = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_modelScaling = Vector3(1.8f);
    Vector3 m_viewRot = Vector3(0.0f);

    float m_projFovAngleY = 70.0f;
    float m_nearZ = 0.01f;
    float m_farZ = 100.0f;

   /* int m_lightType = 0;
    Light m_lightFromGUI;*/
    float m_materialDiffuse = 1.0f;
    float m_materialSpecular = 1.0f;

    // 노멀 벡터 그리기
    ComPtr<ID3D11VertexShader> m_normalVertexShader;
    ComPtr<ID3D11PixelShader> m_normalPixelShader;
    // ComPtr<ID3D11InputLayout> m_normalInputLayout; // 다른 쉐이더와 같이 사용

    shared_ptr<Mesh> m_normalLines;
    NormalVertexConstantBuffer m_normalVertexConstantBufferData;
    bool m_drawNormals = false;
    bool m_drawNormalsDirtyFlag = false;

    // 큐브 매핑
    CubeMapping m_cubeMapping;

}; 
} // namespace FEFE