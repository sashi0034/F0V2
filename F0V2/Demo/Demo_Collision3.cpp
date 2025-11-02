#include "pch.h"
#include "Demo_Collision3.h"

#include "imgui/imgui.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
#include "TY/Gamepad.h"
#include "TY/Graphics3D.h"
#include "TY/InlineComponent.h"
#include "TY/Intersects3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/ModelLoader.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/Transformer3D.h"
#include "TY/TriangleBvh.h"

using namespace TY;

namespace
{
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

    DynamicTexture makeGroundPlane(
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

        return {image};
    }

    constexpr float groundPositionY = -25.0f;

    constexpr float fovFarZ = 1000.0f;

    // -----------------------------------------------

    struct Resource_Demo_Collision3 : IInlineComponent
    {
        struct
        {
            GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

            GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

            // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

            GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

            GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};
        } shaders;

        struct modelData_t
        {
            ModelData toy_terrain{ModelLoader::Load("asset/model/toy_terrain.obj")};
        };

        modelData_t modelData;

        struct models_t
        {
            ModelBuffer toy_terrain;
        } models;

        struct
        {
            ConstantBufferWrapper<PhongLight_b4> phongLight{};
        } cb;

        Resource_Demo_Collision3()
        {
            models.toy_terrain = ModelBuffer{modelData.toy_terrain};
        }
    };

    InlineComponent<Resource_Demo_Collision3> s_resource_Demo_Collision3{};

    Resource_Demo_Collision3& getRsc()
    {
        return s_resource_Demo_Collision3.get();
    }

    // -----------------------------------------------

    struct TerrainObject
    {
        Pose m_pose{};
        ModelDrawer m_drawer;

        Array<IndexedTriangle> m_polygons{};

        TriangleBvh m_bvh{};

        void Init()
        {
            m_drawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(getRsc().models.toy_terrain)
                .setShader(getRsc().shaders.phong)
                .setCbv10AndLater({getRsc().cb.phongLight})
            };

            const auto& modelData = getRsc().modelData.toy_terrain;
            for (int i = 0; i < modelData.shapes.size(); ++i)
            {
                const auto& shape = modelData.shapes[i];
                for (int j = 0; j < shape.indexBuffer.size(); j += 3)
                {
                    const auto& v0 = shape.vertexBuffer[shape.indexBuffer[j + 0]].position;
                    const auto& v1 = shape.vertexBuffer[shape.indexBuffer[j + 1]].position;
                    const auto& v2 = shape.vertexBuffer[shape.indexBuffer[j + 2]].position;
                    m_polygons.push_back(IndexedTriangle{v0, v1, v2, 0});
                }
            }

            m_bvh = TriangleBvh{m_polygons};
        }

        void Draw() const
        {
            m_drawer.uploadWorldMatrix(m_pose.getMatrix()).draw();
        }
    };

    struct TriangleObject
    {
        Triangle3D m_tri{
            Float3{-10, 1, 5},
            Float3{10, 1, 5},
            Float3{0, 10, 5}
        };

        ModelBuffer m_model;
        ModelDrawer m_drawer;

        void Init()
        {
            m_model = ModelBuffer{PrimitiveModel3D::Triangle(m_tri, ColorF32{1.0f, 0.7f, 0.5f})};

            m_drawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(m_model)
                .setShader(getRsc().shaders.phong)
                .setCbv10AndLater({getRsc().cb.phongLight})
            };
        }

        void DebugUI()
        {
            ImGui::Begin("Triangle");

            bool changed = false;
            changed |= ImGui::DragFloat3("v0", &m_tri.p0.x, 0.1f);
            changed |= ImGui::DragFloat3("v1", &m_tri.p1.x, 0.1f);
            changed |= ImGui::DragFloat3("v2", &m_tri.p2.x, 0.1f);

            if (changed)
            {
                Init();
            }

            ImGui::End();
        }
    };

    struct CapsuleObject
    {
        float m_radius = 1;
        float m_height = 2;

        ModelBuffer m_model;
        ModelDrawer m_drawer;
        Float3 m_pos{};
        Float3 m_normalOnGround{0, 1, 0};

        void Init()
        {
            m_model = ModelBuffer{PrimitiveModel3D::Capsule(m_radius, m_height, ColorF32{0.5f, 0.7f, 1.0f})};

            m_drawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(m_model)
                .setShader(getRsc().shaders.phong)
                .setCbv10AndLater({getRsc().cb.phongLight})
            };
        }

        void DebugUI()
        {
            ImGui::Begin("Capsule");

            bool changed = false;
            changed |= ImGui::DragFloat3("Pos", &m_pos.x, 0.05f);

            changed |= ImGui::DragFloat("Radius", &m_radius, 0.1f, 0.1f, 100.0f);
            changed |= ImGui::DragFloat("Height", &m_height, 0.1f, 0.1f, 100.0f);
            if (changed)
            {
                Init();
            }

            if (ImGui::Button("Pos = (0, 10), 0)"))
            {
                m_pos = Float3{0, 10, 0};
            }

            if (ImGui::Button("Pos = (-100, 30, -100)"))
            {
                m_pos = Float3{-100, 30, -100};
            }

            if (ImGui::Button("Y = 30"))
            {
                m_pos.y = 30.0f;
            }

            ImGui::End();
        }
    };

    void drawBvh(
        const TriangleBvh::NodeReference& node, Immediate3D::LineSet& lineSet, std::pair<int, int> targetRange,
        int nest = 0)
    {
        if (not node)
        {
            return;
        }

        if (targetRange.first <= nest && nest <= targetRange.second)
        {
            lineSet.appendAabb(node.aabb());
        }

        if (const auto branch = node.asBranch())
        {
            drawBvh(branch.left(), lineSet, targetRange, nest + 1);
            drawBvh(branch.right(), lineSet, targetRange, nest + 1);
        }
    }

    void drawLeafAabb(
        const TriangleBvh::NodeReference& node, Immediate3D::LineSet& lineSet, int targetIndex,
        int* currentIndex = nullptr)
    {
        if (not node)
        {
            return;
        }

        std::unique_ptr<int> currentIndexPtr{};
        if (not currentIndex)
        {
            currentIndexPtr = std::make_unique<int>(0);
            currentIndex = currentIndexPtr.get();
        }

        if (const auto leaf = node.asLeaf())
        {
            if (*currentIndex == targetIndex)
            {
                lineSet.appendAabb(node.aabb());
            }

            ++(*currentIndex);

            return;
        }

        if (const auto& branch = node.asBranch())
        {
            drawLeafAabb(branch.left(), lineSet, targetIndex, currentIndex);
            drawLeafAabb(branch.right(), lineSet, targetIndex, currentIndex);
        }
    }

    void printBvhLeaf(const TriangleBvh::NodeReference& node, int nest = 0)
    {
        if (nest == 0)
        {
            std::cout << "----------------------------------------------- BVH Leaf Information\n";
        }

        if (not node)
        {
            return;
        }

        if (const auto leaf = node.asLeaf())
        {
            std::cout << std::format("[{}] tris: {}, aabb-volume: {}\n", nest, leaf.triCount(), leaf.aabb().volume());

            return;
        }

        if (const auto branch = node.asBranch())
        {
            printBvhLeaf(branch.left(), nest + 1);
            printBvhLeaf(branch.right(), nest + 1);
        }

        if (nest == 0)
        {
            std::cout << "----------------------------------------------- End BVH Leaf Information\n";
        }
    }
}

