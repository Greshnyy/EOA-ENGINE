#pragma once
#include "core/component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace eoa {

// ============================================================================
// ФИЗИЧЕСКИЕ КОМПОНЕНТЫ
// ============================================================================

enum class ColliderType : uint8_t {
    Box,
    Sphere,
    Capsule,
    Mesh,
    Heightfield
};

enum class CollisionLayer : uint32_t {
    Default = 1 << 0,
    Static = 1 << 1,
    Dynamic = 1 << 2,
    Character = 1 << 3,
    Trigger = 1 << 4,
    Vehicle = 1 << 5,
    Projectile = 1 << 6,
    UI = 1 << 7,
    All = 0xFFFFFFFF
};

// Структура для результата raycast
struct HitResult {
    bool hit = false;
    glm::vec3 point;
    glm::vec3 normal;
    float distance = 0.0f;
    Actor* actor = nullptr;
    Component* component = nullptr;
    int triangleIndex = -1;
};

// ============================================================================
// RigidbodyComponent - физическое тело
// ============================================================================

class RigidbodyComponent : public Component {
public:
    EOA_CLASS_DECL(RigidbodyComponent, Component)

    explicit RigidbodyComponent(const std::string& name = "Rigidbody");
    ~RigidbodyComponent() override;

    // Инициализация физики
    void InitializePhysics();
    void ShutdownPhysics();

    // Обновление физики
    void Tick(float deltaTime) override;

    // Свойства
    float GetMass() const { return mass_; }
    void SetMass(float mass);

    bool IsKinematic() const { return isKinematic_; }
    void SetKinematic(bool kinematic);

    bool UseGravity() const { return useGravity_; }
    void SetUseGravity(bool gravity) { useGravity_ = gravity; }

    float GetLinearDamping() const { return linearDamping_; }
    void SetLinearDamping(float damping) { linearDamping_ = damping; }

    float GetAngularDamping() const { return angularDamping_; }
    void SetAngularDamping(float damping) { angularDamping_ = damping; }

    // Силы и импульсы
    void AddForce(const glm::vec3& force);
    void AddForceAtPosition(const glm::vec3& force, const glm::vec3& position);
    void AddTorque(const glm::vec3& torque);
    void AddImpulse(const glm::vec3& impulse);
    void AddAngularImpulse(const glm::vec3& impulse);

    // Скорости
    glm::vec3 GetLinearVelocity() const;
    void SetLinearVelocity(const glm::vec3& velocity);

    glm::vec3 GetAngularVelocity() const;
    void SetAngularVelocity(const glm::vec3& velocity);

    // Позиция и вращение
    glm::vec3 GetPhysicsPosition() const;
    void SetPhysicsPosition(const glm::vec3& pos);

    glm::quat GetPhysicsRotation() const;
    void SetPhysicsRotation(const glm::quat& rot);

    // Статус
    bool IsSleeping() const;
    void WakeUp();
    void PutToSleep();

    // Коллизионные слои
    CollisionLayer GetCollisionLayer() const { return collisionLayer_; }
    void SetCollisionLayer(CollisionLayer layer);

    uint32_t GetCollisionMask() const { return collisionMask_; }
    void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }

private:
    float mass_ = 1.0f;
    bool isKinematic_ = false;
    bool useGravity_ = true;
    float linearDamping_ = 0.05f;
    float angularDamping_ = 0.05f;
    
    CollisionLayer collisionLayer_ = CollisionLayer::Dynamic;
    uint32_t collisionMask_ = static_cast<uint32_t>(CollisionLayer::All);
    
    // Внутренний указатель на физическое тело (Jolt/Bullet)
    void* physicsBody_ = nullptr;
    bool initialized_ = false;
};

// ============================================================================
// ColliderComponent - компонент коллизии
// ============================================================================

class ColliderComponent : public Component {
public:
    EOA_CLASS_DECL(ColliderComponent, Component)

    explicit ColliderComponent(const std::string& name = "Collider");
    ~ColliderComponent() override;

    // Тип коллайдера
    ColliderType GetColliderType() const { return type_; }
    void SetColliderType(ColliderType type);

    // Размеры для разных типов
    const glm::vec3& GetBoxSize() const { return boxSize_; }
    void SetBoxSize(const glm::vec3& size) { boxSize_ = size; }

    float GetSphereRadius() const { return sphereRadius_; }
    void SetSphereRadius(float radius) { sphereRadius_ = radius; }

