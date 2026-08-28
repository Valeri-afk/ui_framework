#include "ui_framework/components/text_input.hpp"

#include "input_system.hpp"
#include "layout_system.hpp"
#include "node_tree.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cassert>
#include <filesystem>
#include <memory>

namespace
{
    std::filesystem::path findTestFont()
    {
        const std::filesystem::path filePath(__FILE__);
        const std::filesystem::path sourceRoot = filePath.parent_path().parent_path().parent_path();
        const std::filesystem::path fromSource = sourceRoot / "chess_client" / "fonts" / "Roboto-Medium.ttf";
        const std::filesystem::path fromWorkingDirectory =
            std::filesystem::current_path() / ".." / "chess_client" / "fonts" / "Roboto-Medium.ttf";
        const std::filesystem::path fromRepositoryRoot =
            std::filesystem::current_path() / "chess_client" / "fonts" / "Roboto-Medium.ttf";

        if (std::filesystem::exists(fromSource))
            return fromSource;
        if (std::filesystem::exists(fromWorkingDirectory))
            return fromWorkingDirectory;
        if (std::filesystem::exists(fromRepositoryRoot))
            return fromRepositoryRoot;
        return {};
    }
}

int main()
{
    assert(TTF_Init());
    const std::filesystem::path fontPath = findTestFont();
    assert(!fontPath.empty());

    TTF_Font *font = TTF_OpenFont(fontPath.string().c_str(), 24.0f);
    assert(font != nullptr);

    ui::NodeTree tree;
    auto input = std::make_unique<ui::TextInput>();
    ui::TextInput *inputPtr = input.get();
    input->setFont(font);
    input->setText("hello");
    input->setPosition({20.0f, 20.0f});
    input->setSize(ui::LayoutSizeValue::fixed(120.0f, 40.0f));
    tree.attachRoot(0, std::move(input));

    ui::LayoutSystem layout;
    layout.setViewportSize({200.0f, 100.0f});
    layout.requestFullLayout(tree);
    layout.processLayoutQueue(tree);

    ui::InputSystem inputSystem;

    SDL_Event down{};
    down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.button = SDL_BUTTON_LEFT;
    down.button.x = 25.0f;
    down.button.y = 25.0f;
    inputSystem.processEvent(down, tree, nullptr);

    assert(inputSystem.focusedNode() == inputPtr);
    assert(inputSystem.capturedNode() == inputPtr);
    assert(inputSystem.pressedNode() == inputPtr);

    SDL_Event move{};
    move.type = SDL_EVENT_MOUSE_MOTION;
    move.motion.x = 500.0f;
    move.motion.y = 500.0f;
    inputSystem.processEvent(move, tree, nullptr);

    assert(inputSystem.capturedNode() == inputPtr);
    assert(inputPtr->getCaretPosition() == 5);
    assert(inputPtr->hasSelection());

    SDL_Event up{};
    up.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.button = SDL_BUTTON_LEFT;
    up.button.x = 500.0f;
    up.button.y = 500.0f;
    inputSystem.processEvent(up, tree, nullptr);

    assert(inputSystem.capturedNode() == nullptr);
    assert(inputSystem.pressedNode() == nullptr);

    TTF_CloseFont(font);
    TTF_Quit();
    return 0;
}
