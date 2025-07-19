#pragma once
namespace TY
{
    struct VertexInputElement
    {
        std::string semanticName;
        int semanticIndex;
        DXGI_FORMAT format;

        VertexInputElement() = default;

        VertexInputElement(std::string semanticName, int semanticIndex, DXGI_FORMAT format) :
            semanticName(std::move(semanticName)),
            semanticIndex(semanticIndex),
            format(format)
        {
        }

        bool operator==(const VertexInputElement& other) const = default;
    };
}
