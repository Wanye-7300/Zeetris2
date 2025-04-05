/// Zeetris 2: 一个由现代 C++ 构建、完全现代的俄罗斯方块 第二代。

#include <SFML/Graphics.hpp>
#include <print>

#include "game.h"

int main() {
    std::println("Hello Zeetris 2!");
    std::println("Loading fonts...");
    sf::Font unifont;
    if (!unifont.openFromFile("assets/unifont-16.0.02.otf")) {
        throw std::runtime_error("Failed to load unifont");
    }
    const auto font = std::make_shared<sf::Font>(unifont);

    std::println("Creating sf::RenderWindow...");
    sf::RenderWindow render_window{sf::VideoMode{sf::Vector2u{1366, 768}}, L"Zeetris 2"};
    render_window.setFramerateLimit(60);

    Game game{&render_window, font};
    game.run();

    return 0;
}
