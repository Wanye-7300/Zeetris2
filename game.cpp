#include "game.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <print>
#include <random>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>


bool block::operator==(const block &block) const {
    return this->points == block.points && this->anchor == block.anchor;
}

kickwall *get_kickwall(const BlockType block_type) {
    switch (block_type) {
        case BlockType::I:
            return &kickwall_I;
        case BlockType::O:
            return &kickwall_O;
        case BlockType::None:
        case BlockType::Unknown:
            throw std::invalid_argument("Invalid block type");
        default:
            return &kickwall_JLSTZ;
    }
}

bool GameData::move(block &block, const point<int32_t> offset, const bool refresh_shadow) {
    temp_block = block;
    for (auto &[y, x]: block.points) {
        y += offset.y;
        x += offset.x;
    }
    block.anchor.y += offset.y;
    block.anchor.x += offset.x;
    if (!check(block)) {
        block = temp_block;
        return false;
    }
    if (refresh_shadow) {
        this->refresh_shadow();
    }
    return true;
}

bool GameData::rotate(block &block, RotationState &block_rotation_state, const BlockType block_type,
                      const RotationState rotation, const bool refresh_shadow) {
    auto [center_y, center_x] = rotating_centers[block_type];
    center_y += static_cast<float>(block.anchor.y);
    center_x += static_cast<float>(block.anchor.x);

    temp_block = block;

    for (auto &[y, x]: block.points) {
        // 致敬经典
        const float tmp_y = static_cast<float>(x) - center_x;
        const float tmp_x = static_cast<float>(y) - center_y;
        switch (rotation) {
            case RotationState::Left:
                y = static_cast<int32_t>(center_y + tmp_y);
                x = static_cast<int32_t>(center_x - tmp_x);
                break;
            case RotationState::Right:
                y = static_cast<int32_t>(center_y - tmp_y);
                x = static_cast<int32_t>(center_x + tmp_x);
                break;
            default:
                throw std::invalid_argument("Invalid rotation type");
        }
    }

    RotationState new_state = block_rotation_state;

    switch (rotation) {
        case RotationState::Left:
            new_state = static_cast<RotationState>((static_cast<int>(new_state) + 3) % 4);
            break;
        case RotationState::Right:
            new_state = static_cast<RotationState>((static_cast<int>(new_state) + 1) % 4);
        default:
            break;
    }

    const auto temp_rotated_block = block;

    for (auto &list_offset = get_kickwall(block_type)->at(std::pair{block_rotation_state, new_state});
         auto &[offset_x, offset_y]: list_offset) {
        for (auto &[block_y, block_x]: block.points) {
            block_y += offset_y;
            block_x += offset_x;
        }

        if (check(block)) {
            block.anchor.y += offset_y;
            block.anchor.x += offset_x;
            goto success;
        }

        block = temp_rotated_block;
    }

    block = temp_block;
    return false;
success:
    block_rotation_state = new_state;
    if (refresh_shadow) {
        this->refresh_shadow();
    }
    return true;
}

void GameData::new_bag(std::mt19937 &rng, const size_t bag_count) {
    for (size_t i = 0; i < bag_count; i++) {
        std::vector list{BlockType::I, BlockType::J, BlockType::L, BlockType::O,
                         BlockType::S, BlockType::Z, BlockType::T};
        std::ranges::shuffle(list, rng);
        next_queue.append_range(list);
    }
}

void GameData::new_block(const BlockType block_type) {
    BlockType type;
    [[likely]]
    if (block_type == BlockType::None) {
        if (next_queue.empty()) {
            // 连预览块都不给我，我怎么生成啊？
            throw std::runtime_error("Member `next_queue` is empty.");
        }
        type = next_queue.front();
        next_queue.pop_front();
    } else {
        type = block_type;
    }

    current_block = blocks[static_cast<size_t>(type)];
    for (auto &[y, x]: current_block.points) {
        y += 20;
        x += 3;
    }
    current_block.anchor = {20, 3};
    current_block_type = type;
    current_block_rotation_state = RotationState::Zero;
    can_exchange_hold = true;
    on_land = false;
    scheduled_frame_stamp_down.set_frame_stamp(*logical_frame_count);
    scheduled_frame_stamp_down.set_state(ScheduledState::Loop);
    this->refresh_shadow();
}

void GameData::exchange_hold() {
    if (!can_exchange_hold) {
        return;
    }

    const auto temp = current_block_type;
    current_block_type = hold_block_type;
    hold_block_type = temp;

    // 如果 current_block_type 是 None 的话（意味着 hold 本来是 None），传给 new_block 生成一个新的；
    // 如果不是，也传给它。
    new_block(current_block_type);

    can_exchange_hold = !can_exchange_hold;
}

