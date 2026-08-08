#include "Animation/Animation.h"
#include "Resources/Resource.h"
#include <algorithm>
#include <cmath>

namespace eoa {

// ============================================================================
// AnimationClip Implementation
// ============================================================================

glm::vec3 AnimationClip::InterpolatePosition(float animationTime, const BoneAnimation& anim) const {
    if (anim.positionKeys.empty()) return glm::vec3(0);
    if (anim.positionKeys.size() == 1) return anim.positionKeys[0].position;

    // Найти ключи между которыми находится текущее время
    for (size_t i = 0; i < anim.positionKeys.size() - 1; ++i) {
        if (animationTime >= anim.positionKeys[i].time && 
            animationTime < anim.positionKeys[i + 1].time) {
            
            auto& currentKey = anim.positionKeys[i];
            auto& nextKey = anim.positionKeys[i + 1];
            
            float range = nextKey.time - currentKey.time;
            float ratio = (animationTime - currentKey.time) / (range > 0 ? range : 1.0f);
            
            return glm::mix(currentKey.position, nextKey.position, ratio);
        }
    }
    
    return anim.positionKeys.back().position;
}

glm::quat AnimationClip::InterpolateRotation(float animationTime, const BoneAnimation& anim) const {
    if (anim.rotationKeys.empty()) return glm::quat(1, 0, 0, 0);
    if (anim.rotationKeys.size() == 1) return anim.rotationKeys[0].rotation;

    for (size_t i = 0; i < anim.rotationKeys.size() - 1; ++i) {
        if (animationTime >= anim.rotationKeys[i].time && 
            animationTime < anim.rotationKeys[i + 1].time) {
            
            auto& currentKey = anim.rotationKeys[i];
            auto& nextKey = anim.rotationKeys[i + 1];
            
            float range = nextKey.time - currentKey.time;
            float ratio = (animationTime - currentKey.time) / (range > 0 ? range : 1.0f);
            
            return glm::slerp(currentKey.rotation, nextKey.rotation, ratio);
        }
    }
    
    return anim.rotationKeys.back().rotation;
}

glm::vec3 AnimationClip::InterpolateScale(float animationTime, const BoneAnimation& anim) const {
    if (anim.scaleKeys.empty()) return glm::vec3(1);
    if (anim.scaleKeys.size() == 1) return anim.scaleKeys[0].scale;

    for (size_t i = 0; i < anim.scaleKeys.size() - 1; ++i) {
        if (animationTime >= anim.scaleKeys[i].time && 
            animationTime < anim.scaleKeys[i + 1].time) {
            
            auto& currentKey = anim.scaleKeys[i];
            auto& nextKey = anim.scaleKeys[i + 1];
            
            float range = nextKey.time - currentKey.time;
            float ratio = (animationTime - currentKey.time) / (range > 0 ? range : 1.0f);
            
            return glm::mix(currentKey.scale, nextKey.scale, ratio);
        }
    }
    
    return anim.scaleKeys.back().scale;
}

// ============================================================================
// Skeleton Implementation
// ============================================================================

void Skeleton::AddBone(const std::string& name, int parentIndex) {
    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    bone.localTransform = glm::mat4(1);
    bone.finalTransform = glm::mat4(1);
    
    boneMap_[name] = static_cast<int>(bones_.size());
    bones_.push_back(bone);
    finalTransforms_.resize(bones_.size());
}

int Skeleton::FindBoneIndex(const std::string& name) const {
    auto it = boneMap_.find(name);
    return (it != boneMap_.end()) ? it->second : -1;
}

void Skeleton::UpdateFinalTransforms() {
    finalTransforms_.resize(bones_.size());
    
    // Рекурсивное вычисление финальных трансформаций
    std::function<void(int, const glm::mat4&)> computeTransform = 
        [&](int boneIndex, const glm::mat4& parentTransform) {
            if (boneIndex < 0 || boneIndex >= static_cast<int>(bones_.size())) return;
            
            auto& bone = bones_[boneIndex];
            glm::mat4 globalTransform = parentTransform * bone.localTransform;
            finalTransforms_[boneIndex] = globalTransform;
            
            // Найти всех детей
            for (size_t i = 0; i < bones_.size(); ++i) {
                if (bones_[i].parentIndex == boneIndex) {
                    computeTransform(static_cast<int>(i), globalTransform);
                }
            }
        };
    
    // Начать с корневых костей (parentIndex == -1)
    for (size_t i = 0; i < bones_.size(); ++i) {
        if (bones_[i].parentIndex == -1) {
            computeTransform(static_cast<int>(i), glm::mat4(1));
        }
    }
}

// ============================================================================
// AnimatorComponent Implementation
// ============================================================================

AnimatorComponent::AnimatorComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(AnimatorComponent, Component)
}

