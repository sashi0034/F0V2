#include "pch.h"
#include "AssertObject.h"

#include <stdexcept>

#include "System.h"
#include "Utils.h"

namespace
{
}

namespace ZG
{
#if _DEBUG
    AssertObject::AssertObject(std::string_view errorMessage, const std::source_location& location) :
        errorMessage(errorMessage),
        location(location)
    {
    }
#else
    AssertObject::AssertObject(std::string_view errorMessage) :
        errorMessage(errorMessage)
    {
    }
#endif

    void AssertObject::throwError() const
    {
        const std::string errorWithId =
            std::string{errorMessage} + " (" + std::to_string(index) + ")";
#if _DEBUG
        const auto message = ToUtf16(errorWithId);
        const auto filename = ToUtf16(location.file_name());
        const std::wstring output =
            message + L"\n" + filename + L":" + std::to_wstring(location.line());

        System::ModalError(output);
#endif

        if (not errorMessage.empty()) throw std::runtime_error(errorWithId);
        else throw std::runtime_error("An error occurred");
    }

    AssertWin32 AssertWin32::operator|(HRESULT result) const
    {
        if (FAILED(result)) throwError();

        auto next = *this;
        next.index++;
        return next;
    }

    AssertNotNull AssertNotNull::operator|(const void* ptr) const
    {
        if (ptr == nullptr) throwError();

        auto next = *this;
        next.index++;
        return next;
    }

    AssertTrue AssertTrue::operator|(bool condition) const
    {
        if (not condition) throwError();

        auto next = *this;
        next.index++;
        return next;
    }
}