struct Demo_Collision3_impl
{
    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    TerrainObject m_terrain{};
    TriangleObject m_triangleObject{};
    CapsuleObject m_capsuleObject{};

    Demo_Collision3_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        resetCamera();

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(getRsc().shaders.skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(getRsc().shaders.model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_terrain.Init();
        m_triangleObject.Init();
        m_capsuleObject.Init();
    }

    void Update()
    {
        static float s_moveSpeed = 1.0f;
        static float s_gravity = 1.0f;
        Float3 moveVector{};
        if (KeyShift.pressed())
        {
            m_camera.transformBySimpleInput();
        }
        else
        {
            const auto input = SimpleInput::GetPlayerMovement3D();
            const auto cameraMat = m_camera.worldMatrix();
            moveVector += cameraMat.right().withY(0).normalized() * input.x * s_moveSpeed;
            moveVector += cameraMat.forward().withY(0).normalized() * input.z * s_moveSpeed;
            moveVector.y -= (KeySpace.pressed() ? -1.0f : s_gravity);

            moveVector *= 10.0f * System::DeltaTime();
        }

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        {
            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Screen::Size().horizontalAspectRatio(),
                0.1f,
                fovFarZ
            );

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        getRsc().cb.phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        getRsc().cb.phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        getRsc().cb.phongLight->eyePosition = m_camera.eyePosition();
        getRsc().cb.phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};

        getRsc().cb.phongLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        // -----------------------------------------------

        m_terrain.Draw();

        static std::pair s_visibleBvhRange{0, 16};
        static int s_visibleBvhLeaf = -1;
        Immediate3D::LineSet lineSet{};

        if (s_visibleBvhLeaf < 0)
        {
            drawBvh(m_terrain.m_bvh.root(), lineSet, s_visibleBvhRange);
            lineSet.setColor(ColorF32{0.3f, 1, 0.3f}).pushAuto();
        }
        else
        {
            drawLeafAabb(m_terrain.m_bvh.root(), lineSet, s_visibleBvhLeaf);
            lineSet.setColor(ColorF32{1.0f, 0.00f, 1.0f}).pushAuto();
        }

        static bool s_moveEnabled = true;

        {
            const Float3 previousPos = m_capsuleObject.m_pos;
            Float3 newPos = previousPos;

            Immediate3D::Line{
                    previousPos,
                    previousPos + Float3{0, 1, 0} * 10
                }.setColor(ColorF32{1.0f, 0.5f, 0.5f})
                 .pushAuto();
            Immediate3D::Line{
                    previousPos,
                    previousPos + m_capsuleObject.m_normalOnGround * 10
                }.setColor(ColorF32{1.0f, 0.0f, 0.0f})
                 .pushAuto();

            if (s_moveEnabled)
            {
                Immediate3D::Line{
                        previousPos,
                        previousPos + moveVector * 10
                    }.setColor(ColorF32{0.5f})
                     .pushAuto();

                newPos = previousPos + moveVector;

                updateCapsulePosition(previousPos, newPos - previousPos);
            }
        }

        {
            // 原点
            m_capsuleObject.m_drawer.uploadWorldMatrix(Mat4x4::Identity()).draw();
        }

        m_capsuleObject.m_drawer.uploadWorldMatrix(Mat4x4::Translate(m_capsuleObject.m_pos)).draw();

        m_capsuleObject.DebugUI();

        Capsule3D testCapsule = Capsule3D::AlongY(
            m_capsuleObject.m_pos, m_capsuleObject.m_height, m_capsuleObject.m_radius);

        for (int i = -5; i <= 5; ++i)
        {
            m_triangleObject.m_drawer.uploadWorldMatrix(Mat4x4::Translate(Float3{0, 0, static_cast<float>(i)})).draw();
        }

        m_triangleObject.DebugUI();

        ImmediateDrawer::Global().draw(); // <-- flush

        bool intersectionTest = Intersects(testCapsule, m_triangleObject.m_tri);

        {
            ImGui::Begin("Intersection Test");

            if (intersectionTest)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Intersection: True");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Intersection: False");
            }

