#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    class JsonWriter final
    {
    public:
        void BeginObject();
        void EndObject();
        void BeginArray();
        void EndArray();
        void Key(std::string_view svKey);
        void String(std::string_view svValue);
        void Number(float fValue);
        void Integer(int64_t iValue);
        void Bool(bool bValue);

        [[nodiscard]] const std::string& GetText() const noexcept;

    private:
        void BeginValue();
        void AppendEscaped(std::string_view svText);

        std::string m_sText;
        std::vector<bool> m_vFirstInScope;
        bool m_bAfterKey = false;
    };
}
