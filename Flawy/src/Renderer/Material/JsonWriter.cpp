#include "Renderer/Material/JsonWriter.hpp"

#include <cmath>
#include <format>
#include <stdexcept>

namespace Flawy::Renderer
{
    void JsonWriter::BeginObject()
    {
        BeginValue();
        m_sText.push_back('{');
        m_vFirstInScope.push_back(true);
    }

    void JsonWriter::EndObject()
    {
        m_vFirstInScope.pop_back();
        m_sText.push_back('}');
    }

    void JsonWriter::BeginArray()
    {
        BeginValue();
        m_sText.push_back('[');
        m_vFirstInScope.push_back(true);
    }

    void JsonWriter::EndArray()
    {
        m_vFirstInScope.pop_back();
        m_sText.push_back(']');
    }

    void JsonWriter::Key(std::string_view svKey)
    {
        BeginValue();
        AppendEscaped(svKey);
        m_sText.push_back(':');
        m_bAfterKey = true;
    }

    void JsonWriter::String(std::string_view svValue)
    {
        BeginValue();
        AppendEscaped(svValue);
    }

    void JsonWriter::Number(float fValue)
    {
        if (!std::isfinite(fValue))
        {
            throw std::invalid_argument("JSON cannot represent non-finite numbers");
        }
        BeginValue();
        m_sText.append(std::format("{}", fValue));
    }

    void JsonWriter::Integer(int64_t iValue)
    {
        BeginValue();
        m_sText.append(std::format("{}", iValue));
    }

    void JsonWriter::Bool(bool bValue)
    {
        BeginValue();
        m_sText.append(bValue ? "true" : "false");
    }

    const std::string& JsonWriter::GetText() const noexcept
    {
        return m_sText;
    }

    void JsonWriter::BeginValue()
    {
        if (m_bAfterKey)
        {
            m_bAfterKey = false;
            return;
        }
        if (m_vFirstInScope.empty())
        {
            return;
        }
        if (!m_vFirstInScope.back())
        {
            m_sText.push_back(',');
        }
        m_vFirstInScope.back() = false;
    }

    void JsonWriter::AppendEscaped(std::string_view svText)
    {
        m_sText.push_back('"');
        for (const char cCharacter : svText)
        {
            switch (cCharacter)
            {
            case '"':
                m_sText.append("\\\"");
                break;
            case '\\':
                m_sText.append("\\\\");
                break;
            case '\n':
                m_sText.append("\\n");
                break;
            case '\r':
                m_sText.append("\\r");
                break;
            case '\t':
                m_sText.append("\\t");
                break;
            default:
                if (static_cast<unsigned char>(cCharacter) < 0x20)
                {
                    m_sText.append(std::format("\\u{:04x}", static_cast<unsigned int>(static_cast<unsigned char>(cCharacter))));
                }
                else
                {
                    m_sText.push_back(cCharacter);
                }
                break;
            }
        }
        m_sText.push_back('"');
    }
}
