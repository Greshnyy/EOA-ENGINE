#pragma once
#include "core/component.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace eoa {

// ============================================================================
// ANIMATION SYSTEM - Скелетная анимация и Blend Trees
// ============================================================================

// Ключ анимации для позиции
struct PositionKeyframe {
    float time = 0.0f;
    glm::vec3 position = glm::vec3(0);
};

// Ключ анимации для вращения
struct RotationKeyframe {
    float time = 0.0f;
    glm::quat rotation = glm::quat(1, 0, 0, 0);
};

// Ключ анимации для масштаба
struct ScaleKeyframe {
    float time = 0.0f;
    glm::vec3 scale = glm::vec3(1);
};

// Анимация отдельной кости
struct BoneAnimation {
    std::string boneName;
    int boneIndex = -1;
    std::vector<PositionKeyframe> positionKeys;
    std::vector<RotationKeyframe> rotationKeys;
    std::vector<ScaleKeyframe> scaleKeys;
};

// Клип анимации
struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 25.0f;
    std::vector<BoneAnimation> boneAnimations;
    
    // Interpolation
    glm::vec3 InterpolatePosition(float animationTime, const BoneAnimation& anim) const;
    glm::quat InterpolateRotation(float animationTime, const BoneAnimation& anim) const;
    glm::vec3 InterpolateScale(float animationTime, const BoneAnimation& anim) const;
};

// Состояние анимации в State Machine
enum class AnimationStateType {
    Idle,
    Walk,
    Run,
    Jump,
    Attack,
    Death,
    Custom
};

struct AnimationState {
    std::string name;
    AnimationStateType type = AnimationStateType::Idle;
    std::string clipName;
    float speed = 1.0f;
    bool looping = true;
    float blendInTime = 0.2f;
    float blendOutTime = 0.2f;
    
    // Transitions
    struct Transition {
        std::string targetState;
        std::function<bool()> condition;
        float transitionDuration = 0.2f;
    };
    std::vector<Transition> transitions;
};

// ============================================================================
// Skeleton - скелет модели
// ============================================================================

struct Bone {
    std::string name;
    int parentIndex = -1;
    glm::mat4 inverseBindMatrix = glm::mat4(1);
    glm::mat4 localTransform = glm::mat4(1);
    glm::mat4 finalTransform = glm::mat4(1);
};

class Skeleton {
public:
    Skeleton() = default;
    ~Skeleton() = default;

    void AddBone(const std::string& name, int parentIndex);
    int FindBoneIndex(const std::string& name) const;
    
    const std::vector<Bone>& GetBones() const { return bones_; }
    size_t GetBoneCount() const { return bones_.size(); }
    
    void UpdateFinalTransforms();
    const std::vector<glm::mat4>& GetFinalTransforms() const { return finalTransforms_; }

private:
    std::vector<Bone> bones_;
    std::vector<glm::mat4> finalTransforms_;
    std::unordered_map<std::string, int> boneMap_;
};

// ============================================================================
// AnimatorComponent - компонент аниматора
// ============================================================================

class AnimatorComponent : public Component {
public:
    EOA_CLASS_DECL(AnimatorComponent, Component)

    explicit AnimatorComponent(const std::string& name = "Animator");
    ~AnimatorComponent() override;

    // Загрузка анимаций из glTF
    bool LoadAnimationsFromModel(const std::string& modelPath);
    
    // Добавление клипа
    void AddClip(const AnimationClip& clip);
    const AnimationClip* GetClip(const std::string& name) const;
    
    // Воспроизведение
    void Play(const std::string& clipName, float blendTime = 0.2f);
    void Stop();
    void Pause();
    void Resume();
    
    bool IsPlaying() const { return isPlaying_; }
    bool IsPaused() const { return isPaused_; }
    
    // Текущий клип
    const std::string& GetCurrentClip() const { return currentClip_; }
    float GetNormalizedTime() const { return normalizedTime_; }
    
    // Speed
    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    
    // Blending между двумя клипами
    void CrossFade(const std::string& fromClip, const std::string& toClip, float duration);
    
    // Additive blending
    void AddLayer(const std::string& layerName, const std::string& clipName, float weight = 1.0f);
    void SetLayerWeight(const std::string& layerName, float weight);
    
    // State Machine
    void AddState(const AnimationState& state);
    void SetCurrentState(const std::string& stateName);
    const std::string& GetCurrentState() const { return currentState_; }
    void UpdateStateMachine(float deltaTime);
    
    // Skeleton
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    std::shared_ptr<Skeleton> GetSkeleton() const { return skeleton_; }
    
    // Получение финальных трансформаций костей для скиннинга
    const std::vector<glm::mat4>& GetBoneTransforms() const;
    
    // Tick
    void Tick(float deltaTime) override;
    
    // Callback при завершении анимации
    using OnAnimationFinishedCallback = std::function<void(const std::string&)>;
    void SetOnAnimationFinished(OnAnimationFinishedCallback callback) {
        onAnimationFinished_ = callback;
    }

private:
    std::unordered_map<std::string, AnimationClip> clips_;
    std::shared_ptr<Skeleton> skeleton_;
    