AnimatorComponent::~AnimatorComponent() {
    Stop();
}

bool AnimatorComponent::LoadAnimationsFromModel(const std::string& modelPath) {
    // Загрузка анимаций из glTF модели
    // В полной реализации здесь будет парсинг glTF анимаций
    LOG_INFO("Loading animations from model: {}", modelPath);
    return true;
}

void AnimatorComponent::AddClip(const AnimationClip& clip) {
    clips_[clip.name] = clip;
}

const AnimationClip* AnimatorComponent::GetClip(const std::string& name) const {
    auto it = clips_.find(name);
    return (it != clips_.end()) ? &it->second : nullptr;
}

void AnimatorComponent::Play(const std::string& clipName, float blendTime) {
    if (clips_.find(clipName) == clips_.end()) {
        LOG_ERROR("Animation clip not found: {}", clipName);
        return;
    }
    
    previousClip_ = currentClip_;
    currentClip_ = clipName;
    currentTime_ = 0.0f;
    blendTime_ = blendTime;
    blendProgress_ = 0.0f;
    isPlaying_ = true;
    isPaused_ = false;
    needsUpdate_ = true;
    
    LOG_INFO("Playing animation: {}", clipName);
}

void AnimatorComponent::Stop() {
    isPlaying_ = false;
    isPaused_ = false;
    currentTime_ = 0.0f;
    normalizedTime_ = 0.0f;
    
    if (onAnimationFinished_) {
        onAnimationFinished_(currentClip_);
    }
}

void AnimatorComponent::Pause() {
    isPaused_ = true;
}

void AnimatorComponent::Resume() {
    isPaused_ = false;
}

void AnimatorComponent::CrossFade(const std::string& fromClip, const std::string& toClip, float duration) {
    if (currentClip_ != fromClip) {
        LOG_WARNING("Current clip is not {}, crossfade cancelled", fromClip);
        return;
    }
    
    Play(toClip, duration);
}

void AnimatorComponent::AddLayer(const std::string& layerName, const std::string& clipName, float weight) {
    AnimationLayer layer;
    layer.clipName = clipName;
    layer.weight = weight;
    layer.time = 0.0f;
    layers_[layerName] = layer;
}

void AnimatorComponent::SetLayerWeight(const std::string& layerName, float weight) {
    auto it = layers_.find(layerName);
    if (it != layers_.end()) {
        it->second.weight = weight;
    }
}

void AnimatorComponent::AddState(const AnimationState& state) {
    states_[state.name] = state;
}

void AnimatorComponent::SetCurrentState(const std::string& stateName) {
    auto it = states_.find(stateName);
    if (it != states_.end()) {
        currentState_ = stateName;
        stateTime_ = 0.0f;
        
        // Автоматически запустить клип состояния
        if (!it->second.clipName.empty()) {
            Play(it->second.clipName, it->second.blendInTime);
        }
    }
}

void AnimatorComponent::UpdateStateMachine(float deltaTime) {
    if (currentState_.empty()) return;
    
    auto it = states_.find(currentState_);
    if (it == states_.end()) return;
    
    auto& state = it->second;
    stateTime_ += deltaTime * state.speed;
    
    // Проверка переходов
    for (auto& transition : state.transitions) {
        if (transition.condition && transition.condition()) {
            SetCurrentState(transition.targetState);
            return;
        }
    }
}

void AnimatorComponent::SetSkeleton(std::shared_ptr<Skeleton> skeleton) {
    skeleton_ = skeleton;
}

const std::vector<glm::mat4>& AnimatorComponent::GetBoneTransforms() const {
    if (skeleton_) {
        return skeleton_->GetFinalTransforms();
    }
    static std::vector<glm::mat4> empty;
    return empty;
}

void AnimatorComponent::Tick(float deltaTime) {
    if (!isPlaying_ || isPaused_) return;
    
    UpdateAnimation(deltaTime);
    
    if (needsUpdate_ && skeleton_) {
        skeleton_->UpdateFinalTransforms();
    }
}

