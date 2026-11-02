#pragma once

#include "Flawy/Assets/MeshData.hpp"

namespace Flawy::Import
{
    class MeshWelder final
    {
    public:
        MeshWelder() = delete;

        static void Weld(Assets::MeshData& oMesh);
    };
}
