#pragma once
#include "core/component.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace eoa {

// ============================================================================
// AUDIO SYSTEM - 3D звук и музыка
// ============================================================================

enum class SoundType : uint8_t {
    SFX,          // Звуковые эффекты
    Music,        // Музыка
    Ambient,      // Фоновые звуки
    Voice,        // Голос/диалоги
    UI            // UI звуки
};

enum class SoundLoopMode : uint8_t {
    None,
    Loop,
    PingPong
};

struct AudioConfig {
    int sampleRate = 44100;
    int bufferCount = 4;
    float masterVolume = 1.0f;
    float sfxVolume = 1.0f;
    float musicVolume = 0.7f;
    float ambientVolume = 0.5f;
    float voiceVolume = 1.0f;
    bool enable3DAudio = true;
    float dopplerFactor = 1.0f;
    int maxSoundChannels = 64;
};

// Структура звукового источника
struct AudioSourceData {
    std::string name;
    std::string filename;
    SoundType type = SoundType::SFX;
    SoundLoopMode loopMode = SoundLoopMode::None;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;           // -1 (left) to 1 (right)
    float minDistance = 1.0f;   // Для 3D звука
    float maxDistance = 100.0f; // Для 3D звука
    bool spatial = false;       // 3D или 2D звук
    bool autoPlay = false;
};

// ============================================================================
// AudioComponent - компонент воспроизведения звука
// ============================================================================

class AudioComponent : public Component {
public:
    EOA_CLASS_DECL(AudioComponent, Component)

    explicit AudioComponent(const std::string& name = "Audio");
    ~AudioComponent() override;

    // Загрузка звука
    bool LoadSound(const std::string& filename);
    void UnloadSound();

    // Воспроизведение
    void Play();
    void Stop();
    void Pause();
    void Resume();
    
    bool IsPlaying() const { return isPlaying_; }
    bool IsPaused() const { return isPaused_; }

    // Позиция в звуке (в секундах)
    float GetTime() const;
    void SetTime(float seconds);
    float GetDuration() const { return duration_; }

    // Настройки воспроизведения
    float GetVolume() const { return volume_; }
    void SetVolume(float vol) { volume_ = vol; }

    float GetPitch() const { return pitch_; }
    void SetPitch(float pitch) { pitch_ = pitch; }

    float GetPan() const { return pan_; }
    void SetPan(float pan) { pan_ = pan; }

    // Loop
    bool IsLooping() const { return loopMode_ != SoundLoopMode::None; }
    void SetLooping(bool loop) { loopMode_ = loop ? SoundLoopMode::Loop : SoundLoopMode::None; }
    SoundLoopMode GetLoopMode() const { return loopMode_; }
    void SetLoopMode(SoundLoopMode mode) { loopMode_ = mode; }

    // 3D аудио настройки
    bool IsSpatial() const { return spatial_; }
    void SetSpatial(bool spatial) { spatial_ = spatial; }

    float GetMinDistance() const { return minDistance_; }
    void SetMinDistance(float dist) { minDistance_ = dist; }

    float GetMaxDistance() const { return maxDistance_; }
    void SetMaxDistance(float dist) { maxDistance_ = dist; }

    // Получение 3D параметров (рассчитывается автоматически)
    float GetSpatialVolume() const;  // Volume с учётом расстояния
    float GetSpatialPan() const;     // Pan с учётом позиции слушателя

    // Callbacks
    using OnSoundFinishedCallback = std::function<void()>;
    void SetOnSoundFinished(OnSoundFinishedCallback callback) {
        onSoundFinished_ = callback;
    }

    // Tick для обновления 3D звука
    void Tick(float deltaTime) override;

private:
    std::string soundFile_;
    float volume_ = 1.0f;
    float pitch_ = 1.0f;
    float pan_ = 0.0f;
    float duration_ = 0.0f;
    SoundLoopMode loopMode_ = SoundLoopMode::None;
    bool spatial_ = false;
    float minDistance_ = 1.0f;
    float maxDistance_ = 100.0f;
    
    bool isPlaying_ = false;
    bool isPaused_ = false;
    
    OnSoundFinishedCallback onSoundFinished_;
    
    // Внутренний handle на звук
    void* soundHandle_ = nullptr;
    int channelId_ = -1;
};

// ============================================================================
// AudioListenerComponent - слушатель (обычно камера/игрок)
// ============================================================================

class AudioListenerComponent : public Component {
public:
    EOA_CLASS_DECL(AudioListenerComponent, Component)

    explicit AudioListenerComponent(const std::string& name = "AudioListener");
    ~AudioListenerComponent() override;

    // Orientation
    glm::vec3 GetForward() const { return forward_; }
    glm::vec3 GetUp() const { return up_; }

    // Velocity (для Doppler effect)
    const glm::vec3& GetVelocity() const { return velocity_; }
    void SetVelocity(const glm::vec3& vel) { velocity_ = vel; }

    // Update из Transform
    void UpdateFromTransform();

    // Tick для расчёта velocity
    void Tick(float deltaTime) override;

private:
    glm::vec3 forward_ = glm::vec3(0, 0, -1);
    glm::vec3 up_ = glm::vec3(0, 1, 0);
    glm::vec3 velocity_ = glm::vec3(0);
    glm::vec3 lastPosition_ = glm::vec3(0);
};

// ============================================================================
// AudioManager - глобальный менеджер аудио
// ============================================================================

class AudioManager {
public:
    static AudioManager& Get() {
        static AudioManager instance;
        return instance;
    }

    // Инициализация/завершение
    bool Initialize(const AudioConfig& config = AudioConfig());
    void Shutdown();

    // Конфигурация
    const AudioConfig& GetConfig() const { return config_; }
    void SetConfig(const AudioConfig& config);

    // Master volume
    float GetMasterVolume() const { return config_.masterVolume; }
    void SetMasterVolume(float volume);

    // Volume по типам
    void SetVolumeByType(SoundType type, float volume);
    float GetVolumeByType(SoundType type) const;

    // Создание/удаление источников
    AudioComponent* CreateSource(const AudioSourceData& data);
    void DestroySource(AudioComponent* source);

    // Найти источник по имени
    AudioComponent* FindSourceByName(const std::string& name) const;

    // Получить все источники
    const std::vector<AudioComponent*>& GetAllSources() const { return sources_; }

    // Активный listener
    AudioListenerComponent* GetActiveListener() const { return activeListener_; }
    void SetActiveListener(AudioListenerComponent* listener) { activeListener_ = listener; }

    // Обновление всех 3D звуков
    void Update3DAudio();

    // Статистика
    int GetActiveChannelCount() const;
    int GetLoadedSoundCount() const;

    // Загрузка/выгрузка банка звуков
    bool LoadSoundBank(const std::string& bankName, const std::vector<std::string>& files);
    void UnloadSoundBank(const std::string& bankName);

private:
    AudioManager() = default;
    ~AudioManager();

    AudioConfig config_;
    std::vector<AudioComponent*> sources_;
    std::unordered_map<std::string, void*> loadedSounds_; // name -> handle
    AudioListenerComponent* activeListener_ = nullptr;
    
    bool initialized_ = false;
    void* audioContext_ = nullptr;
    void* audioDevice_ = nullptr;
};

} // namespace eoa