    float GetCapsuleRadius() const { return capsuleRadius_; }
    void SetCapsuleRadius(float radius) { capsuleRadius_ = radius; }
    float GetCapsuleHeight() const { return capsuleHeight_; }
    void SetCapsuleHeight(float height) { capsuleHeight_ = height; }

    // Convex mesh
    void SetConvexMesh(const std::vector<glm::vec3>& vertices);

    // Флаг триггера
    bool IsTrigger() const { return isTrigger_; }
    void SetIsTrigger(bool trigger) { isTrigger_ = trigger; }

    // Материал физики
    float GetFriction() const { return friction_; }
    void SetFriction(float friction) { friction_ = friction; }

    float GetRestitution() const { return restitution_; }
    void SetRestitution(float restitution) { restitution_ = restitution; }

    // Callbacks
    using OnCollisionCallback = std::function<void(Actor*, const HitResult&)>;
    using OnTriggerCallback = std::function<void(Actor*, bool)>;

    void SetOnCollision(OnCollisionCallback callback) { onCollision_ = callback; }
    void SetOnTrigger(OnTriggerCallback callback) { onTrigger_ = callback; }

    // Инициализация
    void InitializePhysics();
    void ShutdownPhysics();

private:
    ColliderType type_ = ColliderType::Box;
    glm::vec3 boxSize_ = glm::vec3(1.0f);
    float sphereRadius_ = 0.5f;
    float capsuleRadius_ = 0.5f;
    float capsuleHeight_ = 1.0f;
    std::vector<glm::vec3> meshVertices_;
    
    bool isTrigger_ = false;
    float friction_ = 0.5f;
    float restitution_ = 0.0f;
    
    OnCollisionCallback onCollision_;
    OnTriggerCallback onTrigger_;
    
    void* physicsShape_ = nullptr;
    bool initialized_ = false;
};

// ============================================================================
// CharacterControllerComponent - контроллер персонажа
// ============================================================================

class CharacterControllerComponent : public Component {
public:
    EOA_CLASS_DECL(CharacterControllerComponent, Component)

    explicit CharacterControllerComponent(const std::string& name = "CharacterController");
    ~CharacterControllerComponent() override;

    // Размеры
    float GetHeight() const { return height_; }
    void SetHeight(float height) { height_ = height; }

    float GetRadius() const { return radius_; }
    void SetRadius(float radius) { radius_ = radius; }

    // Движение
    void Move(const glm::vec3& deltaPosition);
    void SetMoveInput(const glm::vec2& input);
    void Jump();
    void StopJumping();

    // Состояние
    bool IsGrounded() const;
    bool CanJump() const;

    float GetJumpForce() const { return jumpForce_; }
    void SetJumpForce(float force) { jumpForce_ = force; }

    float GetMoveSpeed() const { return moveSpeed_; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }

    float GetAcceleration() const { return acceleration_; }
    void SetAcceleration(float accel) { acceleration_ = accel; }

    float GetDeceleration() const { return deceleration_; }
    void SetDeceleration(float decel) { deceleration_ = decel; }

    // Наклоны
    float GetMaxSlopeAngle() const { return maxSlopeAngle_; }
    void SetMaxSlopeAngle(float degrees) { maxSlopeAngle_ = degrees; }

    // Step offset
    float GetStepOffset() const { return stepOffset_; }
    void SetStepOffset(float offset) { stepOffset_ = offset; }

    // Текущая скорость
    glm::vec3 GetVelocity() const { return velocity_; }

    // Raycast вниз
    HitResult GroundProbe() const;

    // Инициализация
    void InitializePhysics();
    void ShutdownPhysics();
    void Tick(float deltaTime) override;

private:
    float height_ = 1.8f;
    float radius_ = 0.4f;
    float jumpForce_ = 10.0f;
    float moveSpeed_ = 5.0f;
    float acceleration_ = 10.0f;
    float deceleration_ = 10.0f;
    float maxSlopeAngle_ = 45.0f;
    float stepOffset_ = 0.5f;

    glm::vec2 moveInput_;
    glm::vec3 velocity_;
    bool isGrounded_ = false;
    bool isJumping_ = false;

    void* characterController_ = nullptr;
    bool initialized_ = false;
};

// ============================================================================
// VehicleComponent - транспортное средство
// ============================================================================

class VehicleComponent : public Component {
public:
    EOA_CLASS_DECL(VehicleComponent, Component)

    explicit VehicleComponent(const std::string& name = "Vehicle");
    ~VehicleComponent() override;

    // Управление
    void SetThrottle(float throttle); // -1 to 1
    void SetSteering(float steering); // -1 to 1
    void SetBrake(float brake);       // 0 to 1
    void SetHandbrake(bool handbrake);

