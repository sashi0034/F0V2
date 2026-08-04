#include "pch.h"
#include "GamepadMapping.h"

#include "Logger.h"
#include "../../external/toml.h"

using namespace TY;

namespace
{
    constexpr GamepadMapping defaultMapping{};
}

namespace TY
{
    void GamepadMapping::writeToTomlFile(const std::string& path) const
    {
        toml::table t;

        t.insert("a", a);
        t.insert("b", b);
        t.insert("x", x);
        t.insert("y", y);
        t.insert("lb", lb);
        t.insert("rb", rb);
        t.insert("lt", lt);
        t.insert("rt", rt);
        t.insert("menu", menu);
        t.insert("view", view);
        t.insert("axis_lx", axis_lx);
        t.insert("axis_ly", axis_ly);
        t.insert("axis_rx", axis_rx);
        t.insert("axis_ry", axis_ry);
        t.insert("axis_trigger", axis_trigger);
        t.insert("axis_trigger_inverted", axis_trigger_inverted);

        // TODO: ディレクトリが存在しない場合の対処
        try
        {
            std::ofstream stream(path);
            if (not stream)
            {
                LogError("GamepadMapping::writeToTomlFile(): Failed to open file '{}'", path);
                return;
            }

            stream << t;
        }
        catch (const std::exception& e)
        {
            LogError("GamepadMapping::writeToTomlFile(): Exception: {}", e.what());
        }
    }

    GamepadMapping GamepadMapping::FromTomlFile(const std::string& path)
    {
        toml::table t;
        try
        {
            t = toml::parse_file(path);
        }
        catch (const toml::parse_error& err)
        {
            LogError("GamepadMapping::FromTomlFile(): Failed to open or parse TOML file at '{}': {}",
                     path,
                     err.description());
            return defaultMapping;
        }

        GamepadMapping m;
        m.a = t["a"].value_or(defaultMapping.a);
        m.b = t["b"].value_or(defaultMapping.b);
        m.x = t["x"].value_or(defaultMapping.x);
        m.y = t["y"].value_or(defaultMapping.y);
        m.lb = t["lb"].value_or(defaultMapping.lb);
        m.rb = t["rb"].value_or(defaultMapping.rb);
        m.lt = t["lt"].value_or(defaultMapping.lt);
        m.rt = t["rt"].value_or(defaultMapping.rt);
        m.menu = t["menu"].value_or(defaultMapping.menu);
        m.view = t["view"].value_or(defaultMapping.view);
        m.axis_lx = t["axis_lx"].value_or(defaultMapping.axis_lx);
        m.axis_ly = t["axis_ly"].value_or(defaultMapping.axis_ly);
        m.axis_rx = t["axis_rx"].value_or(defaultMapping.axis_rx);
        m.axis_ry = t["axis_ry"].value_or(defaultMapping.axis_ry);
        m.axis_trigger = t["axis_trigger"].value_or(defaultMapping.axis_trigger);
        m.axis_trigger_inverted = t["axis_trigger_inverted"].value_or(defaultMapping.axis_trigger_inverted);
        return m;
    }
}