void AnimatorComponent::UpdateAnimation(float deltaTime) {
    auto* clip = GetClip(currentClip_);
    if (!clip || clip->duration <= 0) return;
    
    // Обновление времени
    currentTime_ += deltaTime * speed_;
    
    // Обработка зацикливания
    if (currentTime_ >= clip->duration) {
        const auto* stateIt = states_.find(currentState_);
        bool loop = (stateIt != states_.end()) ? stateIt->second.looping : true;
        
        if (loop) {
            currentTime_ = fmod(currentTime_, clip->duration);
        } else {
            currentTime_ = clip->duration;
            Stop();
            return;
        }
    }
    
    normalizedTime_ = currentTime_ / clip->duration;
    
    // Blend прогресс
    if (blendTime_ > 0) {
        blendProgress_ += deltaTime / blendTime_;
        if (blendProgress_ >= 1.0f) {
            blendProgress_ = 1.0f;
            blendTime_ = 0;
            previousClip_.clear();
        }
    }
    
    needsUpdate_ = true;
}

void AnimatorComponent::CalculateBoneTransform(int boneIndex, const glm::mat4& parentTransform) {
    if (!skeleton_ || boneIndex < 0 || 
        boneIndex >= static_cast<int>(skeleton_->GetBones().size())) {
        return;
    }
    
    // Вычисление трансформации кости на основе текущей анимации
    auto* clip = GetClip(currentClip_);
    if (!clip) return;
    
    const auto& bones = skeleton_->GetBones();
    const auto& bone = bones[boneIndex];
    
    // Найти анимацию для этой кости
    auto boneAnimIt = std::find_if(clip->boneAnimations.begin(), 
                                    clip->boneAnimations.end(),
                                    [&bone](const BoneAnimation& ba) {
                                        return ba.boneName == bone.name;
                                    });
    
    glm::mat4 localTransform = glm::mat4(1);
    
    if (boneAnimIt != clip->boneAnimations.end()) {
        // Interpolate transforms
        glm::vec3 pos = clip->InterpolatePosition(currentTime_, *boneAnimIt);
        glm::quat rot = clip->InterpolateRotation(currentTime_, *boneAnimIt);
        glm::vec3 scale = clip->InterpolateScale(currentTime_, *boneAnimIt);
        
        localTransform = glm::translate(glm::mat4(1), pos) *
                        glm::mat4_cast(rot) *
                        glm::scale(glm::mat4(1), scale);
    } else {
        localTransform = bone.localTransform;
    }
    
    // Apply blend with previous clip
    if (!previousClip_.empty() && blendProgress_ < 1.0f) {
        auto* prevClip = GetClip(previousClip_);
        if (prevClip) {
            auto prevBoneAnimIt = std::find_if(prevClip->boneAnimations.begin(),
                                                prevClip->boneAnimations.end(),
                                                [&bone](const BoneAnimation& ba) {
                                                    return ba.boneName == bone.name;
                                                });
            
            if (prevBoneAnimIt != prevClip->boneAnimations.end()) {
                glm::vec3 prevPos = prevClip->InterpolatePosition(currentTime_, *prevBoneAnimIt);
                glm::quat prevRot = prevClip->InterpolateRotation(currentTime_, *prevBoneAnimIt);
                glm::vec3 prevScale = prevClip->InterpolateScale(currentTime_, *prevBoneAnimIt);
                
                glm::mat4 prevLocalTransform = glm::translate(glm::mat4(1), prevPos) *
                                               glm::mat4_cast(prevRot) *
                                               glm::scale(glm::mat4(1), prevScale);
                
                // Linear interpolation between transforms
                float t = blendProgress_;
                localTransform = glm::mix(prevLocalTransform, localTransform, t);
            }
        }
    }
    
    // Update layers (additive blending)
    for (auto& [layerName, layer] : layers_) {
        auto* layerClip = GetClip(layer.clipName);
        if (layerClip && layer.weight > 0) {
            auto layerBoneAnimIt = std::find_if(layerClip->boneAnimations.begin(),
                                                 layerClip->boneAnimations.end(),
                                                 [&bone](const BoneAnimation& ba) {
                                                     return ba.boneName == bone.name;
                                                 });
            
            if (layerBoneAnimIt != layerClip->boneAnimations.end()) {
                // Additive transform application
                glm::vec3 addPos = layerClip->InterpolatePosition(layer.time, *layerBoneAnimIt);
                glm::quat addRot = layerClip->InterpolateRotation(layer.time, *layerBoneAnimIt);
                
                localTransform = glm::translate(localTransform, addPos * layer.weight) *
                                glm::mat4_cast(glm::slerp(glm::quat(1,0,0,0), addRot, layer.weight));
            }
        }
        layer.time += deltaTime_ * speed_;
    }
    
    // Set final local transform
    const_cast<Bone&>(bone).localTransform = localTransform;
    
    // Process children
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex == boneIndex) {
            CalculateBoneTransform(static_cast<int>(i), localTransform * parentTransform);
        }
    }
}

