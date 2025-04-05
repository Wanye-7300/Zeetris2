#include "game.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <stdexcept>


bool GameData::move(block &block, const point offset, const bool refresh_shadow) {
    temp_block = block;
    for (auto &[y, x]: block) {
        y += offset.y;
        x += offset.x;
    }
    if (!check(block)) {
        block = temp_block;
        return false;
    }
    if (refresh_shadow) {
        this->refresh_shadow();
    }
    return true;
}

bool GameData::rotate(block &block, Rotation rotation, bool refresh_shadow) {
    // TODO: 实现 rotate 函数
    throw std::logic_error("Not implemented");
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
    current_block_type = type;
    for (auto &[y, x]: current_block) {
        y += 20;
        x += 3;
    }
    this->refresh_shadow();
}

void GameData::exchange_hold() {
    const auto temp = current_block_type;
    current_block_type = hold_block_type;
    hold_block_type = temp;

    // 如果 current_block_type 是 None 的话（意味着 hold 本来是 None），传给 new_block 生成一个新的；
    // 如果不是，也传给它。
    new_block(current_block_type);
}

bool GameData::check(block &block) {
    return std::ranges::all_of(block, [this](auto point) {
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

Game::Game(sf::RenderWindow *render_window, std::shared_ptr<sf::Font> font) {
    game_data_ = std::make_shared<GameData>();
    font_ = std::move(font);
    render_window_ = render_window;
}

void Game::run() {
    using namespace std::literals; // 启用后缀，例如 24h, 1ms, 1s 之类的

    const auto update_vertices = [](sf::Vertex *begin, const size_t offset, const size_t y, const size_t x,
                                    const sf::Color color) {
        // 注意这里 sf::Vector2f 先是 x 再是 y 的，和项目里通行的记法正好相反
        begin[offset + 0].position = sf::Vector2f{static_cast<float>(x + 0) * block_size,
                                                  static_cast<float>(GameData::height - y - 1) * block_size};
        begin[offset + 1].position = sf::Vector2f{static_cast<float>(x + 0) * block_size,
                                                  static_cast<float>(GameData::height - y - 0) * block_size};
        begin[offset + 2].position = sf::Vector2f{static_cast<float>(x + 1) * block_size,
                                                  static_cast<float>(GameData::height - y - 1) * block_size};

        begin[offset + 3].position = sf::Vector2f{static_cast<float>(x + 1) * block_size,
                                                  static_cast<float>(GameData::height - y - 1) * block_size};
        begin[offset + 4].position = sf::Vector2f{static_cast<float>(x + 0) * block_size,
                                                  static_cast<float>(GameData::height - y - 0) * block_size};
        begin[offset + 5].position = sf::Vector2f{static_cast<float>(x + 1) * block_size,
                                                  static_cast<float>(GameData::height - y - 0) * block_size};

        for (size_t idx = offset; idx < offset + 6; idx++) {
            begin[idx].color = color;
        }
    };

    std::array<sf::Vertex, GameData::height * GameData::width * 6> vertices_matrix;
    std::array<sf::Vertex, 4 * 6> vertices_current_block;
    sf::Text text_fps{*font_, L"", 24};

    // TODO: 移除下面的两行，本来是用来调试的
    game_data_->matrix[0][0] = BlockType::I;
    game_data_->matrix[21][0] = BlockType::I;

    // 初始化游戏数据
    auto rd = std::random_device();
    auto rng = std::mt19937(rd());
    game_data_->new_bag(rng, 2);
    game_data_->new_block();

    while (render_window_->isOpen()) {
        const std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

        while (const std::optional event = render_window_->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                render_window_->close();
                return;
            }
            [[likely]] if (const auto *key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (key_pressed->scancode) {
                    case sf::Keyboard::Scancode::A:
                        game_data_->move(game_data_->current_block, {0, -1});
                        break;
                    case sf::Keyboard::Scancode::D:
                        game_data_->move(game_data_->current_block, {0, 1});
                        break;
                    default:
                        break;
                }
            }
        }

        // vvv 计算 vertices
        for (size_t y = 0; y < GameData::height; y++) {
            for (size_t x = 0; x < GameData::width; x++) {
                const size_t offset = (y * GameData::width + x) * 6;
                update_vertices(vertices_matrix.data(), offset, y, x, block_colors[game_data_->matrix[y][x]]);
            }
        }

        for (size_t idx = 0; idx < game_data_->current_block.size(); idx++) {
            auto &[y, x] = game_data_->current_block[idx];
            update_vertices(vertices_current_block.data(), idx * 6, y, x, block_colors[game_data_->current_block_type]);
        }
        // ^^^ 计算 vertices

        render_window_->clear();
        render_window_->draw(vertices_matrix.data(), vertices_matrix.size(), sf::PrimitiveType::Triangles);
        render_window_->draw(vertices_current_block.data(), vertices_current_block.size(),
                             sf::PrimitiveType::Triangles);
        render_window_->draw(text_fps);
        render_window_->display();

        const std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
        text_fps.setString(std::format(L"{} fps", 1s / (end - start)));
    }
}
