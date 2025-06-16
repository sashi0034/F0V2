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
    GamepadMapping GamepadMapping::FromTomlFile(const std::string& path)
    {
        toml::table t = toml::parse_file(path);
        if (t.empty())
        {
            LogError.writeln(std::format("GamepadMapping::FromToml: Failed to parse TOML file at '{}'", path));
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
        return m;
    }
}