void AnimatorComponent::InterpolatePose(float time, const AnimationClip& clip, 
                                         std::vector<glm::mat4>& outPoses) {
    if (!skeleton_) return;
    
    const auto& bones = skeleton_->GetBones();
    outPoses.resize(bones.size());
    
    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];
        
        auto boneAnimIt = std::find_if(clip.boneAnimations.begin(),
                                        clip.boneAnimations.end(),
                                        [&bone](const BoneAnimation& ba) {
                                            return ba.boneName == bone.name;
                                        });
        
        glm::mat4 pose = glm::mat4(1);
        
        if (boneAnimIt != clip.boneAnimations.end()) {
            glm::vec3 pos = clip.InterpolatePosition(time, *boneAnimIt);
            glm::quat rot = clip.InterpolateRotation(time, *boneAnimIt);
            glm::vec3 scale = clip.InterpolateScale(time, *boneAnimIt);
            
            pose = glm::translate(glm::mat4(1), pos) *
                   glm::mat4_cast(rot) *
                   glm::scale(glm::mat4(1), scale);
        }
        
        outPoses[i] = pose;
    }
}

// ============================================================================
// BlendTreeNode Implementations
// ============================================================================

BlendTreeClipNode::BlendTreeClipNode(const std::string& name, const std::string& clipName, float weight)
    : name_(name), clipName_(clipName), weight_(weight) {}

void BlendTreeClipNode::Update(float deltaTime) {
    time_ += deltaTime;
}

BlendTree1DNode::BlendTree1DNode(const std::string& name, const std::string& parameterName)
    : name_(name), parameterName_(parameterName) {}

void BlendTree1DNode::AddClip(const std::string& clipName, float threshold) {
    clips_.emplace_back(clipName, threshold);
    // Сортировка по threshold
    std::sort(clips_.begin(), clips_.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
}

void BlendTree1DNode::Update(float deltaTime) {
    // Нахождение двух ближайших клипов для блендинга
    for (size_t i = 0; i < clips_.size() - 1; ++i) {
        if (parameterValue_ >= clips_[i].second && parameterValue_ <= clips_[i + 1].second) {
            // Блендинг между этими двумя клипами
            break;
        }
    }
}

BlendTree2DNode::BlendTree2DNode(const std::string& name, const std::string& paramX, const std::string& paramY)
    : name_(name), paramX_(paramX), paramY_(paramY) {}

void BlendTree2DNode::AddClip(const std::string& clipName, float x, float y) {
    clips_.push_back({clipName, x, y});
}

void BlendTree2DNode::Update(float deltaTime) {
    // Barycentric interpolation для 2D blend
}

BlendTree::BlendTree(const std::string& name) : name_(name) {}

BlendTree::~BlendTree() = default;

void BlendTree::Update(float deltaTime) {
    activeClips_.clear();
    if (root_) {
        root_->Update(deltaTime);
    }
}

void BlendTree::SetParameter(const std::string& name, float value) {
    parameters_[name] = value;
}

float BlendTree::GetParameter(const std::string& name, float defaultValue) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second : defaultValue;
}

void BlendTree::EvaluateNode(BlendTreeNode* node, AnimatorComponent* animator) {
    // Рекурсивная оценка дерева
}

// ============================================================================
// AnimationComponent Implementation
// ============================================================================

AnimationComponent::AnimationComponent(const std::string& name)
    : Component(name), blendTree_("DefaultBlendTree") {
    EOA_CLASS_CONSTRUCT(AnimationComponent, Component)
}

AnimationComponent::~AnimationComponent() = default;

void AnimationComponent::PlayAnimation(const std::string& clipName, bool loop) {
    if (animator_) {
        animator_->Play(clipName);
    }
}

void AnimationComponent::StopAnimation() {
    if (animator_) {
        animator_->Stop();
    }
}

void AnimationComponent::SetFloatParameter(const std::string& name, float value) {
    blendTree_.SetParameter(name, value);
}

void AnimationComponent::SetTrigger(const std::string& name) {
    // Установка триггера для state machine
}

void AnimationComponent::ResetTrigger(const std::string& name) {
    // Сброс триггера
}

void AnimationComponent::Tick(float deltaTime) {
    blendTree_.Update(deltaTime);
}

} // namespace eoa
