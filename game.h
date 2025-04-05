#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <random>


/// 表示一个点 / 一个坐标。由于这个项目的特殊性，先存储 y 再存储 x，要与 SFML 中通行的 (x, y) 存储方式区别开来。
struct point {
    int32_t y, x;
};

/// 一个方块。
using block = std::array<point, 4>;

/// 预设值，表示每个方块对应的 4 个点的坐标。
static constexpr std::array<block, 8> blocks{
        block{
                point{0, 0},
                {0, 0},
                {0, 0},
                {0, 0},
        },

        block{
                point{0, 0},
                {0, 1},
                {0, 2},
                {0, 3},
        }, // I
        block{
                point{0, 0},
                {1, 0},
                {0, 1},
                {0, 2},
        }, // J
        block{
                point{0, 0},
                {0, 1},
                {0, 2},
                {1, 2},
        }, // L
        block{
                point{0, 1},
                {0, 2},
                {1, 1},
                {1, 2},
        }, // O
        block{
                point{0, 0},
                {0, 1},
                {1, 1},
                {1, 2},
        }, // S
        block{
                point{1, 0},
                {1, 1},
                {0, 1},
                {0, 2},
        }, // Z
        block{
                point{0, 0},
                {0, 1},
                {0, 2},
                {1, 1},
        }, // T
};

/// 方块类型。默认应该是 None (0)。
enum class BlockType {
    Unknown = -1,
    None = 0,
    I,
    J,
    L,
    O,
    S,
    Z,
    T,
};

/// 预设值，表示一种方块对应的颜色值
static std::map<BlockType, sf::Color> block_colors{
        {BlockType::None, sf::Color::Transparent},
        {BlockType::I, sf::Color::Cyan},
        {BlockType::J, sf::Color::Blue},
        {BlockType::L, sf::Color{225, 127, 0}},
        {BlockType::O, sf::Color::Yellow},
        {BlockType::S, sf::Color::Green},
        {BlockType::Z, sf::Color::Red},
        {BlockType::T, sf::Color{128, 0, 128}},
};

/// 旋转的方向。
///
/// TODO: 目前并不支持 180 度旋转。
enum class Rotation {
    /// 顺时针
    Clockwise,

    /// 逆时针
    CounterClockwise,
};

/// 用来存储游戏数据的类。一些与游戏数据操作有关的方法也放在这里面，但是不是 static 的。
class GameData {
public:
    /// 当前的方块。注意现在所有的方块都是 4 连方块。
    block current_block{};
    /// 影子方块。这个应该是惰性的，就是只有在场地 / 方块更新时才重新计算这个
    block shadow_block{};
    /// 备用的方块，这个做什么都可以。
    block temp_block{};

    /// 当前方块的类型
    BlockType current_block_type = BlockType::None;
    /// 暂存块的类型
    BlockType hold_block_type = BlockType::None;

    /// 预览序列。这是一个链表 list。
    std::list<BlockType> next_queue{};

    /// 场地的高 (y)
    static constexpr size_t height = 22;
    /// 场地的宽 (x)
    static constexpr size_t width = 10;

    /// 场地 / 矩阵，y = 0 为底，以 matrix[y][x] 的方式访问。
    std::array<std::array<BlockType, width>, height> matrix{};

    /// 是否可以交换暂存块
    bool can_exchange_hold = true;

    GameData() = default;
    ~GameData() = default;

    /// 移动选定的方块。
    /// @param block 选定要移动的方块
    /// @param offset 移动的偏移量
    /// @param refresh_shadow 是否要刷新影子
    /// @return 是否成功移动了方块。若 check() 不成立，那么实际上不会移动方块，并且返回 false。
    bool move(block &block, point offset, bool refresh_shadow = true);

    /// 旋转选定的方块。
    /// @param block 选定要旋转的方块
    /// @param rotation 旋转角度
    /// @param refresh_shadow 是否要刷新影子
    /// @return 是否成功旋转了方块。若 check() 不成立，那么实际上不会旋转方块，并且返回 false。
    bool rotate(block &block, Rotation rotation, bool refresh_shadow = true);

    /// 生成新的一个或若干个包。
    /// @param rng 随机数生成器
    /// @param bag_count 要生成几个包，不填就是一个
    void new_bag(std::mt19937 &rng, size_t bag_count = 1);

    /// 生成新方块。
    /// @param block_type 生成新的方块类型。如果不填默认从 next_queue 中拿第一个下来。
    /// @exception std::runtime_error 当预览块序列为空的时候，抛出这个 exception。
    void new_block(BlockType block_type = BlockType::None);

    /// 交换暂存块。
    void exchange_hold();

    /// 检查一个方块的位置是否合法。
    /// @param block 选定要检查的方块
    /// @return 检查是否通过
    bool check(block &block);

    /// 刷新影子方块。
    void refresh_shadow();
};

/// 游戏主类。
class Game {
    /// 游戏数据
    std::shared_ptr<GameData> game_data_;
    /// 字体，由 main 传过来
    std::shared_ptr<sf::Font> font_;
    /// 要渲染的窗口，从 main 传过来
    sf::RenderWindow *render_window_;

    /// 渲染相关选项：方块大小
    ///
    /// TODO: 放到一个单独的 Config 类中
    static constexpr float block_size = 25.f;

public:
    Game() = delete;

    explicit Game(sf::RenderWindow *render_window, std::shared_ptr<sf::Font> font);

    ~Game() = default;

    /// 运行游戏。
    void run();
};


#endif // GAME_H