bool GameData::check(block &block) {
    return std::ranges::all_of(block.points, [this](auto point) {
        return 0 <= point.x && point.x < 10 && 0 <= point.y && point.y < 22 &&
               matrix[point.y][point.x] == BlockType::None;
    });
}

void GameData::refresh_shadow() {
    shadow_block = current_block;
    // 一直让它下落，直到下落不了了为止
    while (this->move(shadow_block, {-1, 0}, false))
        ;
}

void GameData::lock() {
    for (auto &[y, x]: current_block.points) {
        matrix[y][x] = current_block_type;
    }
    new_block();
}

void GameData::hard_drop() {
    current_block = shadow_block;
    lock();
}

void GameData::logic_frame([[maybe_unused]] const boost::system::error_code &error_code,
                           boost::asio::steady_timer *timer, std::atomic_size_t *logical_frame_count, std::mt19937 &rng,
                           const std::shared_ptr<Keyboard> &keyboard, std::mutex *keyboard_mutex,
                           std::atomic_flag *flag_thread_quit) {
    timer->expires_after(boost::asio::chrono::nanoseconds(1000000000 / 60));
    using sf::Keyboard::Scancode;

    bool move_changed = false;
    if (keyboard->is_key_pressed(Scancode::Left)) {
        state_move_left = state_move_right + 1;
        move_changed = true;
    } else if (!keyboard->is_key_pressing(Scancode::Left) && state_move_left != 0) {
        state_move_left = 0;
        move_changed = true;
    }
    if (keyboard->is_key_pressed(Scancode::Right)) {
        state_move_right = state_move_left + 1;
        move_changed = true;
    } else if (!keyboard->is_key_pressing(Scancode::Right) && state_move_right != 0) {
        state_move_right = 0;
        move_changed = true;
    }

    if (move_changed) {
        if (state_move_left == state_move_right) {
            scheduled_frame_stamp_move.set_state(ScheduledState::Inactive);
            move_offset.x = 0;
        } else {
            spdlog::debug("Move started at frame {}", logical_frame_count->load());
            scheduled_frame_stamp_move.set_state(ScheduledState::Loop);
            scheduled_frame_stamp_move.set_frame_stamp(*logical_frame_count);
            scheduled_frame_stamp_move.set_duration(GameConfig::DAS);
            scheduled_frame_stamp_move.set_next_duration(std::make_optional(GameConfig::ARR));
            move_offset.x = state_move_left > state_move_right ? -1 : 1;
            // 按下的那一瞬间也是要移动的
            move(current_block, move_offset);
        }
    }

    if (keyboard->is_key_pressed(Scancode::Z)) {
        rotate(current_block, current_block_rotation_state, current_block_type, RotationState::Left);
    }
    if (keyboard->is_key_pressed(Scancode::X)) {
        rotate(current_block, current_block_rotation_state, current_block_type, RotationState::Right);
    }
    if (keyboard->is_key_pressed(Scancode::Space)) {
        hard_drop();
    }
    if (keyboard->is_key_pressed(Scancode::LShift)) {
        exchange_hold();
    }
    if (keyboard->is_key_pressed(Scancode::Down)) {
        scheduled_frame_stamp_down.set_duration(GameConfig::soft_down_delay);
    } else if (!keyboard->is_key_pressing(Scancode::Down)) {
        scheduled_frame_stamp_down.set_duration(GameConfig::down_delay);
    }
    {
        std::lock_guard guard(*keyboard_mutex);
        keyboard->update();
    }

    if (scheduled_frame_stamp_move.on_update(*logical_frame_count)) {
        move(current_block, move_offset);
    }

    // 着地 / 锁定逻辑
    if (shadow_block == current_block && !scheduled_frame_stamp_lock.is_active()) {
        scheduled_frame_stamp_lock.set_active(*logical_frame_count);
        scheduled_frame_stamp_down.set_state(ScheduledState::Inactive);
    } else if (shadow_block != current_block && scheduled_frame_stamp_lock.is_active()) {
        scheduled_frame_stamp_lock.set_state(ScheduledState::Inactive);
        scheduled_frame_stamp_down.set_frame_stamp(*logical_frame_count);
        scheduled_frame_stamp_down.set_state(ScheduledState::Loop);
    }
    if (scheduled_frame_stamp_lock.on_update(*logical_frame_count)) {
        lock();
    }

    // 下落逻辑
    if (scheduled_frame_stamp_down.on_update(*logical_frame_count)) {
        move(current_block, {-1, 0});
    }

    // 处理消行逻辑
    {
        size_t count = 0;
        for (size_t y = 0; y < height_main + height_buffer; y++) {
            if (std::ranges::all_of(matrix[y], [](auto block_type) { return block_type != BlockType::None; })) {
                std::ranges::fill(matrix[y], BlockType::None);
                count++;
            } else if (count != 0) {
                for (size_t x = 0; x < width; x++) {
                    matrix[y - count][x] = matrix[y][x];
                }
                std::ranges::fill(matrix[y], BlockType::None);
            }
        }
        if (count > 0) {
            clear_line_count += count;
            spdlog::info("Cleared {} lines, {} in total", count, clear_line_count);
            refresh_shadow();
        }
    }

    // 预览块序列不足时，生成新的包
    if (next_queue.size() == 7) {
        new_bag(rng);
    }

    logical_frame_count->fetch_add(1);

    if (flag_thread_quit->test()) {
        return;
    }

    using namespace std::literals;

    timer->async_wait(boost::bind(&GameData::logic_frame, this, boost::asio::placeholders::error, timer,
                                  logical_frame_count, rng, keyboard, keyboard_mutex, flag_thread_quit));
}

