#include "pch.h"
#include "DebugPlayground.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "DebugUI.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Scene.h"
#include "TY/ShapeDrawer.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/System.h"
#include "TY/TextureResource.h"
#include "TY/Utils.h"
#include "TY_Extension/SerializeTransform.h"

using namespace Combat;

using namespace TY;

namespace
{
    Array<RectF> SliceRectByLength(const RectF& rect, float length, Direction2 dir)
    {
        const auto& pos = rect.pos;
        const auto& size = rect.size;
        const float edgeLength = dir == Direction2::Horizontal ? size.x : size.y;
        const float padding = std::fmod(edgeLength, length) / 2;
        const int count = static_cast<int>(edgeLength / length);

        Array<RectF> result(count);
        for (int i = 0; i < count; ++i)
        {
            if (dir == Direction2::Horizontal)
            {
                result[i] = RectF{Float2{pos.x + padding + length * i, pos.y}, Float2{length, size.y}};
            }
            else
            {
                result[i] = RectF{Float2{pos.x, pos.y + padding + length * i}, Float2{size.x, length}};
            }
        }

        return result;
    }
}

namespace
{
    void transformListEditorDemo(Array<SerializeTransform>& transformList)
    {
        const auto backgroundRegion = RectF{Scene::Rect().stretched(-10).bl(), Alignment9::BottomLeft, Size{400, 800}};

        Shape2D::RoundRect{backgroundRegion}
            .setColor(ColorPalette::EditorBackground)
            .setOutline({1.0f, ColorPalette::GrayOrange})
            .pushAuto();

        constexpr float lineLength = 28.0f;
        auto [headerRegion, contentRegion] = backgroundRegion.separate(lineLength, Direction4::Up);

        Shape2D::RoundRect{headerRegion.stretched(-1)}
            .setColor(ColorF32{ColorPalette::DarkOrange} * 1.05f)
            .pushAuto();

        Shape2D_Text::MPlus1_16_Bitmap(U"Transform Editor")
            .setPosition(headerRegion.stretched(-5).middleLeft(), Alignment9::MiddleLeft)
            .pushAuto();

        const auto [operationRegion, contentRegion2] =
            contentRegion.stretched(-5).stretched(5, Direction4::Up).separate(lineLength, Direction4::Down);

        const auto [sliderRegion, listRegion] = contentRegion2.separate(10, Direction4::Left);

        // Shape2D::RoundRect{sliderRegion.stretched(0, -1)}
        //     .setColor(ColorF32{"#4F4F4F"})
        //     .pushAuto();

        const auto listRects = SliceRectByLength(listRegion.stretched(-5), lineLength, Direction2::Vertical);

        static int s_listStart = 0;
        DebugUI::ListSlider(
            s_listStart, listRects.size(), transformList.size(), sliderRegion.stretched(0, -1), contentRegion2);

        for (int i = 0; i < listRects.size(); ++i)
        {
            const int index = i + s_listStart;
            if (index >= transformList.size()) break;

            const auto& r = listRects[i];
            Shape2D::RoundRect{r.stretched(-1)}
                .setColor(ColorF32{0.15})
                .pushAuto();

            const auto& e = transformList[index];
            Shape2D_Text::MPlus1_16_Bitmap(ToUtf32(e.tag))
                .setPosition(r.stretched(-10).middleLeft(), Alignment9::MiddleLeft)
                .pushAuto();
        }

        const auto operationRects = SliceRectByLength(operationRegion, operationRegion.w / 4, Direction2::Horizontal);

        if (DebugUI::Button(operationRects[0].stretched(-1), U"Add"))
        {
            transformList.push_back(SerializeTransform{
                "NewEntity",
                {0, 0, 0},
                {0, 0, 0},
                {1, 1, 1}
            });
        }

        // Shape2D_Text::MPlus1_24_Bitmap(U"デバッグ機能 ABC abc 012")
        //     .setPosition(Float2{10, 100})
        //     .setSize(100)
        //     .pushAuto();

        ShapeDrawer::Global().draw();

        if (ImGui::Begin("Transform Editor"))
        {
            // Add ボタン
            if (ImGui::Button("Add Entity"))
            {
                transformList.push_back(SerializeTransform{
                    "NewEntity",
                    {0, 0, 0},
                    {0, 0, 0},
                    {1, 1, 1}
                });
            }

            ImGui::Separator();

            // リスト表示
            for (size_t i = 0; i < transformList.size();)
            {
                auto& e = transformList[i];
                ImGui::PushID(static_cast<int>(i)); // ID衝突防止

                // タグ名を編集
                char buffer[128];
                std::snprintf(buffer, sizeof(buffer), "%s", e.tag.c_str());
                if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
                {
                    e.tag = buffer;
                }

                ImGui::DragFloat3("Position", &e.position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &e.rotation.x, 0.5f);
                ImGui::DragFloat3("Scale", &e.scale.x, 0.1f);

                // Remove ボタン
                if (ImGui::Button("Remove"))
                {
                    transformList.erase(transformList.begin() + i);
                    ImGui::PopID();
                    continue; // eraseしたので i を進めない
                }

                ImGui::Separator();
                ImGui::PopID();
                ++i;
            }
        }

        ImGui::End();
    }

    // -----------------------------------------------

    const std::string transformsFilepath = "asset/edit/transforms.toml";

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

