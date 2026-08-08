#pragma once
#include "core/component.h"
#include "Physics/Physics.h"
#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <unordered_map>

namespace eoa {

// ============================================================================
// AI NAVIGATION & BEHAVIOR SYSTEM
// ============================================================================

// Типы узлов навигации
enum class NavNodeType : uint8_t {
    Walkable,
    NotWalkable,
    Water,
    Ladder,
    JumpPoint,
    Teleport
};

// Структура узла навигационной сетки
struct NavNode {
    int id = -1;
    glm::vec3 position;
    glm::vec3 normal;
    float size = 1.0f;
    NavNodeType type = NavNodeType::Walkable;
    float cost = 1.0f;
    int areaId = 0;
    bool enabled = true;
    
    // Соседние узлы
    std::vector<int> neighbors;
    
    // Для A* поиска
    float gCost = 0.0f;  // Стоимость от старта
    float hCost = 0.0f;  // Эвристическая стоимость до цели
    float fCost() const { return gCost + hCost; }
    int parent = -1;
};

// Полигон навигации (для NavMesh)
struct NavPolygon {
    int id = -1;
    std::vector<glm::vec3> vertices;
    glm::vec3 normal;
    int areaId = 0;
    std::vector<int> neighborPolygons;
    bool enabled = true;
};

// Запрос пути
struct PathQuery {
    glm::vec3 startPos;
    glm::vec3 endPos;
    float agentRadius = 0.5f;
    float agentHeight = 2.0f;
    float maxClimb = 0.5f;
    float maxSlope = 45.0f;
    uint32_t navigationMask = 0xFFFFFFFF;
};

// Результат поиска пути
struct PathResult {
    bool found = false;
    std::vector<glm::vec3> path;
    float totalDistance = 0.0f;
    int processedNodes = 0;
    float searchTime = 0.0f;
};

// ============================================================================
// NavMeshComponent - компонент навигационной сетки
// ============================================================================

class NavMeshComponent : public Component {
public:
    EOA_CLASS_DECL(NavMeshComponent, Component)

    explicit NavMeshComponent(const std::string& name = "NavMesh");
    ~NavMeshComponent() override;

    // Генерация навмеша из геометрии
    void BuildFromGeometry(const std::vector<glm::vec3>& vertices,
                           const std::vector<uint32_t>& indices,
                           float cellSize = 0.3f,
                           float cellHeight = 0.2f);

    // Очистка навмеша
    void Clear();

    // Поиск пути
    PathResult FindPath(const glm::vec3& start, const glm::vec3& end,
                        float agentRadius = 0.5f,
                        float agentHeight = 2.0f);

    PathResult FindPathSmooth(const glm::vec3& start, const glm::vec3& end,
                              float agentRadius = 0.5f,
                              float agentHeight = 2.0f);

    // Проекция точки на навмеш
    glm::vec3 ProjectPoint(const glm::vec3& point) const;
    glm::vec3 GetRandomPoint() const;
    glm::vec3 GetRandomPointAround(const glm::vec3& center, float radius) const;

    // Query
    bool IsValidPosition(const glm::vec3& position, float radius = 0.5f) const;
    float GetDistanceToNearestWalkable(const glm::vec3& position) const;

    // Узлы и полигоны
    const std::vector<NavNode>& GetNodes() const { return nodes_; }
    const std::vector<NavPolygon>& GetPolygons() const { return polygons_; }

    // Настройки
    float GetCellSize() const { return cellSize_; }
    float GetAgentRadius() const { return agentRadius_; }
    float GetAgentHeight() const { return agentHeight_; }
    float GetMaxSlope() const { return maxSlope_; }
    float GetMaxClimb() const { return maxClimb_; }

    void SetAgentRadius(float radius) { agentRadius_ = radius; }
    void SetAgentHeight(float height) { agentHeight_ = height; }
    void SetMaxSlope(float degrees) { maxSlope_ = degrees; }
    void SetMaxClimb(float climb) { maxClimb_ = climb; }

    // Визуализация (для дебага)
    bool IsDebugVisualizationEnabled() const { return debugVisualization_; }
    void SetDebugVisualizationEnabled(bool enabled) { debugVisualization_ = enabled; }

    // Сохранение/загрузка
    bool SaveToFile(const std::string& filename) const;
    bool LoadFromFile(const std::string& filename);

    // Tick для динамического обновления
    void Tick(float deltaTime) override;

private:
    std::vector<NavNode> nodes_;
    std::vector<NavPolygon> polygons_;
    
