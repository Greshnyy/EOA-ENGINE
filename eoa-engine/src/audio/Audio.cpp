#include "Audio/Audio.h"
#include "Core/Logger.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

// Простая реализация на основе miniaudio концепций
// В реальной проекте здесь была бы интеграция с FMOD, Wwise или miniaudio

namespace eoa {

// ============================================================================
// Color32 static constants
// ============================================================================

const Color32 Color32::White(255, 255, 255, 255);
const Color32 Color32::Black(0, 0, 0, 255);
const Color32 Color32::Red(255, 0, 0, 255);
const Color32 Color32::Green(0, 255, 0, 255);
const Color32 Color32::Blue(0, 0, 255, 255);
const Color32 Color32::Transparent(0, 0, 0, 0);

// ============================================================================
// AudioComponent
// ============================================================================

AudioComponent::AudioComponent(const std::string& name) 
    : Component(name) {}

AudioComponent::~AudioComponent() {
    Stop();
    UnloadSound();
}

bool AudioComponent::LoadSound(const std::string& filename) {
    soundFile_ = filename;
    
    // В реальной реализации здесь была бы загрузка аудиофайла
    // miniaudio, FMOD или другой библиотекой
    EOA_LOG_INFO("AudioComponent: Loading sound '{}'", filename);
    
    // Заглушка - считаем что звук загружен
    duration_ = 3.0f; // Дефолтная длительность
    return true;
}

void AudioComponent::UnloadSound() {
    if (soundHandle_) {
        // Выгрузка звука
        soundHandle_ = nullptr;
    }
    soundFile_.clear();
    duration_ = 0.0f;
}

void AudioComponent::Play() {
    if (soundFile_.empty()) {
        EOA_LOG_WARNING("AudioComponent: No sound loaded to play");
        return;
    }
    
    if (isPlaying_ && !isPaused_) {
        return; // Уже играет
    }
    
    if (isPaused_) {
        Resume();
    } else {
        // В реальной реализации: запуск воспроизведения через аудиосистему
        EOA_LOG_INFO("AudioComponent: Playing sound '{}'", soundFile_);
        isPlaying_ = true;
        isPaused_ = false;
        channelId_ = 0; // Заглушка ID канала
    }
}

void AudioComponent::Stop() {
    if (!isPlaying_) return;
    
    isPlaying_ = false;
    isPaused_ = false;
    channelId_ = -1;
    
    if (onSoundFinished_) {
        // Callback вызывается только если звук доиграл до конца
    }
}

void AudioComponent::Pause() {
    if (!isPlaying_ || isPaused_) return;
    isPaused_ = true;
}

void AudioComponent::Resume() {
    if (!isPaused_) return;
    isPaused_ = false;
}

float AudioComponent::GetTime() const {
    // В реальной реализации получение текущей позиции воспроизведения
    return 0.0f;
}

void AudioComponent::SetTime(float seconds) {
    // В реальной установке позиции воспроизведения
    EOA_LOG_INFO("AudioComponent: Seek to {}s", seconds);
}

float AudioComponent::GetSpatialVolume() const {
    if (!spatial_ || !GetActor()) {
        return volume_;
    }
    
    // Найти listener
    auto* listener = AudioManager::Get().GetActiveListener();
    if (!listener) {
        return volume_;
    }
    
    // Расчёт расстояния
    auto listenerPos = listener->GetActor()->GetTransform()->GetPosition();
    auto sourcePos = GetActor()->GetTransform()->GetPosition();
    float distance = glm::length(sourcePos - listenerPos);
    
    // Attenuation по расстоянию
    if (distance <= minDistance_) {
        return volume_;
    }
    
    if (distance >= maxDistance_) {
        return 0.0f;
    }
    
    // Linear attenuation
    float attenuation = 1.0f - (distance - minDistance_) / (maxDistance_ - minDistance_);
    return volume_ * attenuation;
}

float AudioComponent::GetSpatialPan() const {
    if (!spatial_ || !GetActor()) {
        return pan_;
    }
    
    auto* listener = AudioManager::Get().GetActiveListener();
    if (!listener) {
        return pan_;
    }
    
    // Расчёт пана на основе позиции относительно слушателя
    auto listenerPos = listener->GetActor()->GetTransform()->GetPosition();
    auto sourcePos = GetActor()->GetTransform()->GetPosition();
    
    // Получить forward и right векторы слушателя
    auto forward = listener->GetForward();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    
    glm::vec3 direction = sourcePos - listenerPos;
    float dotProduct = glm::dot(glm::normalize(direction), right);
    
    // Clamp от -1 до 1
    return glm::clamp(dotProduct, -1.0f, 1.0f);
}

void AudioComponent::Tick(float deltaTime) {
    if (!isPlaying_ || isPaused_) return;
    
    // Обновление 3D параметров
    if (spatial_) {
        // В реальной реализации обновление позиции источника в аудиодвижке
    }
    
    // Проверка завершения воспроизведения (заглушка)
    // В реальной реализации проверка состояния канала
}

// ============================================================================
// AudioListenerComponent
// ============================================================================

AudioListenerComponent::AudioListenerComponent(const std::string& name)
    : Component(name) {}

AudioListenerComponent::~AudioListenerComponent() = default;

void AudioListenerComponent::UpdateFromTransform() {
    if (!GetActor()) return;
    
    auto* transform = GetActor()->GetTransform();
    if (!transform) return;
    
    // Получить orientation из трансформации
    glm::mat4 matrix = transform->GetMatrix();
    forward_ = -glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
    up_ = glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]);
    
    // Нормализация
    forward_ = glm::normalize(forward_);
    up_ = glm::normalize(up_);
}