    SerializeTransform from_toml(const toml::table& tbl)
    {
        SerializeTransform st;
        st.tag = tbl["tag"].value_or<std::string>("");

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

    Array<SerializeTransform> loadTransformList()
    {
        Array<SerializeTransform> result;

        try
        {
            auto tbl = toml::parse_file(transformsFilepath);

            if (auto* arr = tbl["transforms"].as_array())
            {
                for (auto&& node : *arr)
                {
                    if (auto* t = node.as_table())
                        result.push_back(from_toml(*t));
                }
            }
        }
        catch (const toml::parse_error& err)
        {
            std::cerr << "TOML parse error: " << err.description() << " at " << err.source().begin << "\n";
        }

        return result;
    }

    toml::table to_toml(const SerializeTransform& st)
    {
        return toml::table{
            {"tag", st.tag},
            {
                "transform",
                toml::array{
                    toml::array{st.position.x, st.position.y, st.position.z},
                    toml::array{st.rotation.x, st.rotation.y, st.rotation.z},
                    toml::array{st.scale.x, st.scale.y, st.scale.z},
                },
            }
        };
    }

    void saveTransformList(const Array<SerializeTransform>& transformList)
    {
        toml::array transforms;
        for (auto& e : transformList)
        {
            transforms.push_back(to_toml(e));
        }

        toml::table root{
            {"transforms", transforms}
        };

        std::ofstream file(transformsFilepath);
        file << root;
    }

    // -----------------------------------------------

    struct LambertLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct PhongLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
        alignas(16) Float3 eyePosition{};
        alignas(16) Float3 ambientColor{};
    };

    struct Skydome_b4
    {
        alignas(16) ColorF32 topColor;
        alignas(16) ColorF32 bottomColor;
        float sphereRadius{};
    };

    struct Pose
    {
        Float3 position{};
        Quaternion rotation{}; // Euler angles in radians

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotated(rotation)
                   .translated(position);
        }

        Float3 eulerAngles() const
        {
            return rotation.eulerAngles();
        }
    };

    TextureResource makeGroundPlane(
        const Size& size, int lineSpacing, const UnifiedColor& lineColor, const UnifiedColor& backColor)
    {
        Image image{size, backColor};
        const ColorU8 backColor2 = backColor.toColorU8().multiplied(0.9f);

        for (int x = 0; x < size.x; x += 2)
        {
            for (int y = 0; y < size.y; y += 2)
            {
                image[Point{x, y}] = backColor2;
            }
        }

        const Size padding = (size % lineSpacing) / 2;

        for (int x = padding.x; x < size.x; x += lineSpacing)
        {
            for (int y = 0; y < size.y; y++)
            {
                image[Point{x, y}] = lineColor;
            }
        }

        for (int y = padding.y; y < size.y; y += lineSpacing)
        {
            for (int x = 0; x < size.x; x++)
            {
                image[Point{x, y}] = lineColor;
            }
        }

        return TextureResource{image};
    }

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct DebugPlayground::Impl : ActorBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    ModelDrawer m_playerDrawer{};

    Pose m_playerPose{};

    SimpleCamera3D m_camera{};

    Mat4x4 m_projectionMat{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<SerializeTransform> m_transformList{};

    void init()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(Asset_shader::skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        ConstantBufferWrapper<PhongLight_b4> phongLight{};

        phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        phongLight->eyePosition = m_camera.eyePosition();
        phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};
        phongLight.upload();

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Asset_model::tie_fighter)
            .setShader(Asset_shader::phong)
            .setCbv10AndLater({phongLight})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(Asset_shader::model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_transformList = loadTransformList();
    }

    void update() override
    {
        m_children.updateEach();

        // ウィンドウ内でドック可能にする
        // ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // -----------------------------------------------

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        {
            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                fovFarZ
            );

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::End();
        }

        // -----------------------------------------------

        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

        if (not ImGui::IsAnyItemActive())
        {
            if (not KeyShift.pressed())
            {
                m_camera.transformBySimpleInput();
            }
            else
            {
                const auto matrix = m_playerPose.getMatrix();
                const auto forward = matrix.forward();
                const auto right = matrix.right();
                const auto up = matrix.up();

                const auto moveInput = SimpleInput::GetPlayerMovement3D();
                constexpr auto speed = 10.0f;
                m_playerPose.position += forward * moveInput.z * speed * System::DeltaTime();
                m_playerPose.position += right * moveInput.x * speed * System::DeltaTime();
                m_playerPose.position += up * moveInput.y * speed * System::DeltaTime();

                const auto rotateInput = SimpleInput::GetCameraRotation();
                constexpr auto rotationSpeed = 2.0f;
                m_playerPose.rotation *= Quaternion::RotateY(rotateInput.x * rotationSpeed * System::DeltaTime());
            }
        }

        // -----------------------------------------------

        m_skydomeDrawer.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_playerDrawer.draw();

        m_groundPlaneDrawer.draw();

        // -----------------------------------------------

        transformListEditorDemo(m_transformList);
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();

        saveTransformList(m_transformList);
    }
};

namespace Combat
{
    DebugPlayground::DebugPlayground()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void DebugPlayground::init()
    {
        p_impl->init();
    }

    std::shared_ptr<ActorBase> DebugPlayground::asActor() const
    {
        return p_impl;
    }
}
