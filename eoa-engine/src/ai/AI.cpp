#include "AI/AI.h"
#include "core/actor.h"
#include "core/transform_component.h"
#include "core/world.h"
#include <glm/gtc/random.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace eoa {

// ============================================================================
// NavMeshComponent Implementation
// ============================================================================

NavMeshComponent::NavMeshComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(NavMeshComponent, Component)
}

NavMeshComponent::~NavMeshComponent() {
    Clear();
}

void NavMeshComponent::BuildFromGeometry(const std::vector<glm::vec3>& vertices,
                                          const std::vector<uint32_t>& indices,
                                          float cellSize,
                                          float cellHeight) {
    cellSize_ = cellSize;
    cellHeight_ = cellHeight;
    
    Clear();
    
    // Здесь будет реализация Recast/Detour для генерации NavMesh
    // 1. Вокселизация геометрии
    // 2. Выделение walkable поверхностей
    // 3. Создание полигонов
    // 4. Построение графа связей
    
    GenerateNavMesh();
}

void NavMeshComponent::Clear() {
    nodes_.clear();
    polygons_.clear();
}

void NavMeshComponent::GenerateNavMesh() {
    // Заглушка для генерации NavMesh
    // В полной реализации здесь будет:
    // - Использование Recast Navigation
    // - Вокселизация входной геометрии
    // - Выделение регионов
    // - Упрощение контуров
    // - Триангуляция в полигоны
    
    // Пример простого узла
    NavNode node;
    node.id = static_cast<int>(nodes_.size());
    node.position = glm::vec3(0, 0, 0);
    node.normal = glm::vec3(0, 1, 0);
    node.type = NavNodeType::Walkable;
    nodes_.push_back(node);
}

PathResult NavMeshComponent::FindPath(const glm::vec3& start, const glm::vec3& end,
                                       float agentRadius, float agentHeight) {
    PathResult result;
    
    int startNode = FindNearestNode(start);
    int endNode = FindNearestNode(end);
    
    if (startNode == -1 || endNode == -1) {
        return result;
    }
    
    return AStarSearch(startNode, endNode);
}

PathResult NavMeshComponent::FindPathSmooth(const glm::vec3& start, const glm::vec3& end,
                                             float agentRadius, float agentHeight) {
    PathResult result = FindPath(start, end, agentRadius, agentHeight);
    
    if (result.found) {
        SmoothPath(result.path);
    }
    
    return result;
}

PathResult NavMeshComponent::AStarSearch(int startNode, int endNode) {
    PathResult result;
    
    if (startNode < 0 || startNode >= static_cast<int>(nodes_.size()) ||
        endNode < 0 || endNode >= static_cast<int>(nodes_.size())) {
        return result;
    }
    
    // Инициализация A*
    for (auto& node : nodes_) {
        node.gCost = FLT_MAX;
        node.hCost = 0;
        node.parent = -1;
    }
    
    nodes_[startNode].gCost = 0;
    
    // Open и closed списки
    std::priority_queue<std::pair<float, int>, 
                        std::vector<std::pair<float, int>>,
                        std::greater<>> openSet;
    std::vector<bool> closedSet(nodes_.size(), false);
    
    openSet.push({0, startNode});
    
    while (!openSet.empty()) {
        int currentId = openSet.top().second;
        openSet.pop();
        
        if (closedSet[currentId]) continue;
        closedSet[currentId] = true;
        result.processedNodes++;
        
        if (currentId == endNode) {
            // Путь найден, восстанавливаем
            std::vector<glm::vec3> path;
            int node = endNode;
            while (node != -1) {
                path.push_back(nodes_[node].position);
                node = nodes_[node].parent;
            }
            std::reverse(path.begin(), path.end());
            result.path = path;
            result.found = true;
            
            // Считаем общую дистанцию
            for (size_t i = 1; i < path.size(); i++) {
                result.totalDistance += glm::distance(path[i-1], path[i]);
            }
            
            return result;
        }
        
        // Обрабатываем соседей
        for (int neighborId : nodes_[currentId].neighbors) {
            if (closedSet[neighborId] || !nodes_[neighborId].enabled) {
                continue;
            }
            
            float dist = glm::distance(nodes_[currentId].position, 
                                       nodes_[neighborId].position);
            float newGCost = nodes_[currentId].gCost + dist * nodes_[neighborId].cost;
            
            if (newGCost < nodes_[neighborId].gCost) {
                nodes_[neighborId].gCost = newGCost;
                nodes_[neighborId].hCost = glm::distance(nodes_[neighborId].position,
                                                         nodes_[endNode].position);
                nodes_[neighborId].parent = currentId;
                
                openSet.push({nodes_[neighborId].fCost(), neighborId});
            }
        }
    }
    
    return result;
}