    // Состояние
    float GetSpeed() const;
    float GetEngineRPM() const { return engineRPM_; }
    bool IsInAir() const;

    // Настройки двигателя
    float GetEngineForce() const { return engineForce_; }
    void SetEngineForce(float force) { engineForce_ = force; }

    float GetBrakeForce() const { return brakeForce_; }
    void SetBrakeForce(float force) { brakeForce_ = force; }

    float GetMaxSteeringAngle() const { return maxSteeringAngle_; }
    void SetMaxSteeringAngle(float degrees) { maxSteeringAngle_ = degrees; }

    // Инициализация
    void InitializePhysics();
    void ShutdownPhysics();
    void Tick(float deltaTime) override;

private:
    float engineForce_ = 2000.0f;
    float brakeForce_ = 100.0f;
    float maxSteeringAngle_ = 30.0f;
    
    float throttle_ = 0.0f;
    float steering_ = 0.0f;
    float brake_ = 0.0f;
    bool handbrake_ = false;
    
    float engineRPM_ = 0.0f;
    
    void* vehicle_ = nullptr;
    bool initialized_ = false;
};

// ============================================================================
// PHYSICS WORLD - менеджер физического мира
// ============================================================================

class PhysicsWorld {
public:
    static PhysicsWorld& Get() {
        static PhysicsWorld instance;
        return instance;
    }

    // Инициализация/завершение
    void Initialize();
    void Shutdown();

    // Обновление физики
    void Simulate(float deltaTime);

    // Создание/удаление тел
    void CreateRigidbody(RigidbodyComponent* rb);
    void RemoveRigidbody(RigidbodyComponent* rb);

    void CreateCollider(ColliderComponent* collider);
    void RemoveCollider(ColliderComponent* collider);

    void CreateCharacterController(CharacterControllerComponent* cc);
    void RemoveCharacterController(CharacterControllerComponent* cc);

    // Raycasting
    HitResult Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                      float maxDistance, uint32_t collisionMask = 0xFFFFFFFF);
    
    HitResult RaycastClosest(const glm::vec3& origin, const glm::vec3& direction,
                             float maxDistance, uint32_t collisionMask = 0xFFFFFFFF);

    std::vector<HitResult> RaycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                      float maxDistance, uint32_t collisionMask = 0xFFFFFFFF);

    // Overlap тесты
    bool OverlapSphere(const glm::vec3& center, float radius,
                       std::vector<Actor*>& results, uint32_t collisionMask = 0xFFFFFFFF);
    
    bool OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                    const glm::quat& rotation, std::vector<Actor*>& results,
                    uint32_t collisionMask = 0xFFFFFFFF);

    // Гравитация
    const glm::vec3& GetGravity() const { return gravity_; }
    void SetGravity(const glm::vec3& gravity) { gravity_ = gravity; }

    // Слои коллизий
    void SetCollisionEnabled(CollisionLayer layer1, CollisionLayer layer2, bool enabled);
    bool IsCollisionEnabled(CollisionLayer layer1, CollisionLayer layer2) const;

    // Debug visualization
    bool IsDebugVisualizationEnabled() const { return debugVisualization_; }
    void SetDebugVisualizationEnabled(bool enabled) { debugVisualization_ = enabled; }

    // Статистика
    int GetActiveBodiesCount() const;
    int GetCollidersCount() const;
    float GetSimulationTime() const { return simulationTime_; }

private:
    PhysicsWorld() = default;
    ~PhysicsWorld();

    glm::vec3 gravity_ = glm::vec3(0.0f, -9.81f, 0.0f);
    float simulationTime_ = 0.0f;
    bool debugVisualization_ = false;

    // Матрица коллизий [layer][mask]
    bool collisionMatrix_[32][32];

    // Внутренний мир физики
    void* physicsSystem_ = nullptr;
    void* physicsScene_ = nullptr;
};

// ============================================================================
// УТИЛИТЫ ФИЗИКИ
// ============================================================================

namespace PhysicsUtils {
    // Конвертация типов
    ColliderType StringToColliderType(const std::string& str);
    std::string ColliderTypeToString(ColliderType type);

    // Вычисление массы для примитивов
    float CalculateBoxMass(const glm::vec3& size, float density);
    float CalculateSphereMass(float radius, float density);
    float CalculateCapsuleMass(float radius, float height, float density);

    // Момент инерции
    glm::vec3 CalculateBoxInertia(const glm::vec3& size, float mass);
    glm::vec3 CalculateSphereInertia(float radius, float mass);
}

} // namespace eoa