    float cellSize_ = 0.3f;
    float cellHeight_ = 0.2f;
    float agentRadius_ = 0.5f;
    float agentHeight_ = 2.0f;
    float maxSlope_ = 45.0f;
    float maxClimb_ = 0.5f;
    
    bool debugVisualization_ = false;
    bool needsRebuild_ = false;
    
    // A* поиск
    PathResult AStarSearch(int startNode, int endNode);
    void SmoothPath(std::vector<glm::vec3>& path);
    
    // Внутренние структуры для генерации
    void GenerateNavMesh();
    int FindNearestNode(const glm::vec3& position) const;
};

// ============================================================================
// AI Perception System
// ============================================================================

enum class SenseType : uint8_t {
    Sight,
    Hearing,
    Touch,
    Smell
};

struct Stimulus {
    SenseType type = SenseType::Sight;
    glm::vec3 position;
    float strength = 1.0f;
    Actor* source = nullptr;
    double timestamp = 0.0;
    std::string tag;
};

struct PerceptionConfig {
    float sightRange = 50.0f;
    float hearingRange = 30.0f;
    float peripheralVisionAngle = 60.0f;  // Угол периферийного зрения
    float fieldOfView = 90.0f;            // Полное поле зрения
    float detectionSpeed = 1.0f;          // Скорость обнаружения
    std::vector<std::string> detectionLayers;
};

class AIPerceptionComponent : public Component {
public:
    EOA_CLASS_DECL(AIPerceptionComponent, Component)

    explicit AIPerceptionComponent(const std::string& name = "AIPerception");
    ~AIPerceptionComponent() override;

    // Конфигурация
    const PerceptionConfig& GetConfig() const { return config_; }
    void SetConfig(const PerceptionConfig& config) { config_ = config; }

    // Обновление восприятия
    void Tick(float deltaTime) override;

    // Регистрация стимулов
    void AddStimulus(const Stimulus& stimulus);
    void RemoveStimulus(Actor* source);

    // Получение обнаруженных стимулов
    const std::vector<Stimulus>& GetRecentStimuli() const { return recentStimuli_; }
    const std::vector<Stimulus>& GetCurrentStimuli() const { return currentStimuli_; }

    // Проверка видимости
    bool CanSee(Actor* target, float& outVisibility) const;
    bool CanHear(Actor* target) const;
    bool IsInFieldOfView(const glm::vec3& targetPos) const;

    // Callbacks
    using OnTargetPerceivedCallback = std::function<void(Actor*)>;
    using OnStimulusDetectedCallback = std::function<void(const Stimulus&)>;

    void SetOnTargetPerceived(OnTargetPerceivedCallback callback) { 
        onTargetPerceived_ = callback; 
    }
    void SetOnStimulusDetected(OnStimulusDetectedCallback callback) {
        onStimulusDetected_ = callback;
    }

    // Last known position of target
    glm::vec3 GetLastKnownPosition() const { return lastKnownPosition_; }
    double GetLastPerceptionTime() const { return lastPerceptionTime_; }

private:
    PerceptionConfig config_;
    std::vector<Stimulus> currentStimuli_;
    std::vector<Stimulus> recentStimuli_;
    
    glm::vec3 lastKnownPosition_;
    double lastPerceptionTime_ = 0.0;
    
    OnTargetPerceivedCallback onTargetPerceived_;
    OnStimulusDetectedCallback onStimulusDetected_;
    
    void UpdateVision(float deltaTime);
    void UpdateHearing(float deltaTime);
    bool LineOfSightCheck(const glm::vec3& from, const glm::vec3& to) const;
};

// ============================================================================
// Behavior Tree Nodes
// ============================================================================

enum class BTNodeState {
    Ready,
    Running,
    Success,
    Failure
};

class BehaviorTree;

// Базовый класс узла дерева поведения
class BTNode {
public:
    virtual ~BTNode() = default;
    virtual BTNodeState Execute(BehaviorTree* tree, float deltaTime) = 0;
    virtual void Reset() {}
    virtual std::string GetName() const = 0;
    
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }

protected:
    std::string name_;
};

// Узел действия
class BTAction : public BTNode {
public:
    using ActionFunc = std::function<BTNodeState(float)>;
    