            if (m_invalidParallel == System::FrameCount())
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "m_invalidParallel: True");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "m_invalidParallel: False");
            }

            ImGui::Checkbox("Use BVH", &s_useBvh);

            ImGui::DragInt2("Visible BVH Range", &s_visibleBvhRange.first, 1, 0, 16);
            if (ImGui::Button("Expand Visible BVH Range"))
            {
                s_visibleBvhRange.second++;
            }

            ImGui::DragInt("Visible BVH Leaf (-1: off)", &s_visibleBvhLeaf, 1, -1, m_terrain.m_polygons.size());
            if (ImGui::Button("Next Visible BVH Leaf"))
            {
                s_visibleBvhLeaf++;
            }

            if (ImGui::Button("Print BVH Leaf Info to Console"))
            {
                printBvhLeaf(m_terrain.m_bvh.root());
            }

            ImGui::Text("Triangle Test Count: %d", s_triTestCount);
            s_triTestCount = 0;

            ImGui::Checkbox("Move Enabled", &s_moveEnabled);

            ImGui::DragFloat("Move Speed", &s_moveSpeed, 0.1f, 0.1f, 10.0f);

            ImGui::DragFloat("Gravity", &s_gravity, 0.1f, 0.0f, 10.0f);

            ImGui::End();
        }

        {
            ImGui::Begin("Camera");

            ImGui::Text("Eye Position: (%.2f, %.2f, %.2f)",
                        m_camera.eyePosition().x,
                        m_camera.eyePosition().y,
                        m_camera.eyePosition().z);

            const auto targetPosition = m_camera.targetPosition();
            ImGui::Text("Target Position: (%.2f, %.2f, %.2f)",
                        targetPosition.x,
                        targetPosition.y,
                        targetPosition.z);

            ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)",
                        getRsc().cb.phongLight->lightDirection.x,
                        getRsc().cb.phongLight->lightDirection.y,
                        getRsc().cb.phongLight->lightDirection.z);

            ImGui::Separator();

            if (ImGui::Button("Reset Camera"))
            {
                resetCamera();
            }

            ImGui::End();
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::Text("VRAM Usage: %.2f MB", System::GpuMemoryUsage().estimateLocalUsageInMB());

            ImGui::End();
        }
    }

