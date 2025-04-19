#include "keyboard.h"

#include <algorithm>
#include <ranges>

void Keyboard::update_event(const sf::Event &event) {
    if (const auto *key_pressed = event.getIf<sf::Event::KeyPressed>()) {
        // releasing -> pressed
        if (!key_state_.contains(key_pressed->scancode) || key_state_[key_pressed->scancode] == KeyState::Releasing) {
            key_state_[key_pressed->scancode] = KeyState::Pressed;
        }
    } else if (const auto *key_released = event.getIf<sf::Event::KeyReleased>()) {
        // pressed / pressing -> releasing
        key_state_[key_released->scancode] = KeyState::Releasing;
    }
}

void Keyboard::update() {
    // pressed (上一次更新) -> pressing
    std::ranges::for_each(key_state_ | std::views::filter([](const auto &p) { return p.second == KeyState::Pressed; }),
                          [](auto &p) { p.second = KeyState::Pressing; });
}

bool Keyboard::is_key_pressed(const sf::Keyboard::Scancode scancode) {
    return key_state_[scancode] == KeyState::Pressed;
}

bool Keyboard::is_key_pressing(const sf::Keyboard::Scancode scancode) {
    return key_state_[scancode] == KeyState::Pressed || key_state_[scancode] == KeyState::Pressing;
}
