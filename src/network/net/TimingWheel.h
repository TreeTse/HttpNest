#pragma once

#include <memory>
#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <deque>

namespace nest
{
    namespace net
    {
        using EntryPtr = std::shared_ptr<void>;
        using WheelEntry = std::unordered_set<EntryPtr>;
        using Wheel = std::deque<WheelEntry>;
        using Wheels = std::vector<Wheel>;
        const int kTimingMinute = 60;
        const int kTimingHour = 60*60;
        const int kTimingDay = 60*60*24;
        enum TimingType
        {
            kTimingTypeSecond = 0,
            kTimingTypeMinute = 1,
            kTimingTypeHour = 2,
            kTimingTypeDay = 3,
        };
        class CallbackEntry
        {
        public:
            CallbackEntry(const std::function<void()> &cb):cb_(cb) 
            {}
            ~CallbackEntry()
            {
                if(cb_) {
                    cb_();
                }
            }
        private:
            std::function<void()> cb_;
        };
        using CallbackEntryPtr = std::shared_ptr<CallbackEntry>;

        class TimingWheel
        {
        public:
            TimingWheel();
            ~TimingWheel();

            void InsertEntry(uint32_t delay, EntryPtr entryPtr);
            void OnTimer(int64_t now);
            void PopUp(Wheel &bq);
            void RunAfter(double delay, const std::function<void()> &cb);
            void RunAfter(double delay, std::function<void()> &&cb);
            void RunEvery(double interval, const std::function<void()> &cb);
            void RunEvery(double interval, std::function<void()> &&cb);
        
        private:
            void InsertSecondEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertHourEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertDayEntry(uint32_t delay, EntryPtr entryPtr);
            Wheels wheels_;
            int64_t lastTs_{0};
            uint64_t tick_{0};
        };
    }
}