#include "pch.h"
#include "SerializeTransform.h"

namespace
{
    Float3 parse_vec3(const toml::array& arr, Float3 def = {0, 0, 0})
    {
        Float3 f = def;
        if (arr.size() >= 3)
        {
            f.x = arr[0].value_or(def.x);
            f.y = arr[1].value_or(def.y);
            f.z = arr[2].value_or(def.z);
        }

        return f;
    }
}

namespace TY
{
    SerializeTransform SerializeTransform::Deserialize(const toml::table& tbl)
    {
        SerializeTransform st;
        if (auto* arr = tbl["transform"].as_array())
        {
            if (arr->size() >= 3)
            {
                if (auto* pos = (*arr)[0].as_array())
                    st.position = parse_vec3(*pos, {0, 0, 0});
                if (auto* rot = (*arr)[1].as_array())
                    st.rotation = parse_vec3(*rot, {0, 0, 0});
                if (auto* scale = (*arr)[2].as_array())
                    st.scale = parse_vec3(*scale, {1, 1, 1});
            }
        }

        return st;
    }

    toml::table SerializeTransform::serialize() const
    {
        return toml::table{
            {"transform", toml::array{
                toml::array{position.x, position.y, position.z},
                toml::array{rotation.x, rotation.y, rotation.z},
                toml::array{scale.x, scale.y, scale.z},
            }}
        };
    }

}
