#ifndef SCHEDULED_FRAME_STAMP_H
#define SCHEDULED_FRAME_STAMP_H


#include <optional>

/// 一个计划帧的状态
enum class ScheduledState {
    /// 不活动
    Inactive,
    /// 活动中，在 on_update 返回 true 之后返回到 Inactive 状态
    Active,
    /// 循环，在 on_update 返回 true 之后状态不变
    Loop,
};

/// 计划帧
class ScheduledFrameStamp {
    /// 计划开始的帧
    size_t frame_stamp_;
    /// 间隔。在 frame_stamp_count >= frame_stamp_ + duration_ 时表示这次计划帧到了
    size_t duration_;
    /// 计划帧的状态
    ScheduledState state_;
    /// 如果这个 optional 有值，且 state_ 的状态是 Loop，那么就在下次计划帧到的时候把 duration_ 切换到这个值。
    std::optional<size_t> next_duration_;

    [[nodiscard]] bool times_up_(const size_t &frame_stamp_count) const;

public:
    ScheduledFrameStamp() :
        frame_stamp_(0), duration_(0), state_(ScheduledState::Inactive), next_duration_(std::nullopt) {}
    ScheduledFrameStamp(const size_t frame_stamp, const size_t duration) :
        frame_stamp_(frame_stamp), duration_(duration), state_(ScheduledState::Inactive), next_duration_(std::nullopt) {
    }
    ScheduledFrameStamp(const size_t frame_stamp, const size_t duration, const ScheduledState state) :
        frame_stamp_(frame_stamp), duration_(duration), state_(state), next_duration_(std::nullopt) {}
    ScheduledFrameStamp(const size_t frame_stamp, const size_t duration, const ScheduledState state,
                        const size_t next_duration) :
        frame_stamp_(frame_stamp), duration_(duration), state_(state), next_duration_(std::optional{next_duration}) {}

    [[nodiscard]] size_t frame_stamp() const { return frame_stamp_; }
    void set_frame_stamp(const size_t frame_stamp) { frame_stamp_ = frame_stamp; }
    [[nodiscard]] size_t duration() const { return duration_; }
    void set_duration(const size_t duration) { duration_ = duration; }
    [[nodiscard]] ScheduledState state() const { return state_; }
    void set_state(const ScheduledState state) { state_ = state; }
    [[nodiscard]] std::optional<size_t> next_duration() const { return next_duration_; }
    void set_next_duration(const std::optional<size_t> &next_duration) { next_duration_ = next_duration; }

    [[nodiscard]] bool on_update(const size_t &frame_stamp_count);
    void set_active(const size_t &frame_stamp_count);
    bool is_active() const;
};

#endif // SCHEDULED_FRAME_STAMP_H