    std::string currentClip_;
    std::string previousClip_;
    float currentTime_ = 0.0f;
    float normalizedTime_ = 0.0f;
    float blendTime_ = 0.0f;
    float blendProgress_ = 0.0f;
    float speed_ = 1.0f;
    
    bool isPlaying_ = false;
    bool isPaused_ = false;
    bool needsUpdate_ = true;
    
    // Layers для additive blending
    struct AnimationLayer {
        std::string clipName;
        float weight = 1.0f;
        float time = 0.0f;
    };
    std::unordered_map<std::string, AnimationLayer> layers_;
    
    // State machine
    std::unordered_map<std::string, AnimationState> states_;
    std::string currentState_;
    float stateTime_ = 0.0f;
    
    OnAnimationFinishedCallback onAnimationFinished_;
    
    void UpdateAnimation(float deltaTime);
    void CalculateBoneTransform(int boneIndex, const glm::mat4& parentTransform);
    void InterpolatePose(float time, const AnimationClip& clip, std::vector<glm::mat4>& outPoses);
};

// ============================================================================
// BlendTree - дерево блендинга анимаций
// ============================================================================

class BlendTreeNode {
public:
    virtual ~BlendTreeNode() = default;
    virtual void Update(float deltaTime) = 0;
    virtual float GetWeight() const = 0;
    virtual std::string GetName() const = 0;
};

// Лист дерева - проигрывание одного клипа
class BlendTreeClipNode : public BlendTreeNode {
public:
    explicit BlendTreeClipNode(const std::string& name, const std::string& clipName, float weight = 1.0f);
    
    void SetClipName(const std::string& name) { clipName_ = name; }
    const std::string& GetClipName() const { return clipName_; }
    
    void Update(float deltaTime) override;
    float GetWeight() const override { return weight_; }
    void SetWeight(float weight) { weight_ = weight; }
    std::string GetName() const override { return name_; }
    
    float GetTime() const { return time_; }
    void SetTime(float time) { time_ = time; }

private:
    std::string name_;
    std::string clipName_;
    float weight_ = 1.0f;
    float time_ = 0.0f;
};

// 1D Blend Node (blend по одному параметру)
class BlendTree1DNode : public BlendTreeNode {
public:
    explicit BlendTree1DNode(const std::string& name, const std::string& parameterName);
    
    void AddClip(const std::string& clipName, float threshold);
    
    void SetParameter(float value) { parameterValue_ = value; }
    float GetParameter() const { return parameterValue_; }
    
    void Update(float deltaTime) override;
    float GetWeight() const override { return 1.0f; }
    std::string GetName() const override { return name_; }
    
    const std::vector<std::pair<std::string, float>>& GetClips() const { return clips_; }

private:
    std::string name_;
    std::string parameterName_;
    float parameterValue_ = 0.0f;
    std::vector<std::pair<std::string, float>> clips_; // clipName, threshold
};

// 2D Blend Node (blend по двум параметрам)
class BlendTree2DNode : public BlendTreeNode {
public:
    explicit BlendTree2DNode(const std::string& name, 
                             const std::string& paramX, 
                             const std::string& paramY);
    
    void AddClip(const std::string& clipName, float x, float y);
    
    void SetParameters(float x, float y) { paramX_ = x; paramY_ = y; }
    
    void Update(float deltaTime) override;
    float GetWeight() const override { return 1.0f; }
    std::string GetName() const override { return name_; }

private:
    struct ClipPoint {
        std::string clipName;
        float x, y;
    };
    std::string name_;
    std::string paramX_, paramY_;
    float paramX_ = 0.0f, paramY_ = 0.0f;
    std::vector<ClipPoint> clips_;
};

// Blend Tree Manager
class BlendTree {
public:
    explicit BlendTree(const std::string& name = "BlendTree");
    ~BlendTree();
    
    void SetRoot(std::unique_ptr<BlendTreeNode> root) { root_ = std::move(root); }
    
    void Update(float deltaTime);
    
    // Параметры для blend nodes
    void SetParameter(const std::string& name, float value);
    float GetParameter(const std::string& name, float defaultValue = 0.0f) const;
    
    // Получение активных клипов с весами
    const std::unordered_map<std::string, float>& GetActiveClips() const { return activeClips_; }

private:
    std::string name_;
    std::unique_ptr<BlendTreeNode> root_;
    std::unordered_map<std::string, float> parameters_;
    std::unordered_map<std::string, float> activeClips_;
    
    void EvaluateNode(BlendTreeNode* node, AnimatorComponent* animator);
};

// ============================================================================
// AnimationComponent - упрощённый компонент для простой анимации
// ============================================================================

class AnimationComponent : public Component {
public:
    EOA_CLASS_DECL(AnimationComponent, Component)

    explicit AnimationComponent(const std::string& name = "Animation");
    ~AnimationComponent() override;

    // Простое воспроизведение
    void PlayAnimation(const std::string& clipName, bool loop = true);
    void StopAnimation();
    
    // Parameters для blend trees
    void SetFloatParameter(const std::string& name, float value);
    void SetTrigger(const std::string& name);
    void ResetTrigger(const std::string& name);
    
    // Получить Animator
    AnimatorComponent* GetAnimator() { return animator_; }
    
    void Tick(float deltaTime) override;

private:
    AnimatorComponent* animator_ = nullptr;
    BlendTree blendTree_;
};

} // namespace eoa
