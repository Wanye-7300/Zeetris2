#include "scheduled_frame_stamp.h"

#include <spdlog/spdlog.h>

bool ScheduledFrameStamp::times_up_(const size_t &frame_stamp_count) const {
    return frame_stamp_count >= frame_stamp_ + duration_;
}

bool ScheduledFrameStamp::on_update(const size_t &frame_stamp_count) {
    switch (state_) {
        case ScheduledState::Inactive:
            break;
        case ScheduledState::Active:
            if (times_up_(frame_stamp_count)) {
                spdlog::debug("Times up at frame {}", frame_stamp_count);
                state_ = ScheduledState::Inactive;
                return true;
            }
            break;
        case ScheduledState::Loop:
            if (times_up_(frame_stamp_count)) {
                spdlog::debug("Times up at frame {}, looping", frame_stamp_count);
                frame_stamp_ = frame_stamp_count;
                if (next_duration_.has_value()) {
                    duration_ = next_duration_.value();
                    next_duration_ = std::nullopt;
                }
                return true;
            }
            break;
    }
    return false;
}

void ScheduledFrameStamp::set_active(const size_t &frame_stamp_count) {
    state_ = ScheduledState::Active;
    frame_stamp_ = frame_stamp_count;
}

bool ScheduledFrameStamp::is_active() const {
    return state_ == ScheduledState::Active || state_ == ScheduledState::Loop;
}
