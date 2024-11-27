#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxtk/SimpleMath.h>
#include <memory>

#include "Mesh.h"

namespace FEFE 
{

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;

// Water : (0.02, 0.02, 0.02)
// Glass : (0.08, 0.08, 0.08)
// Plastic : (0.05, 0.05, 0.05)
// Gold: (1.0, 0.71, 0.29)
// Silver: (0.95, 0.93, 0.88)
// Copper: (0.95, 0.64, 0.54)
struct Material 
{
    Vector3 ambient = Vector3(0.0f);                 // 12
    float shininess = 0.01f;                         // 4
    Vector3 diffuse = Vector3(0.0f);                 // 12
    float dummy1;                                    // 4
    Vector3 specular = Vector3(1.0f);                // 12
    float dummy2;                                    // 4
    Vector3 fresnelR0 = Vector3(0.95, 0.93, 0.88);   // Copper
    float dummy3;
};

} // namespace FEFE