void NavMeshComponent::SmoothPath(std::vector<glm::vec3>& path) {
    if (path.size() <= 2) return;
    
    // Упрощение пути (line-of-sight smoothing)
    std::vector<glm::vec3> smoothed;
    smoothed.push_back(path.front());
    
    size_t i = 0;
    while (i < path.size() - 1) {
        size_t j = path.size() - 1;
        
        // Ищем самую дальнюю точку, видимую из текущей
        while (j > i + 1) {
            // Простая проверка - в полной версии нужен raycast
            bool visible = true;
            
            if (visible) {
                break;
            }
            j--;
        }
        
        smoothed.push_back(path[j]);
        i = j;
    }
    
    path = smoothed;
}

glm::vec3 NavMeshComponent::ProjectPoint(const glm::vec3& point) const {
    if (nodes_.empty()) return point;
    
    int nearest = FindNearestNode(point);
    if (nearest == -1) return point;
    
    return nodes_[nearest].position;
}

glm::vec3 NavMeshComponent::GetRandomPoint() const {
    if (nodes_.empty()) return glm::vec3(0);
    
    size_t idx = rand() % nodes_.size();
    return nodes_[idx].position;
}

glm::vec3 NavMeshComponent::GetRandomPointAround(const glm::vec3& center, float radius) const {
    std::vector<int> candidates;
    
    for (size_t i = 0; i < nodes_.size(); i++) {
        if (nodes_[i].enabled && 
            glm::distance(nodes_[i].position, center) <= radius) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    
    if (candidates.empty()) return center;
    
    size_t idx = rand() % candidates.size();
    return nodes_[candidates[idx]].position;
}

bool NavMeshComponent::IsValidPosition(const glm::vec3& position, float radius) const {
    int nearest = FindNearestNode(position);
    if (nearest == -1) return false;
    
    const auto& node = nodes_[nearest];
    return node.enabled && node.type == NavNodeType::Walkable &&
           glm::distance(node.position, position) <= radius;
}

float NavMeshComponent::GetDistanceToNearestWalkable(const glm::vec3& position) const {
    float minDist = FLT_MAX;
    
    for (const auto& node : nodes_) {
        if (node.enabled && node.type == NavNodeType::Walkable) {
            float dist = glm::distance(node.position, position);
            minDist = std::min(minDist, dist);
        }
    }
    
    return minDist == FLT_MAX ? -1.0f : minDist;
}

int NavMeshComponent::FindNearestNode(const glm::vec3& position) const {
    if (nodes_.empty()) return -1;
    
    int nearest = -1;
    float minDist = FLT_MAX;
    
    for (size_t i = 0; i < nodes_.size(); i++) {
        float dist = glm::distance(nodes_[i].position, position);
        if (dist < minDist) {
            minDist = dist;
            nearest = static_cast<int>(i);
        }
    }
    
    return nearest;
}

void NavMeshComponent::Tick(float deltaTime) {
    if (needsRebuild_) {
        GenerateNavMesh();
        needsRebuild_ = false;
    }
}

bool NavMeshComponent::SaveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    // Сохранение узлов
    file << nodes_.size() << "\n";
    for (const auto& node : nodes_) {
        file << node.position.x << " " << node.position.y << " " << node.position.z << " ";
        file << node.normal.x << " " << node.normal.y << " " << node.normal.z << " ";
        file << node.type << " " << node.cost << " " << node.areaId << " " << node.enabled << "\n";
    }
    
    return true;
}

