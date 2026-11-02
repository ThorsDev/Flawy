#include "Renderer/Shader/ShaderInterfaceText.hpp"

#include <format>

namespace Flawy::Renderer
{
    std::string_view ShaderInterfaceText::GetTypeName(ShaderUniformType eType) noexcept
    {
        switch (eType)
        {
        case ShaderUniformType::Vec4:
            return "Vec4";
        case ShaderUniformType::Mat3:
            return "Mat3";
        case ShaderUniformType::Mat4:
            return "Mat4";
        case ShaderUniformType::Sampler:
            return "Sampler";
        }
        return "Unknown";
    }

    std::string ShaderInterfaceText::DescribeUniforms(const ShaderInterface& oInterface)
    {
        std::string sText;
        for (const ShaderUniform& oUniform : oInterface.vUniforms)
        {
            sText.append(sText.empty() ? "" : ", ");
            sText.append(std::format("{} ({}, count {}", oUniform.sName, GetTypeName(oUniform.eType), oUniform.uCount));
            if (oUniform.eType == ShaderUniformType::Sampler)
            {
                sText.append(std::format(", stage {}", oUniform.uTextureStage));
            }
            sText.append(oUniform.bPredefined ? ", predefined)" : ")");
        }
        return sText.empty() ? std::string("none") : sText;
    }
}