Game::Game(sf::RenderWindow *render_window, std::shared_ptr<sf::Font> font) {
    game_data_ = std::make_shared<GameData>(&logical_frame_count_);
    keyboard_ = std::make_shared<Keyboard>();
    font_ = std::move(font);
    render_window_ = render_window;
}

void Game::handle_game_logic(std::mt19937 &rng, std::atomic_flag *flag_thread_quit) {
    using namespace std::literals;

    logical_thread_ = std::move(std::thread{[this, rng, flag_thread_quit]() {
        boost::asio::io_context io_context;
        boost::asio::steady_timer asio_steady_timer{io_context, boost::asio::chrono::nanoseconds(1000000000 / 60)};
        asio_steady_timer.async_wait(boost::bind(&GameData::logic_frame, game_data_, boost::asio::placeholders::error,
                                                 &asio_steady_timer, &logical_frame_count_, rng, keyboard_,
                                                 &keyboard_mutex_, flag_thread_quit));
        spdlog::info("Game logic thread has been started");
        io_context.run();
        spdlog::info("Game logic thread has been quit");
        flag_thread_quit->test_and_set();
        flag_thread_quit->notify_all();
    }});

    logical_thread_.detach();
}

void Game::run() {
    using namespace std::literals; // 启用后缀，例如 24h, 1ms, 1s 之类的

    const auto update_vertices = [this](sf::Vertex *begin, const size_t offset, const size_t position_y,
                                        const size_t position_x, const sf::Color color) {
        auto [screen_width, screen_height] = this->render_window_->getSize();
        const auto offset_width = static_cast<float>(screen_width) / 2.f -
                                  static_cast<float>(GameData::width) / 2.f * GameConfig::block_size;
        const auto offset_height = static_cast<float>(screen_height) / 2.f -
                                   static_cast<float>(GameData::height_main) / 2.f * GameConfig::block_size;

        const auto y = static_cast<int32_t>(position_y);
        const auto x = static_cast<int32_t>(position_x);

        // 注意这里 sf::Vector2f 先是 x 再是 y 的，和项目里通行的记法正好相反
        begin[offset + 0].position = sf::Vector2f{
                static_cast<float>(x + 0) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 1) * GameConfig::block_size + offset_height};
        begin[offset + 1].position = sf::Vector2f{
                static_cast<float>(x + 0) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 0) * GameConfig::block_size + offset_height};
        begin[offset + 2].position = sf::Vector2f{
                static_cast<float>(x + 1) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 1) * GameConfig::block_size + offset_height};

        begin[offset + 3].position = sf::Vector2f{
                static_cast<float>(x + 1) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 1) * GameConfig::block_size + offset_height};
        begin[offset + 4].position = sf::Vector2f{
                static_cast<float>(x + 0) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 0) * GameConfig::block_size + offset_height};
        begin[offset + 5].position = sf::Vector2f{
                static_cast<float>(x + 1) * GameConfig::block_size + offset_width,
                static_cast<float>(GameData::height_main - y - 0) * GameConfig::block_size + offset_height};

        for (size_t idx = offset; idx < offset + 6; idx++) {
            begin[idx].color = color;
        }
    };

    std::array<sf::Vertex, (GameData::height_main + GameData::height_buffer) * GameData::width * 6> vertices_matrix;
    std::array<sf::Vertex, 4 * 6> vertices_current_block;
    std::array<sf::Vertex, 4 * 6> vertices_shadow_block;
    std::array<sf::Vertex, 5> vertices_rotating_center;

    sf::Text text_fps{*font_, L"Unknown fps", 24};
    sf::Text text_frame_count{*font_, L"frame_count_: 0", 24};
    sf::Text text_logical_frame_count{*font_, L"logical_frame_count_: 0", 24};
    sf::Text text_rotation{*font_, L"rotation: 0", 24};
    text_frame_count.setPosition({0, text_fps.getGlobalBounds().position.y + text_fps.getGlobalBounds().size.y});
    text_logical_frame_count.setPosition(
            {0, text_frame_count.getGlobalBounds().position.y + text_frame_count.getGlobalBounds().size.y});
    text_rotation.setPosition({0, text_logical_frame_count.getGlobalBounds().position.y +
                                          text_logical_frame_count.getGlobalBounds().size.y});

    std::atomic_flag flag_thread_quit{};

    // 初始化游戏数据
    auto rd = std::random_device();
    auto rng = std::mt19937(rd());
    game_data_->new_bag(rng, 2);
    game_data_->new_block();

    handle_game_logic(rng, &flag_thread_quit);

    while (render_window_->isOpen()) {
        const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

        // vvv 处理游戏逻辑
        while (const std::optional event = render_window_->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                render_window_->close();
                flag_thread_quit.test_and_set();
                return;
            }

            std::lock_guard guard(keyboard_mutex_);
            keyboard_->update_event(*event);
        }
        // keyboard_->update();
        // ^^^ 处理游戏逻辑

        // vvv 计算 vertices
        for (size_t y = 0; y < GameData::height_main + GameData::height_buffer; y++) {
            for (size_t x = 0; x < GameData::width; x++) {
                const size_t offset = (y * GameData::width + x) * 6;
                update_vertices(vertices_matrix.data(), offset, y, x, block_colors[game_data_->matrix[y][x]]);
            }
        }

        for (size_t idx = 0; idx < game_data_->current_block.points.size(); idx++) {
            auto &[y, x] = game_data_->current_block.points[idx];
            update_vertices(vertices_current_block.data(), idx * 6, y, x, block_colors[game_data_->current_block_type]);
        }

        for (size_t idx = 0; idx < game_data_->shadow_block.points.size(); idx++) {
            auto &[y, x] = game_data_->shadow_block.points[idx];
            update_vertices(vertices_shadow_block.data(), idx * 6, y, x, sf::Color{255, 255, 255, 196});
        }

        {
            // -这是什么？ -是用来显示旋转中心的。 -原来是这样啊？
            auto [screen_width, screen_height] = this->render_window_->getSize();
            const auto offset_width = static_cast<float>(screen_width) / 2.f -
                                      static_cast<float>(GameData::width) / 2.f * GameConfig::block_size;
            const auto offset_height = static_cast<float>(screen_height) / 2.f -
                                       static_cast<float>(GameData::height_main) / 2.f * GameConfig::block_size;
            auto [center_y, center_x] = rotating_centers[game_data_->current_block_type];
            center_y += static_cast<float>(game_data_->current_block.anchor.y - 0.5);
            center_x += static_cast<float>(game_data_->current_block.anchor.x + 0.5);
            center_y = (static_cast<float>(GameData::height_main) - center_y - 1.f) * GameConfig::block_size;
            center_x = center_x * GameConfig::block_size;
            vertices_rotating_center[0].position = {center_x - 5.f + offset_width, center_y - 5.f + offset_height};
            vertices_rotating_center[1].position = {center_x - 5.f + offset_width, center_y + 5.f + offset_height};
            vertices_rotating_center[2].position = {center_x + 5.f + offset_width, center_y + 5.f + offset_height};
            vertices_rotating_center[3].position = {center_x + 5.f + offset_width, center_y - 5.f + offset_height};
            vertices_rotating_center[4].position = {center_x - 5.f + offset_width, center_y - 5.f + offset_height};
        }
        // ^^^ 计算 vertices

        render_window_->clear();
        render_window_->draw(text_fps);
        render_window_->draw(text_frame_count);
        render_window_->draw(text_logical_frame_count);
        render_window_->draw(text_rotation);
        render_window_->draw(vertices_matrix.data(), vertices_matrix.size(), sf::PrimitiveType::Triangles);
        render_window_->draw(vertices_current_block.data(), vertices_current_block.size(),
                             sf::PrimitiveType::Triangles);
        render_window_->draw(vertices_rotating_center.data(), vertices_rotating_center.size(),
                             sf::PrimitiveType::LineStrip);
        render_window_->draw(vertices_shadow_block.data(), vertices_shadow_block.size(), sf::PrimitiveType::Triangles);
        render_window_->display();

        // 帧结束，自增
        frame_count_++;

        const std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
        text_fps.setString(std::format(L"{} fps", 1s / (end - start)));
        text_frame_count.setString(std::format(L"frame_count_: {}", frame_count_));
        text_logical_frame_count.setString(
                std::format(L"logical_frame_count_: {}", static_cast<size_t>(logical_frame_count_)));
        text_rotation.setString(
                std::format(L"rotation: {}", static_cast<int>(game_data_->current_block_rotation_state)));
    }
}
