#pragma once

namespace TY
{
    class IInlineComponent;

    namespace detail
    {
        class InlineComponentId
        {
        public:
            size_t value() const { return m_value; };

            static InlineComponentId Next();

        private:
            size_t m_value{};

            InlineComponentId(size_t value);
        };

        IInlineComponent& FetchInlineComponent(
            InlineComponentId id,
            const std::function<std::unique_ptr<IInlineComponent>()>& initializer);

        template <typename Component> requires std::is_base_of_v<IInlineComponent, Component>
        Component& FetchInlineComponent(InlineComponentId id)
        {
            return static_cast<Component&>(FetchInlineComponent(id, [] { return std::make_unique<Component>(); }));
        }
    }

    /// @brief Marker interface for inline components
    /// @code
    /// struct Component : IInlineComponent
    /// {
    /// };

    class IInlineComponent
    {
    public:
        virtual ~IInlineComponent() = default;
    };

    /// @brief 実行時に生成し、アプリケーション終了と同時に破棄することを意図する静的オブジェクト
    /// @code
    /// InlineComponent<Component> s_component{};
    template <typename Component> requires std::is_base_of_v<IInlineComponent, Component>
    class InlineComponent
    {
    public:
        InlineComponent() : m_id(detail::InlineComponentId::Next()) { return; }

        Component& get() { return static_cast<Component&>(detail::FetchInlineComponent<Component>(m_id)); }
        const Component& get() const { return detail::FetchInlineComponent<Component>(m_id); }

        Component* operator ->() { return &get(); }
        const Component* operator ->() const { return &get(); }

    private:
        detail::InlineComponentId m_id;
    };
}