bool NavMeshComponent::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    Clear();
    
    size_t count;
    file >> count;
    
    for (size_t i = 0; i < count; i++) {
        NavNode node;
        node.id = static_cast<int>(i);
        file >> node.position.x >> node.position.y >> node.position.z;
        file >> node.normal.x >> node.normal.y >> node.normal.z;
        file >> node.type >> node.cost >> node.areaId >> node.enabled;
        nodes_.push_back(node);
    }
    
    return true;
}

// ============================================================================
// AIPerceptionComponent Implementation
// ============================================================================

AIPerceptionComponent::AIPerceptionComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(AIPerceptionComponent, Component)
}

AIPerceptionComponent::~AIPerceptionComponent() = default;

void AIPerceptionComponent::Tick(float deltaTime) {
    UpdateVision(deltaTime);
    UpdateHearing(deltaTime);
    
    // Удаляем старые стимулы
    double currentTime = 0.0; // Время от TimeSystem
    recentStimuli_.erase(
        std::remove_if(recentStimuli_.begin(), recentStimuli_.end(),
            [currentTime](const Stimulus& s) {
                return currentTime - s.timestamp > 5.0; // 5 секунд памяти
            }),
        recentStimuli_.end()
    );
}

void AIPerceptionComponent::AddStimulus(const Stimulus& stimulus) {
    currentStimuli_.push_back(stimulus);
    recentStimuli_.push_back(stimulus);
    
    if (onStimulusDetected_) {
        onStimulusDetected_(stimulus);
    }
}

void AIPerceptionComponent::RemoveStimulus(Actor* source) {
    currentStimuli_.erase(
        std::remove_if(currentStimuli_.begin(), currentStimuli_.end(),
            [source](const Stimulus& s) { return s.source == source; }),
        currentStimuli_.end()
    );
}

bool AIPerceptionComponent::CanSee(Actor* target, float& outVisibility) const {
    if (!owner_ || !target) {
        outVisibility = 0.0f;
        return false;
    }
    
    auto* ownerTransform = owner_->GetComponent<TransformComponent>();
    auto* targetTransform = target->GetComponent<TransformComponent>();
    
    if (!ownerTransform || !targetTransform) {
        outVisibility = 0.0f;
        return false;
    }
    
    glm::vec3 from = ownerTransform->GetPosition();
    glm::vec3 to = targetTransform->GetPosition();
    
    // Проверка дистанции
    float dist = glm::distance(from, to);
    if (dist > config_.sightRange) {
        outVisibility = 0.0f;
        return false;
    }
    
    // Проверка поля зрения
    if (!IsInFieldOfView(to)) {
        outVisibility = 0.0f;
        return false;
    }
    
    // Line of sight check
    if (!LineOfSightCheck(from, to)) {
        outVisibility = 0.0f;
        return false;
    }
    
    // Вычисляем видимость (0-1)
    outVisibility = 1.0f - (dist / config_.sightRange);
    return outVisibility > 0.5f;
}

bool AIPerceptionComponent::CanHear(Actor* target) const {
    if (!owner_ || !target) return false;
    
    auto* ownerTransform = owner_->GetComponent<TransformComponent>();
    auto* targetTransform = target->GetComponent<TransformComponent>();
    
    if (!ownerTransform || !targetTransform) return false;
    
    float dist = glm::distance(ownerTransform->GetPosition(),
                               targetTransform->GetPosition());
    return dist <= config_.hearingRange;
}

bool AIPerceptionComponent::IsInFieldOfView(const glm::vec3& targetPos) const {
    if (!owner_) return false;
    
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return false;
    
    glm::vec3 forward = transform->Forward();
    glm::vec3 toTarget = glm::normalize(targetPos - transform->GetPosition());
    
    float angle = glm::degrees(glm::acos(glm::dot(forward, toTarget)));
    return angle <= config_.fieldOfView * 0.5f;
}

void AIPerceptionComponent::UpdateVision(float deltaTime) {
    // Сканирование окружения на видимые цели
    // В полной реализации: raycast checks, frustum culling
}

void AIPerceptionComponent::UpdateHearing(float deltaTime) {
    // Слушание звуков окружения
    // В полной реализации: обработка audio events
}

