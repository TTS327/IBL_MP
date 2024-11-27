#pragma once

#pragma once

#include <directxtk/SimpleMath.h>
#include <string>
#include <vector>

#include "Vertex.h"

namespace FEFE 
{

using std::vector;

struct MeshData 
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // uint32로 // 16으로 부족할 수 잇
    std::string textureFilename;
};

} // namespace FEFE