private:
    void resetCamera()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});
    }

    size_t m_invalidParallel = 0;

    void updateCapsulePosition(const Float3& fromPos, const Float3& moveVector, int nest = 0)
    {
        if (moveVector.lengthSq() < 1e-6f)
        {
            return;
        }

        const auto toPos = fromPos + moveVector;
        const auto [newPos, hitTris] = tryMoveCapsulePosition(fromPos, toPos);

        m_capsuleObject.m_pos = newPos;

        if ((toPos - newPos).lengthSq() < 1e-6f)
        {
            return;
        }

        if (hitTris.has_value())
        {
            const auto& tri = *hitTris;
            const auto triCenter = tri.tri.centroid();
            ImmediateDrawer::Global().push(Immediate3D::Line{
                triCenter,
                triCenter + tri.plane.normal * 10
            }.setColor(ColorF32{1.0f, 0.0f, 1.0f}, ColorF32{0.5f, 0, 0.5f}));

            // 面の法線を採用
            m_capsuleObject.m_normalOnGround = tri.plane.normal;

            const auto n = m_capsuleObject.m_normalOnGround;
            const Float3 r = toPos - m_capsuleObject.m_pos;
            const auto newMoveVector = r - n * r.dot(n);
            if (nest < 3)
            {
                updateCapsulePosition(m_capsuleObject.m_pos, newMoveVector, nest + 1);
            }
        }
    }

    struct HitTri
    {
        float moveDistance;
        Triangle3D tri;
        Plane3D plane;
        Float3 intersection;
        Float3 foot;
    };

    struct MoveResult
    {
        Float3 newPos;
        std::optional<HitTri> tri{};
    };

    inline static bool s_useBvh{true};
    inline static int s_triTestCount{0};

    MoveResult tryMoveCapsulePosition(const Float3& fromPos, const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {toPos, {}};
        }

        const auto moveTestCapsule = Capsule3D{fromPos, toPos, m_capsuleObject.m_radius};

        HitTri hitTri{};
        hitTri.moveDistance = FLT_MAX;
        Float3 newPos = toPos;

        if (s_useBvh)
        {
            const auto hits = m_terrain.m_bvh.queryHits(moveTestCapsule.aabb());
            hits.forEachTriangle([&](const Triangle3D& tri)
            {
                tryMoveCapsulePosition_internal(tri, moveTestCapsule, fromPos, toPos, hitTri, newPos);
            });
        }
        else
        {
            for (const auto& tri : m_terrain.m_polygons)
            {
                tryMoveCapsulePosition_internal(tri, moveTestCapsule, fromPos, toPos, hitTri, newPos);
            }
        }

        return {newPos, hitTri};
    }

    void tryMoveCapsulePosition_internal(
        const Triangle3D& testTri,
        const Capsule3D& moveTestCapsule,
        const Float3& fromPos,
        const Float3& toPos,
        HitTri& hitTri,
        Float3& newPos)
    {
        s_triTestCount++;
        if (Intersects(moveTestCapsule, testTri))
        {
            //            U
            //           /|
            //          / |
            //         /  |
            //        /   |
            //       /    |
            //    T /--r--|
            //     /|     |
            //    / |     |
            //   /  |     |
            //  /   |     |
            // /----------|
            // S          H

            const auto lineST = Line3D::FromPoints(fromPos, toPos);

            auto plane = testTri.asPlane();
            if (Abs(lineST.normalizedDir.dot(plane.normal)) < 0.1f)
            {
                // 移動ベクトルと三角形がほぼ並行の場合
                // TODO: 対策考える
                m_invalidParallel = System::FrameCount();
                return;
            }

            const Float3 S = fromPos;
            float lengthSU{};
            const auto tryU = IntersectsAt(lineST, plane, &lengthSU);
            if (not tryU)
            {
                return;
            }

            const Float3 U = *tryU;
            const Float3 SU = U - S;

            const float distance = plane.signedDistanceFrom(fromPos);
            const Float3 H = S - plane.normal * distance;
            const float lengthSH = Abs(distance);

            const Float3 SH = (H - S);

            const float r = m_capsuleObject.m_radius + 1e-2f;

            const float SUoSH = SU.dot(SH);
            const float lengthST = lengthSU - r * (lengthSU * lengthSH) / Max(1e-30f, SUoSH);

            if (hitTri.moveDistance < lengthST)
            {
                return;
            }

            hitTri.moveDistance = lengthST;
            hitTri.tri = testTri;
            hitTri.plane = plane;
            hitTri.intersection = U;
            hitTri.foot = H;
            newPos = S + lineST.normalizedDir * lengthST;
        }
    }
};

void Demo_Collision3()
{
    Demo_Collision3_impl impl{};

    Screen::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
