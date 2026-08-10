#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace eoa {

using Seconds = double;
using Milliseconds = double;

struct Time {
    Seconds deltaTime = 0.0;
    Seconds totalTime = 0.0;
    Seconds fixedDeltaTime = 0.02;
    float fps = 0.0f;
    uint64_t frameCount = 0;
    Seconds GetTimeScale() const { return timeScale_; }
    void SetTimeScale(Seconds scale) { timeScale_ = std::max(0.0, scale); }
    Seconds GetRealDeltaTime() const { return realDeltaTime_; }
    void SetRealDeltaTime(Seconds dt) { realDeltaTime_ = std::max(0.0, dt); }
private:
    Seconds timeScale_ = 1.0;
    Seconds realDeltaTime_ = 0.0;
};

class TimeSystem {
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
public:
    TimeSystem() : lastFrameTime_(Clock::now()) {}
    void SetFixedDeltaTime(Seconds dt) { time_.fixedDeltaTime = std::max(0.0001, dt); }
    void Tick() {
        const auto now = Clock::now();
        const Seconds realDelta = std::max(0.0, std::chrono::duration<Seconds>(now - lastFrameTime_).count());
        lastFrameTime_ = now;
        constexpr Seconds kMaxDeltaTime = 0.25;
        time_.SetRealDeltaTime(realDelta);
        time_.deltaTime = std::min(realDelta, kMaxDeltaTime) * time_.GetTimeScale();
        time_.totalTime += time_.deltaTime;
        ++time_.frameCount;
        frameTimes_.push_back(realDelta);
        if (frameTimes_.size() > 60) frameTimes_.erase(frameTimes_.begin());
        Seconds total = 0.0;
        for (Seconds sample : frameTimes_) total += sample;
        if (total > 0.0) time_.fps = static_cast<float>(frameTimes_.size() / total);
        fixedTimeAccumulator_ += time_.deltaTime;
    }
    bool ShouldFixedUpdate() const { return fixedTimeAccumulator_ >= time_.fixedDeltaTime; }
    bool ConsumeFixedStep() {
        if (!ShouldFixedUpdate()) return false;
        fixedTimeAccumulator_ -= time_.fixedDeltaTime;
        return true;
    }
    void DoFixedUpdate() { ConsumeFixedStep(); }
    const Time& GetTime() const { return time_; }
    static Seconds Now() { return std::chrono::duration<Seconds>(Clock::now().time_since_epoch()).count(); }
    static void Sleep(Milliseconds ms) { if (ms > 0.0) std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms)); }
private:
    TimePoint lastFrameTime_;
    Seconds fixedTimeAccumulator_ = 0.0;
    Time time_;
    std::vector<Seconds> frameTimes_;
};

enum class LogLevel { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4, Fatal = 5 };
using LogCallback = std::function<void(LogLevel, const std::string&)>;

class Logger {
public:
    static Logger& GetInstance() { static Logger instance; return instance; }
    void SetLevel(LogLevel level) { minLevel_ = level; }
    void SetCallback(LogCallback callback) { callback_ = std::move(callback); }
    void Log(LogLevel level, const std::string& message) {
        if (level < minLevel_) return;
        const char* prefix = "[INFO]";
        switch (level) {
            case LogLevel::Trace: prefix = "[TRACE]"; break;
            case LogLevel::Debug: prefix = "[DEBUG]"; break;
            case LogLevel::Info: prefix = "[INFO]"; break;
            case LogLevel::Warn: prefix = "[WARN]"; break;
            case LogLevel::Error: prefix = "[ERROR]"; break;
            case LogLevel::Fatal: prefix = "[FATAL]"; break;
        }
        const std::string full = std::string(prefix) + " " + message;
        if (callback_) callback_(level, full);
        else if (level >= LogLevel::Error) std::fprintf(stderr, "%s\n", full.c_str());
        else std::printf("%s\n", full.c_str());
        if (level == LogLevel::Fatal) std::abort();
    }
    void Trace(const std::string& m) { Log(LogLevel::Trace, m); }
    void Debug(const std::string& m) { Log(LogLevel::Debug, m); }
    void Info(const std::string& m) { Log(LogLevel::Info, m); }
    void Warn(const std::string& m) { Log(LogLevel::Warn, m); }
    void Error(const std::string& m) { Log(LogLevel::Error, m); }
    void Fatal(const std::string& m) { Log(LogLevel::Fatal, m); }
private:
    Logger() = default;
    LogLevel minLevel_ = LogLevel::Info;
    LogCallback callback_;
};

#define EOA_LOG_TRACE(msg) eoa::Logger::GetInstance().Trace(msg)
#define EOA_LOG_DEBUG(msg) eoa::Logger::GetInstance().Debug(msg)
#define EOA_LOG_INFO(msg) eoa::Logger::GetInstance().Info(msg)
#define EOA_LOG_WARN(msg) eoa::Logger::GetInstance().Warn(msg)
#define EOA_LOG_ERROR(msg) eoa::Logger::GetInstance().Error(msg)
#define EOA_LOG_FATAL(msg) eoa::Logger::GetInstance().Fatal(msg)

using Task = std::function<void()>;
class TaskManager {
public:
    static TaskManager& GetInstance() { static TaskManager instance; return instance; }
    void AddTask(Task task) { if (!task) return; std::lock_guard<std::mutex> lock(mutex_); tasks_.push_back(std::move(task)); }
    void AddTaskDelayed(Task task, Seconds delay) { if (!task) return; std::lock_guard<std::mutex> lock(mutex_); delayedTasks_.push_back({std::move(task), std::max(0.0, delay)}); }
    void ProcessTasks(Seconds deltaTime) {
        std::vector<Task> immediate, ready;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(immediate, tasks_);
            const Seconds dt = std::max(0.0, deltaTime);
            for (auto it = delayedTasks_.begin(); it != delayedTasks_.end();) {
                it->delay -= dt;
                if (it->delay <= 0.0) { ready.push_back(std::move(it->task)); it = delayedTasks_.erase(it); }
                else ++it;
            }
        }
        for (auto& task : immediate) if (task) task();
        for (auto& task : ready) if (task) task();
    }
    void ProcessTasks() { ProcessTasks(0.0); }
    void ClearTasks() { std::lock_guard<std::mutex> lock(mutex_); tasks_.clear(); delayedTasks_.clear(); }
private:
    struct DelayedTask { Task task; Seconds delay; };
    std::vector<Task> tasks_;
    std::vector<DelayedTask> delayedTasks_;
    std::mutex mutex_;
};

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Initialize() {}
    virtual void Shutdown() {}
    virtual void Update(Seconds) {}
    virtual void FixedUpdate(Seconds) {}
    virtual void Render() {}
    int GetPriority() const { return priority_; }
    void SetPriority(int priority) { priority_ = priority; }
    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
protected:
    int priority_ = 0;
    bool enabled_ = true;
};

} // namespace eoa
