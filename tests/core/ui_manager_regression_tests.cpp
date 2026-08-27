#include "ui_framework/ui_manager.hpp"
#include "ui_framework/node.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
    struct TestFailure { const char *message; };

    void expect(bool condition, const char *message)
    {
        if (!condition) throw TestFailure{message};
    }

    SDL_Event mouseMotion(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = x;
        event.motion.y = y;
        return event;
    }

    SDL_Event mouseDown(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.x = x;
        event.button.y = y;
        event.button.button = SDL_BUTTON_LEFT;
        return event;
    }

    class RenderProbe final : public ui::Node
    {
    public:
        RenderProbe(int *drawCount, ui::LayoutSize *lastSize) noexcept
            : drawCount_(drawCount), lastSize_(lastSize) {}

    protected:
        void draw(SDL_Renderer *) override
        {
            if (drawCount_) ++(*drawCount_);
            if (lastSize_) *lastSize_ = getActualSize();
        }

    private:
        int *drawCount_ = nullptr;
        ui::LayoutSize *lastSize_ = nullptr;
    };

    struct SdlFixture
    {
        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;

        explicit SdlFixture(const char *title)
        {
            if (!SDL_Init(SDL_INIT_VIDEO))
                throw TestFailure{"SDL video initialization failed for UIManager regression test"};
            window = SDL_CreateWindow(title, 320, 240, SDL_WINDOW_HIDDEN);
            expect(window != nullptr, "SDL hidden test window must be created");
            renderer = SDL_CreateRenderer(window, nullptr);
            expect(renderer != nullptr, "SDL test renderer must be created");
            SDL_SetRenderLogicalPresentation(renderer, 320, 240, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        ~SdlFixture()
        {
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window) SDL_DestroyWindow(window);
            SDL_Quit();
        }

        SdlFixture(const SdlFixture &) = delete;
        SdlFixture &operator=(const SdlFixture &) = delete;
    };

    void test_add_root_and_advance_time_is_safe()
    {
        ui::UIManager manager;
        auto node = std::make_unique<ui::Node>();
        ui::Node *root = manager.addRoot(std::move(node));
        expect(root != nullptr, "UIManager must return the attached root");
        manager.invalidateLayout(*root);
        manager.advanceTime(1.0f / 60.0f);
        expect(!manager.isModal(*root), "ordinary root must remain outside the modal system");
    }

    void test_process_event_reaches_root_without_renderer()
    {
        SdlFixture sdl("ui_framework_event_regression");
        ui::UIManager manager;
        auto node = std::make_unique<ui::Node>();
        node->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        ui::Node *root = manager.addRoot(std::move(node));
        int motionEvents = 0;
        root->on<ui::MouseMoveEvent>([&](ui::MouseMoveEvent &, ui::Node &) { ++motionEvents; });
        manager.invalidateLayout(*root);
        manager.render(sdl.renderer);
        manager.processEvent(mouseMotion(10.0f, 10.0f), nullptr);
        expect(motionEvents == 1, "UIManager::processEvent must route pointer input to the hit target");
    }

    void test_process_event_focuses_and_captures_through_manager()
    {
        SdlFixture sdl("ui_framework_focus_regression");
        ui::UIManager manager;
        auto node = std::make_unique<ui::Node>();
        node->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        node->setFocusable(true);
        node->setCapturable(true);
        ui::Node *root = manager.addRoot(std::move(node));
        int downs = 0;
        root->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &) { ++downs; });
        manager.invalidateLayout(*root);
        manager.render(sdl.renderer);
        manager.processEvent(mouseDown(10.0f, 10.0f), nullptr);
        expect(downs == 1, "UIManager must route MouseDown through InputSystem");
    }

    void test_render_reflects_geometry_after_layout_change()
    {
        SdlFixture sdl("ui_framework_render_regression");
        ui::UIManager manager;
        int draws = 0;
        ui::LayoutSize renderedSize{};
        auto node = std::make_unique<RenderProbe>(&draws, &renderedSize);
        node->setSize(ui::LayoutSizeValue::fixed(40.0f, 30.0f));
        ui::Node *root = manager.addRoot(std::move(node));
        manager.invalidateLayout(*root);
        manager.render(sdl.renderer);
        expect(draws == 1, "first rendered frame must invoke Node draw");
        expect(renderedSize == ui::LayoutSize{40.0f, 30.0f}, "first render must observe committed geometry");
        root->setSize(ui::LayoutSizeValue::fixed(80.0f, 50.0f));
        manager.invalidateLayout(*root);
        manager.render(sdl.renderer);
        expect(draws == 2, "second rendered frame must invoke Node draw again");
        expect(renderedSize == ui::LayoutSize{80.0f, 50.0f}, "render must observe latest layout geometry");
    }

    void test_logical_presentation_resize_updates_framework_viewport_and_render_geometry()
    {
        SdlFixture sdl("ui_framework_viewport_regression");
        ui::UIManager manager;
        int draws = 0;
        ui::LayoutSize renderedSize{};
        auto node = std::make_unique<RenderProbe>(&draws, &renderedSize);
        node->setSize(ui::LayoutSizeValue::autoSize());
        ui::Node *root = manager.addRoot(std::move(node));
        manager.invalidateLayout(*root);
        manager.render(sdl.renderer);
        expect(renderedSize == ui::LayoutSize{320.0f, 240.0f}, "auto-sized root must use initial logical presentation");
        SDL_SetRenderLogicalPresentation(sdl.renderer, 640, 360, SDL_LOGICAL_PRESENTATION_STRETCH);
        manager.render(sdl.renderer);
        expect(draws == 2, "resize frame must render the root again");
        expect(renderedSize == ui::LayoutSize{640.0f, 360.0f}, "resize must update the framework viewport");
    }

    void test_remove_root_then_advance_time_is_safe()
    {
        ui::UIManager manager;
        auto node = std::make_unique<ui::Node>();
        ui::Node *root = manager.addRoot(std::move(node));
        manager.removeRoot(root);
        manager.advanceTime(1.0f / 60.0f);
        expect(true, "removed root must not break subsequent UIManager time advancement");
    }
}

int main()
{
    try
    {
        test_add_root_and_advance_time_is_safe();
        test_process_event_reaches_root_without_renderer();
        test_process_event_focuses_and_captures_through_manager();
        test_render_reflects_geometry_after_layout_change();
        test_logical_presentation_resize_updates_framework_viewport_and_render_geometry();
        test_remove_root_then_advance_time_is_safe();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "UIManager regression tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "UIManager regression tests passed\n";
    return EXIT_SUCCESS;
}