bool AIPerceptionComponent::LineOfSightCheck(const glm::vec3& from, const glm::vec3& to) const {
    // Raycast для проверки прямой видимости
    HitResult hit = PhysicsWorld::Get().Raycast(from, glm::normalize(to - from),
                                                 glm::distance(from, to));
    return !hit.hit || hit.distance >= glm::distance(from, to) * 0.95f;
}

// ============================================================================
// Behavior Tree Nodes Implementation
// ============================================================================

BTNodeState BTSelector::Execute(BehaviorTree* tree, float deltaTime) {
    while (currentChildIndex_ < children_.size()) {
        BTNode* child = children_[currentChildIndex_].get();
        BTNodeState state = child->Execute(tree, deltaTime);
        
        if (state == BTNodeState::Running) {
            return BTNodeState::Running;
        }
        
        if (state == BTNodeState::Success) {
            return BTNodeState::Success;
        }
        
        currentChildIndex_++;
    }
    
    return BTNodeState::Failure;
}

void BTSelector::Reset() {
    currentChildIndex_ = 0;
    for (auto& child : children_) {
        child->Reset();
    }
}

BTNodeState BTSequence::Execute(BehaviorTree* tree, float deltaTime) {
    while (currentChildIndex_ < children_.size()) {
        BTNode* child = children_[currentChildIndex_].get();
        BTNodeState state = child->Execute(tree, deltaTime);
        
        if (state == BTNodeState::Running) {
            return BTNodeState::Running;
        }
        
        if (state == BTNodeState::Failure) {
            return BTNodeState::Failure;
        }
        
        currentChildIndex_++;
    }
    
    return BTNodeState::Success;
}

void BTSequence::Reset() {
    currentChildIndex_ = 0;
    for (auto& child : children_) {
        child->Reset();
    }
}

BTNodeState BTParallel::Execute(BehaviorTree* tree, float deltaTime) {
    if (childStates_.size() != children_.size()) {
        childStates_.resize(children_.size(), BTNodeState::Ready);
    }
    
    int successCount = 0;
    int failureCount = 0;
    int runningCount = 0;
    
    for (size_t i = 0; i < children_.size(); i++) {
        if (childStates_[i] == BTNodeState::Success) {
            successCount++;
            continue;
        }
        
        if (childStates_[i] == BTNodeState::Failure) {
            failureCount++;
            continue;
        }
        
        BTNodeState state = children_[i]->Execute(tree, deltaTime);
        childStates_[i] = state;
        
        if (state == BTNodeState::Running) {
            runningCount++;
        } else if (state == BTNodeState::Success) {
            successCount++;
        } else {
            failureCount++;
        }
    }
    
    switch (policy_) {
        case Policy::RequireOne:
            return successCount > 0 ? BTNodeState::Success :
                   (runningCount > 0 ? BTNodeState::Running : BTNodeState::Failure);
        case Policy::RequireAll:
            return failureCount > 0 ? BTNodeState::Failure :
                   (runningCount > 0 ? BTNodeState::Running : BTNodeState::Success);
        case Policy::Majority:
            return successCount > static_cast<int>(children_.size()) / 2 ?
                   BTNodeState::Success : BTNodeState::Failure;
    }
    
    return BTNodeState::Running;
}

void BTParallel::Reset() {
    childStates_.clear();
    for (auto& child : children_) {
        child->Reset();
    }
}

BTNodeState BTInverter::Execute(BehaviorTree* tree, float deltaTime) {
    if (!child_) return BTNodeState::Failure;
    
    BTNodeState state = child_->Execute(tree, deltaTime);
    
    if (state == BTNodeState::Success) return BTNodeState::Failure;
    if (state == BTNodeState::Failure) return BTNodeState::Success;
    return state;
}

void BTInverter::Reset() {
    if (child_) child_->Reset();
}

