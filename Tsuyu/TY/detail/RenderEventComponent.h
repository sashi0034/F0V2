#pragma once

namespace TY::detail
{
    namespace RenderEvent
    {
        class Listener
        {
        public:
            virtual ~Listener()
            {
                m_shouldRemove = true;
            }

            bool shouldRemove() const
            {
                return m_shouldRemove;
            }

            virtual void beforeFlush()
            {
            }

            virtual void afterPresent()
            {
            }

        private:
            bool m_shouldRemove{};
        };

        void AddLister(const std::shared_ptr<Listener>& subscribable);
    }
}