    explicit BTAction(const std::string& name, ActionFunc func) 
        : func_(func) { 
        name_ = name;
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override {
        return func_(deltaTime);
    }
    
    void Reset() override {}
    std::string GetName() const override { return name_; }

private:
    ActionFunc func_;
};

// Узел условия
class BTCondition : public BTNode {
public:
    using ConditionFunc = std::function<bool()>;
    
    explicit BTCondition(const std::string& name, ConditionFunc func)
        : func_(func) {
        name_ = name;
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override {
        return func_() ? BTNodeState::Success : BTNodeState::Failure;
    }
    
    std::string GetName() const override { return name_; }

private:
    ConditionFunc func_;
};

// Selector (выбирает первый успешный дочерний узел)
class BTSelector : public BTNode {
public:
    explicit BTSelector(const std::string& name = "Selector") {
        name_ = name;
    }
    
    void AddChild(std::unique_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override;
    void Reset() override;
    std::string GetName() const override { return name_; }

private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t currentChildIndex_ = 0;
};

// Sequence (выполняет все дочерние узлы последовательно)
class BTSequence : public BTNode {
public:
    explicit BTSequence(const std::string& name = "Sequence") {
        name_ = name;
    }
    
    void AddChild(std::unique_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override;
    void Reset() override;
    std::string GetName() const override { return name_; }

private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t currentChildIndex_ = 0;
};

// Parallel (выполняет все дочерние узлы параллельно)
class BTParallel : public BTNode {
public:
    enum class Policy {
        RequireOne,    // Успех если хотя бы один успешен
        RequireAll,    // Успех если все успешны
        Majority       // Успех если большинство успешно
    };
    
    explicit BTParallel(const std::string& name = "Parallel", Policy policy = Policy::RequireOne)
        : policy_(policy) {
        name_ = name;
    }
    
    void AddChild(std::unique_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override;
    void Reset() override;
    std::string GetName() const override { return name_; }

private:
    std::vector<std::unique_ptr<BTNode>> children_;
    std::vector<BTNodeState> childStates_;
    Policy policy_;
};

// Decorator Inverter (инвертирует результат)
class BTInverter : public BTNode {
public:
    explicit BTInverter(const std::string& name = "Inverter") {
        name_ = name;
    }
    
    void SetChild(std::unique_ptr<BTNode> child) {
        child_ = std::move(child);
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override;
    void Reset() override;
    std::string GetName() const override { return name_; }

private:
    std::unique_ptr<BTNode> child_;
};

// Decorator Repeater (повторяет выполнение)
class BTRepeater : public BTNode {
public:
    explicit BTRepeater(const std::string& name = "Repeater", int numIterations = -1)
        : numIterations_(numIterations) {
        name_ = name;
    }
    
    void SetChild(std::unique_ptr<BTNode> child) {
        child_ = std::move(child);
    }
    
    BTNodeState Execute(BehaviorTree* tree, float deltaTime) override;
    void Reset() override;
    std::string GetName() const override { return name_; }

private:
    std::unique_ptr<BTNode> child_;
    int numIterations_;
    int currentIteration_ = 0;
};

// ============================================================================
// Blackboard - хранилище данных для AI
// ============================================================================

class Blackboard {
public:
    // Типы значений
    struct Value {
        enum Type { None, Bool, Int, Float, Vector, String, Actor };
        Type type = None;
        
        union {
            bool boolValue;
            int intValue;
            float floatValue;
            Actor* actorValue;
        };
        glm::vec3 vectorValue;
        std::string stringValue;
    };

    // Установка значений
    void SetValue(const std::string& key, bool value);
    void SetValue(const std::string& key, int value);
    void SetValue(const std::string& key, float value);
    void SetValue(const std::string& key, const glm::vec3& value);
    void SetValue(const std::string& key, const std::string& value);
    void SetValue(const std::string& key, Actor* value);

    // Получение значений
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    glm::vec3 GetVector(const std::string& key, const glm::vec3& defaultValue = glm::vec3(0)) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    Actor* GetActor(const std::string& key) const;

    // Проверка существования ключа
    bool HasKey(const std::string& key) const;
    void RemoveKey(const std::string& key);
    void Clear();

    // Callback при изменении значения
    using OnValueChangedCallback = std::function<void(const std::string&)>;
    void SetOnValueChanged(OnValueChangedCallback callback) { onValueChanged_ = callback; }

private:
    std::unordered_map<std::string, Value> values_;
    OnValueChangedCallback onValueChanged_;
};

// ============================================================================
// BehaviorTree - дерево поведения
// ============================================================================

class BehaviorTree {
public:
    explicit BehaviorTree(const std::string& name = "BehaviorTree");
    ~BehaviorTree();