BTNodeState BTRepeater::Execute(BehaviorTree* tree, float deltaTime) {
    if (!child_) return BTNodeState::Failure;
    
    while (numIterations_ == -1 || currentIteration_ < numIterations_) {
        BTNodeState state = child_->Execute(tree, deltaTime);
        
        if (state == BTNodeState::Running) {
            return BTNodeState::Running;
        }
        
        if (state == BTNodeState::Failure) {
            return BTNodeState::Failure;
        }
        
        currentIteration_++;
        if (numIterations_ != -1 && currentIteration_ >= numIterations_) {
            return BTNodeState::Success;
        }
        
        child_->Reset();
    }
    
    return BTNodeState::Success;
}

void BTRepeater::Reset() {
    currentIteration_ = 0;
    if (child_) child_->Reset();
}

// ============================================================================
// Blackboard Implementation
// ============================================================================

void Blackboard::SetValue(const std::string& key, bool value) {
    Value& v = values_[key];
    v.type = Value::Bool;
    v.boolValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

void Blackboard::SetValue(const std::string& key, int value) {
    Value& v = values_[key];
    v.type = Value::Int;
    v.intValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

void Blackboard::SetValue(const std::string& key, float value) {
    Value& v = values_[key];
    v.type = Value::Float;
    v.floatValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

void Blackboard::SetValue(const std::string& key, const glm::vec3& value) {
    Value& v = values_[key];
    v.type = Value::Vector;
    v.vectorValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

void Blackboard::SetValue(const std::string& key, const std::string& value) {
    Value& v = values_[key];
    v.type = Value::String;
    v.stringValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

void Blackboard::SetValue(const std::string& key, Actor* value) {
    Value& v = values_[key];
    v.type = Value::Actor;
    v.actorValue = value;
    if (onValueChanged_) onValueChanged_(key);
}

bool Blackboard::GetBool(const std::string& key, bool defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::Bool) {
        return it->second.boolValue;
    }
    return defaultValue;
}

int Blackboard::GetInt(const std::string& key, int defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::Int) {
        return it->second.intValue;
    }
    return defaultValue;
}

float Blackboard::GetFloat(const std::string& key, float defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::Float) {
        return it->second.floatValue;
    }
    return defaultValue;
}

glm::vec3 Blackboard::GetVector(const std::string& key, const glm::vec3& defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::Vector) {
        return it->second.vectorValue;
    }
    return defaultValue;
}

std::string Blackboard::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::String) {
        return it->second.stringValue;
    }
    return defaultValue;
}

Actor* Blackboard::GetActor(const std::string& key) const {
    auto it = values_.find(key);
    if (it != values_.end() && it->second.type == Value::Actor) {
        return it->second.actorValue;
    }
    return nullptr;
}

bool Blackboard::HasKey(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void Blackboard::RemoveKey(const std::string& key) {
    values_.erase(key);
}

void Blackboard::Clear() {
    values_.clear();
}

// ============================================================================
// BehaviorTree Implementation
// ============================================================================

BehaviorTree::BehaviorTree(const std::string& name) {
    (void)name; // Suppress unused warning
}

BehaviorTree::~BehaviorTree() = default;

void BehaviorTree::Update(float deltaTime) {
    if (!isRunning_ || !root_) return;
    
    totalUpdateTime_ += deltaTime;
    currentDepth_ = 0;
    
    BTNodeState state = root_->Execute(this, deltaTime);
    
    if (state == BTNodeState::Success || state == BTNodeState::Failure) {
        root_->Reset();
    }
}

void BehaviorTree::Start() {
    isRunning_ = true;
    if (root_) root_->Reset();
}

void BehaviorTree::Stop() {
    isRunning_ = false;
    if (root_) root_->Reset();
}

bool BehaviorTree::SaveToFile(const std::string& filename) const {
    // Сериализация дерева поведения в JSON/XML
    return false;
}

bool BehaviorTree::LoadFromFile(const std::string& filename) {
    // Десериализация дерева поведения из JSON/XML
    return false;
}

// ============================================================================
// AIControllerComponent Implementation
// ============================================================================

AIControllerComponent::AIControllerComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(AIControllerComponent, Component)
}

AIControllerComponent::~AIControllerComponent() = default;

