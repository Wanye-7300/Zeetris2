#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <map>


/// 键盘状态
enum class KeyState {
    Releasing = 0,
    /// 这个也归入到 is_key_pressing，仅因为要区分是否是刚刚按下才设定这两个状态。
    Pressed = 1,
    Pressing = 2,
};

/// 键盘
class Keyboard {
    /// 键盘状态的 map
    std::map<sf::Keyboard::Scancode, KeyState> key_state_;

public:
    Keyboard() = default;
    ~Keyboard() = default;

    /// 根据传入的事件更新键盘状态。
    /// @param event 传入的事件
    void update_event(const sf::Event &event);
    /// 更新键盘状态。
    void update();

    /// 测试一个键是否刚被按下。
    /// @param scancode 要测试的键
    /// @return 键是否刚按下
    [[nodiscard]] bool is_key_pressed(sf::Keyboard::Scancode scancode);
    /// 测试一个键是否被按下。
    /// @param scancode 要测试的键
    /// @return 键是否被按下
    [[nodiscard]] bool is_key_pressing(sf::Keyboard::Scancode scancode);
};

#endif // KEYBOARD_H
