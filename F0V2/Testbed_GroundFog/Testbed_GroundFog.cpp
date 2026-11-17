#include "pch.h"
#include "Testbed_GroundFog.h"

#include "Terrain.h"

#include "TY/Gamepad.h"
#include "TY/Graphics3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Mat4x4.h"
#include "TY/Screen.h"
#include "TY/SimpleCamera3D.h"
#include "TY/System.h"
#include "TY/Window.h"

using namespace TY;

namespace
{
    constexpr float FarZ = 500.0f;
}

class Testbed_GroundFog_impl
{
public:
    Testbed_GroundFog_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));
        m_camera.reset(Float3{0.0f, 24.0f, -32.0f}, Float3{0.0f, 2.0f, 0.0f});
    }

    void Update()
    {
        m_camera.transformBySimpleInput();

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());
        Graphics3D::SetProjectionMatrix(Mat4x4::PerspectiveFov(
            75.0_deg,
            Screen::Size().horizontalAspectRatio(),
            0.1f,
            FarZ));

        m_terrain.Draw(m_camera.eyePosition());

#if 1
        // Draw last without depth so the height map is always visible in front.
        ImmediateDrawer::Global()
            .push(Immediate2D::Texture{m_terrain.GetHeightMapDebugTexture()}
                  .resized(Float2{256.0f, 256.0f})
                  .setPosition(Float2{16.0f, 16.0f}))
            .draw();
#endif
    }

private:
    SimpleCamera3D m_camera{};
    Terrain m_terrain{};
};

void Testbed_GroundFog()
{
    Testbed_GroundFog_impl impl{};

    Window::SetTitle("Testbed_GroundFog");

    Screen::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