void AudioListenerComponent::Tick(float deltaTime) {
    if (!GetActor()) return;
    
    glm::vec3 currentPos = GetActor()->GetTransform()->GetPosition();
    
    // Расчёт velocity для Doppler effect
    velocity_ = (currentPos - lastPosition_) / deltaTime;
    lastPosition_ = currentPos;
    
    UpdateFromTransform();
}

// ============================================================================
// AudioManager
// ============================================================================

AudioManager::AudioManager() = default;

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize(const AudioConfig& config) {
    if (initialized_) {
        return true;
    }
    
    config_ = config;
    
    EOA_LOG_INFO("AudioManager: Initializing with sample rate {}", config.sampleRate);
    EOA_LOG_INFO("AudioManager: Max channels: {}", config.maxSoundChannels);
    EOA_LOG_INFO("AudioManager: 3D Audio: {}", config.enable3DAudio ? "enabled" : "disabled");
    
    // В реальной реализации инициализация аудиобиблиотеки
    // miniaudio_device_init, FMOD_System_Create, etc.
    
    initialized_ = true;
    return true;
}

void AudioManager::Shutdown() {
    if (!initialized_) return;
    
    // Остановка всех источников
    for (auto* source : sources_) {
        if (source) {
            source->Stop();
        }
    }
    
    // Выгрузка всех звуков
    for (auto& [name, handle] : loadedSounds_) {
        // Выгрузка звука
    }
    loadedSounds_.clear();
    sources_.clear();
    
    // В реальной реализации shutdown аудиобиблиотеки
    
    initialized_ = false;
    EOA_LOG_INFO("AudioManager: Shutdown complete");
}

void AudioManager::SetConfig(const AudioConfig& config) {
    config_ = config;
    
    // Применение настроек к аудиосистеме
    if (audioContext_) {
        // Обновление параметров контекста
    }
}

void AudioManager::SetMasterVolume(float volume) {
    config_.masterVolume = glm::clamp(volume, 0.0f, 1.0f);
    
    // Обновление master volume в аудиосистеме
}

void AudioManager::SetVolumeByType(SoundType type, float volume) {
    volume = glm::clamp(volume, 0.0f, 1.0f);
    
    switch (type) {
        case SoundType::SFX:
            config_.sfxVolume = volume;
            break;
        case SoundType::Music:
            config_.musicVolume = volume;
            break;
        case SoundType::Ambient:
            config_.ambientVolume = volume;
            break;
        case SoundType::Voice:
            config_.voiceVolume = volume;
            break;
        case SoundType::UI:
            // UI volume обычно не регулируется отдельно
            break;
    }
}

float AudioManager::GetVolumeByType(SoundType type) const {
    switch (type) {
        case SoundType::SFX: return config_.sfxVolume;
        case SoundType::Music: return config_.musicVolume;
        case SoundType::Ambient: return config_.ambientVolume;
        case SoundType::Voice: return config_.voiceVolume;
        default: return 1.0f;
    }
}

AudioComponent* AudioManager::CreateSource(const AudioSourceData& data) {
    auto* actor = new Actor(data.name);
    auto* audioComp = actor->AddComponent<AudioComponent>();
    
    if (!data.filename.empty()) {
        audioComp->LoadSound(data.filename);
    }
    
    audioComp->SetVolume(data.volume);
    audioComp->SetPitch(data.pitch);
    audioComp->SetPan(data.pan);
    audioComp->SetLoopMode(data.loopMode);
    audioComp->SetSpatial(data.spatial);
    audioComp->SetMinDistance(data.minDistance);
    audioComp->SetMaxDistance(data.maxDistance);
    
    sources_.push_back(audioComp);
    
    if (data.autoPlay) {
        audioComp->Play();
    }
    
    return audioComp;
}

void AudioManager::DestroySource(AudioComponent* source) {
    if (!source) return;
    
    source->Stop();
    
    auto it = std::find(sources_.begin(), sources_.end(), source);
    if (it != sources_.end()) {
        sources_.erase(it);
    }
    
    // Удаление актора
    delete source->GetActor();
}

AudioComponent* AudioManager::FindSourceByName(const std::string& name) const {
    for (auto* source : sources_) {
        if (source && source->GetActor()->GetName() == name) {
            return source;
        }
    }
    return nullptr;
}

void AudioManager::Update3DAudio() {
    if (!config_.enable3DAudio) return;
    if (!activeListener_) return;
    
    // Обновление позиций всех spatial источников
    for (auto* source : sources_) {
        if (source && source->IsSpatial() && source->IsPlaying()) {
            // В реальной реализации обновление 3D параметров источника
        }
    }
}

int AudioManager::GetActiveChannelCount() const {
    int count = 0;
    for (auto* source : sources_) {
        if (source && source->IsPlaying() && !source->IsPaused()) {
            count++;
        }
    }
    return count;
}

int AudioManager::GetLoadedSoundCount() const {
    return static_cast<int>(loadedSounds_.size());
}

bool AudioManager::LoadSoundBank(const std::string& bankName, 
                                  const std::vector<std::string>& files) {
    EOA_LOG_INFO("AudioManager: Loading sound bank '{}' with {} files", 
                 bankName, files.size());
    
    for (const auto& file : files) {
        // Загрузка каждого файла
        // В реальной реализации загрузка в память или стриминг
        loadedSounds_[file] = reinterpret_cast<void*>(1); // Заглушка
    }
    
    return true;
}

void AudioManager::UnloadSoundBank(const std::string& bankName) {
    EOA_LOG_INFO("AudioManager: Unloading sound bank '{}'", bankName);
    
    // Выгрузка звуков банка
    // В реальной реализации освобождение памяти
}

} // namespace eoa