void AIControllerComponent::Initialize() {
    perception_ = owner_ ? owner_->GetComponent<AIPerceptionComponent>() : nullptr;
    
    if (!behaviorTree_ && owner_) {
        behaviorTree_ = std::make_unique<BehaviorTree>();
        behaviorTree_->SetOwner(owner_);
    }
}

void AIControllerComponent::Tick(float deltaTime) {
    if (!active_ || !owner_) return;
    
    UpdatePerception(deltaTime);
    UpdateMovement(deltaTime);
    
    if (behaviorTree_ && behaviorTree_->IsRunning()) {
        behaviorTree_->Update(deltaTime);
    }
}

void AIControllerComponent::SetBehaviorTree(std::unique_ptr<BehaviorTree> tree) {
    behaviorTree_ = std::move(tree);
    if (behaviorTree_ && owner_) {
        behaviorTree_->SetOwner(owner_);
        behaviorTree_->Start();
    }
}

void AIControllerComponent::LoadBehaviorTree(const std::string& filename) {
    if (!behaviorTree_) {
        behaviorTree_ = std::make_unique<BehaviorTree>();
        behaviorTree_->SetOwner(owner_);
    }
    
    if (behaviorTree_->LoadFromFile(filename)) {
        behaviorTree_->Start();
    }
}

void AIControllerComponent::MoveTo(const glm::vec3& destination, float speed) {
    currentDestination_ = destination;
    moveSpeed_ = speed;
    isMoving_ = true;
}

void AIControllerComponent::StopMovement() {
    isMoving_ = false;
    if (onMoveCompleted_) {
        onMoveCompleted_(true);
    }
}

void AIControllerComponent::UpdateMovement(float deltaTime) {
    if (!isMoving_ || !owner_) return;
    
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return;
    
    glm::vec3 currentPos = transform->GetPosition();
    glm::vec3 direction = currentDestination_ - currentPos;
    float distance = glm::length(direction);
    
    if (distance < 0.1f) {
        StopMovement();
        return;
    }
    
    direction = glm::normalize(direction);
    transform->SetPosition(currentPos + direction * moveSpeed_ * deltaTime);
    
    // Поворот в сторону движения
    glm::quat targetRotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));
    transform->SetRotation(targetRotation);
}

void AIControllerComponent::UpdatePerception(float deltaTime) {
    if (perception_) {
        perception_->Tick(deltaTime);
        
        // Обновление последней известной позиции цели
        const auto& stimuli = perception_->GetCurrentStimuli();
        if (!stimuli.empty()) {
            lastKnownPosition_ = stimuli.back().position;
            lastPerceptionTime_ = stimuli.back().timestamp;
        }
    }
}

// ============================================================================
// NavMeshSystem Implementation
// ============================================================================

void NavMeshSystem::RegisterNavMesh(NavMeshComponent* navmesh) {
    if (navmesh && std::find(navMeshes_.begin(), navMeshes_.end(), navmesh) == navMeshes_.end()) {
        navMeshes_.push_back(navmesh);
    }
}

void NavMeshSystem::UnregisterNavMesh(NavMeshComponent* navmesh) {
    navMeshes_.erase(
        std::remove(navMeshes_.begin(), navMeshes_.end(), navmesh),
        navMeshes_.end()
    );
}

PathResult NavMeshSystem::FindPath(const glm::vec3& start, const glm::vec3& end,
                                    float agentRadius, float agentHeight) {
    PathResult bestResult;
    
    for (auto* navmesh : navMeshes_) {
        PathResult result = navmesh->FindPath(start, end, agentRadius, agentHeight);
        
        if (result.found && (result.totalDistance < bestResult.totalDistance || !bestResult.found)) {
            bestResult = result;
        }
    }
    
    return bestResult;
}

NavMeshComponent* NavMeshSystem::GetNearestNavMesh(const glm::vec3& position) const {
    NavMeshComponent* nearest = nullptr;
    float minDist = FLT_MAX;
    
    for (auto* navmesh : navMeshes_) {
        float dist = glm::distance(position, glm::vec3(0)); // Упрощённо
        if (dist < minDist) {
            minDist = dist;
            nearest = navmesh;
        }
    }
    
    return nearest;
}

} // namespace eoa
