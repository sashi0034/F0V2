#include "pch.h"
#include "FileWatcher.h"

#include "System.h"
#include "Utils.h"
#include "detail/EngineCore.h"
#include "detail/IEngineUpdatable.h"

using namespace TY;
using namespace TY::detail;

struct FileWatcher::Impl : IEngineUpdatable, ITimestamp
{
    uint64_t m_timestamp{};
    std::string m_filePath;
    std::filesystem::file_time_type m_lastWriteTime{};

    Impl(std::string filePath)
        : m_filePath(std::move(filePath))
    {
        m_timestamp = System::FrameCount();

        Impl::Update();
    }

    void Update() override
    {
        if (std::filesystem::exists(m_filePath))
        {
            const auto latestWriteTime = std::filesystem::last_write_time(m_filePath);
            if (latestWriteTime != m_lastWriteTime)
            {
                m_timestamp = System::FrameCount();
            }

            m_lastWriteTime = latestWriteTime;
        }
        else
        {
            m_lastWriteTime = {};
        }
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }
};

namespace TY
{
    FileWatcher::FileWatcher(const std::string& path)
        : p_impl(std::make_shared<Impl>(path))
    {
        EngineCore::AddUpdatable(p_impl);
    }

    FileWatcher::FileWatcher(const std::wstring& path)
        : FileWatcher(ToUtf8(path))
    {
    }

    std::shared_ptr<ITimestamp> FileWatcher::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }
}
