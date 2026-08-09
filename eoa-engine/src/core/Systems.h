#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>

namespace eoa {

// ============================================================================
// ТИПЫ ВРЕМЕНИ
// ============================================================================

using Seconds = double;
using Milliseconds = double;

struct Time {
    Seconds deltaTime = 0.0;          // Время последнего кадра (сек)
    Seconds totalTime = 0.0;          // Общее время с запуска (сек)
    Seconds fixedDeltaTime = 0.02;    // Фиксированный шаг для физики (сек)
    float fps = 0.0f;                 // Кадров в секунду
    uint64_t frameCount = 0;          // Счётчик кадров
    
    Seconds GetTimeScale() const { return timeScale_; }
    void SetTimeScale(Seconds scale) { timeScale_ = std::max(0.0, scale); }
    
    Seconds GetRealDeltaTime() const { return realDeltaTime_; }
    void SetRealDeltaTime(Seconds dt) { realDeltaTime_ = dt; }
    
private:
    Seconds timeScale_ = 1.0;
public:
    Seconds realDeltaTime_ = 0.0;
};

// ============================================================================
// СИСТЕМА ВРЕМЕНИ
// ============================================================================

class TimeSystem {
public:
    TimeSystem() {
        lastFrameTime_ = Clock::now();
        fixedTimeAccumulator_ = 0.0;
    }
    
    void Tick() {
        auto now = Clock::now();
        std::chrono::duration<Seconds> elapsed = now - lastFrameTime_;
        lastFrameTime_ = now;
        
        time_.SetRealDeltaTime(elapsed.count());
        time_.deltaTime = elapsed.count() * time_.GetTimeScale();
        time_.totalTime += time_.deltaTime;
        time_.frameCount++;
        
        // Расчёт FPS
        frameTimes_.push_back(elapsed.count());
        if (frameTimes_.size() > 60) {
            frameTimes_.erase(frameTimes_.begin());
        }
        
        double avgTime = 0.0;
        for (auto t : frameTimes_) avgTime += t;
        time_.fps = static_cast<float>(frameTimes_.size() / avgTime);
        
        // Накопление времени для фиксированного шага
        fixedTimeAccumulator_ += time_.GetRealDeltaTime();
    }
    
    bool ShouldFixedUpdate() const {
        return fixedTimeAccumulator_ >= time_.fixedDeltaTime;
    }
    
    void DoFixedUpdate() {
        if (ShouldFixedUpdate()) {
            fixedTimeAccumulator_ -= time_.fixedDeltaTime;
        }
    }
    
    const Time& GetTime() const { return time_; }
    
    // Утилиты
    static Seconds Now() {
        auto now = Clock::now();
        return std::chrono::duration<Seconds>(now.time_since_epoch()).count();
    }
    
    static void Sleep(Milliseconds ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(ms)));
    }

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    TimePoint lastFrameTime_;
    Seconds fixedTimeAccumulator_;
    Time time_;
    std::vector<Seconds> frameTimes_;
};

// ============================================================================
// ЛОГГЕР
// ============================================================================

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

using LogCallback = std::function<void(LogLevel, const std::string&)>;

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }
    
    void SetLevel(LogLevel level) { minLevel_ = level; }
    void SetCallback(LogCallback callback) { callback_ = std::move(callback); }
    
    void Log(LogLevel level, const std::string& message) {
        if (level < minLevel_) return;
        
        std::string prefix;
        switch (level) {
            case LogLevel::Trace: prefix = "[TRACE]"; break;
            case LogLevel::Debug: prefix = "[DEBUG]"; break;
            case LogLevel::Info:  prefix = "[INFO] "; break;
            case LogLevel::Warn:  prefix = "[WARN] "; break;
            case LogLevel::Error: prefix = "[ERROR]"; break;
            case LogLevel::Fatal: prefix = "[FATAL]"; break;
        }
        
        std::string fullMessage = prefix + " " + message;
        
        if (callback_) {
            callback_(level, fullMessage);
        } else {
            // Вывод по умолчанию
            if (level >= LogLevel::Error) {
                fprintf(stderr, "%s\n", fullMessage.c_str());
            } else {
                printf("%s\n", fullMessage.c_str());
            }
        }
        
        if (level == LogLevel::Fatal) {
            std::abort();
        }
    }
    
    void Trace(const std::string& msg) { Log(LogLevel::Trace, msg); }
    void Debug(const std::string& msg) { Log(LogLevel::Debug, msg); }
    void Info(const std::string& msg) { Log(LogLevel::Info, msg); }
    void Warn(const std::string& msg) { Log(LogLevel::Warn, msg); }
    void Error(const std::string& msg) { Log(LogLevel::Error, msg); }
    void Fatal(const std::string& msg) { Log(LogLevel::Fatal, msg); }

private:
    Logger() : minLevel_(LogLevel::Info) {}
    LogLevel minLevel_;
    LogCallback callback_;
};

// Макросы для логирования
#define EOA_LOG_TRACE(msg)    eoa::Logger::GetInstance().Trace(msg)
#define EOA_LOG_DEBUG(msg)    eoa::Logger::GetInstance().Debug(msg)
#define EOA_LOG_INFO(msg)     eoa::Logger::GetInstance().Info(msg)
#define EOA_LOG_WARN(msg)     eoa::Logger::GetInstance().Warn(msg)
#define EOA_LOG_ERROR(msg)    eoa::Logger::GetInstance().Error(msg)
#define EOA_LOG_FATAL(msg)    eoa::Logger::GetInstance().Fatal(msg)

// ============================================================================
// МЕНЕДЖЕР ЗАДАЧ (Task System)
// ============================================================================

using Task = std::function<void()>;

class TaskManager {
public:
    static TaskManager& GetInstance() {
        static TaskManager instance;
        return instance;
    }
    
    void AddTask(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push_back(std::move(task));
    }
    
    void AddTaskDelayed(Task task, Seconds delay) {
        std::lock_guard<std::mutex> lock(mutex_);
        delayedTasks_.push_back({task, delay});
    }
    
    void ProcessTasks() {
        std::vector<Task> localTasks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(localTasks, tasks_);
        }
        
        for (auto& task : localTasks) {
            task();
        }
        
        // Обработка отложенных задач
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = delayedTasks_.begin(); it != delayedTasks_.end();) {
            it->delay -= TimeSystem{}.GetTime().deltaTime;
            if (it->delay <= 0.0) {
                it->task();
                it = delayedTasks_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void ClearTasks() {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.clear();
        delayedTasks_.clear();
    }

private:
    struct DelayedTask {
        Task task;
        Seconds delay;
    };
    
    std::vector<Task> tasks_;
    std::vector<DelayedTask> delayedTasks_;
    std::mutex mutex_;
};

// ============================================================================
// БАЗОВЫЙ КЛАСС СИСТЕМЫ
// ============================================================================

class ISystem {
public:
    virtual ~ISystem() = default;
    
    virtual void Initialize() {}
    virtual void Shutdown() {}
    virtual void Update(Seconds dt) {}
    virtual void FixedUpdate(Seconds dt) {}
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