    // Установка корня дерева
    void SetRoot(std::unique_ptr<BTNode> root) { root_ = std::move(root); }
    BTNode* GetRoot() const { return root_.get(); }

    // Обновление дерева
    void Update(float deltaTime);

    // Старт/стоп
    void Start();
    void Stop();
    bool IsRunning() const { return isRunning_; }

    // Blackboard
    Blackboard& GetBlackboard() { return blackboard_; }
    const Blackboard& GetBlackboard() const { return blackboard_; }

    // Владелец (Actor с AIController)
    Actor* GetOwner() const { return owner_; }
    void SetOwner(Actor* owner) { owner_ = owner; }

    // Статистика
    int GetCurrentDepth() const { return currentDepth_; }
    float GetTotalUpdateTime() const { return totalUpdateTime_; }

    // Сохранение/загрузка
    bool SaveToFile(const std::string& filename) const;
    bool LoadFromFile(const std::string& filename);

private:
    std::unique_ptr<BTNode> root_;
    Blackboard blackboard_;
    Actor* owner_ = nullptr;
    
    bool isRunning_ = false;
    int currentDepth_ = 0;
    float totalUpdateTime_ = 0.0f;
};

// ============================================================================
// AIControllerComponent - компонент управления AI
// ============================================================================

class AIControllerComponent : public Component {
public:
    EOA_CLASS_DECL(AIControllerComponent, Component)

    explicit AIControllerComponent(const std::string& name = "AIController");
    ~AIControllerComponent() override;

    // Инициализация
    void Initialize() override;
    void Tick(float deltaTime) override;

    // Behavior Tree
    BehaviorTree* GetBehaviorTree() const { return behaviorTree_.get(); }
    void SetBehaviorTree(std::unique_ptr<BehaviorTree> tree);
    void LoadBehaviorTree(const std::string& filename);

    // Perception
    AIPerceptionComponent* GetPerception() const { return perception_; }

    // Movement
    void MoveTo(const glm::vec3& destination, float speed = 5.0f);
    void StopMovement();
    bool IsMoving() const { return isMoving_; }
    glm::vec3 GetCurrentDestination() const { return currentDestination_; }

    // Focus
    Actor* GetFocusActor() const { return focusActor_; }
    void SetFocusActor(Actor* actor) { focusActor_ = actor; }
    glm::vec3 GetFocalPoint() const { return focalPoint_; }
    void SetFocalPoint(const glm::vec3& point) { focalPoint_ = point; }

    // State
    bool IsActive() const { return active_; }
    void SetActive(bool active) { active_ = active; }

    // Callbacks
    using OnMoveCompletedCallback = std::function<void(bool)>;
    void SetOnMoveCompleted(OnMoveCompletedCallback callback) {
        onMoveCompleted_ = callback;
    }

private:
    std::unique_ptr<BehaviorTree> behaviorTree_;
    AIPerceptionComponent* perception_ = nullptr;
    
    glm::vec3 currentDestination_;
    glm::vec3 focalPoint_;
    Actor* focusActor_ = nullptr;
    
    bool isMoving_ = false;
    bool active_ = true;
    float moveSpeed_ = 5.0f;
    
    OnMoveCompletedCallback onMoveCompleted_;
    
    void UpdateMovement(float deltaTime);
    void UpdatePerception(float deltaTime);
};

// ============================================================================
// NavMeshSystem - глобальная система навигации
// ============================================================================

class NavMeshSystem {
public:
    static NavMeshSystem& Get() {
        static NavMeshSystem instance;
        return instance;
    }

    // Регистрация/удаление NavMesh
    void RegisterNavMesh(NavMeshComponent* navmesh);
    void UnregisterNavMesh(NavMeshComponent* navmesh);

    // Поиск пути через все зарегистрированные NavMesh
    PathResult FindPath(const glm::vec3& start, const glm::vec3& end,
                        float agentRadius = 0.5f,
                        float agentHeight = 2.0f);

    // Получить ближайший NavMesh к точке
    NavMeshComponent* GetNearestNavMesh(const glm::vec3& position) const;

    // Все NavMesh
    const std::vector<NavMeshComponent*>& GetAllNavMeshes() const { return navMeshes_; }

private:
    NavMeshSystem() = default;
    ~NavMeshSystem() = default;
    
    std::vector<NavMeshComponent*> navMeshes_;
};

} // namespace eoa
