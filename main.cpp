/// Zeetris 2: 一个由现代 C++ 构建、完全现代的俄罗斯方块 第二代。

#include <SFML/Graphics.hpp>
#include <print>
#include <spdlog/spdlog.h>

#include "game.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Hello Zeetris 2!");
    spdlog::info("Loading fonts...");
    sf::Font unifont;
    if (!unifont.openFromFile("assets/unifont-16.0.02.otf")) {
        throw std::runtime_error("Failed to load unifont");
    }
    const auto font = std::make_shared<sf::Font>(unifont);

    spdlog::info("Creating sf::RenderWindow...");
    sf::RenderWindow render_window{sf::VideoMode{sf::Vector2u{1366, 768}}, L"Zeetris 2"};
    render_window.setFramerateLimit(120);

    Game game{&render_window, font};
    try {
        game.run();
    } catch (const std::exception &exception) {
        std::println(stderr, "Exception occurred:\n{}", exception.what());
    }

    return 0;
}
