#include "Physics/Physics.h"
#include "core/actor.h"
#include "core/transform_component.h"
#include "core/world.h"
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace eoa {

// ============================================================================
// RigidbodyComponent Implementation
// ============================================================================

RigidbodyComponent::RigidbodyComponent(const std::string& name) 
    : Component(name) {
    EOA_CLASS_CONSTRUCT(RigidbodyComponent, Component)
}

RigidbodyComponent::~RigidbodyComponent() {
    ShutdownPhysics();
}

void RigidbodyComponent::InitializePhysics() {
    if (initialized_) return;
    
    // Здесь будет интеграция с Jolt Physics
    // physicsBody_ = new Jolt::RigidBody();
    // ... настройка тела
    
    initialized_ = true;
}

void RigidbodyComponent::ShutdownPhysics() {
    if (!initialized_) return;
    
    // Удаление физического тела
    // delete physicsBody_;
    physicsBody_ = nullptr;
    initialized_ = false;
}

void RigidbodyComponent::Tick(float deltaTime) {
    if (!initialized_ || !active_) return;
    
    // Синхронизация трансформации с физическим телом
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (transform && physicsBody_) {
        // Получаем позицию и вращение от физического движка
        glm::vec3 pos = GetPhysicsPosition();
        glm::quat rot = GetPhysicsRotation();
        
        transform->SetPosition(pos);
        transform->SetRotation(rot);
    }
}

void RigidbodyComponent::SetMass(float mass) {
    mass_ = mass;
    if (physicsBody_) {
        // Обновление массы в физическом движке
        // physicsBody_->SetMass(mass);
    }
}

void RigidbodyComponent::SetKinematic(bool kinematic) {
    isKinematic_ = kinematic;
    if (physicsBody_) {
        // physicsBody_->SetMotionType(kinematic ? Kinematic : Dynamic);
    }
}

void RigidbodyComponent::AddForce(const glm::vec3& force) {
    if (physicsBody_ && !isKinematic_) {
        // physicsBody_->AddForce(force);
    }
}

void RigidbodyComponent::AddForceAtPosition(const glm::vec3& force, const glm::vec3& position) {
    if (physicsBody_ && !isKinematic_) {
        // physicsBody_->AddForceAtPos(force, position);
    }
}

void RigidbodyComponent::AddTorque(const glm::vec3& torque) {
    if (physicsBody_ && !isKinematic_) {
        // physicsBody_->AddTorque(torque);
    }
}

void RigidbodyComponent::AddImpulse(const glm::vec3& impulse) {
    if (physicsBody_ && !isKinematic_) {
        // physicsBody_->AddImpulse(impulse);
    }
}

void RigidbodyComponent::AddAngularImpulse(const glm::vec3& impulse) {
    if (physicsBody_ && !isKinematic_) {
        // physicsBody_->AddAngularImpulse(impulse);
    }
}

glm::vec3 RigidbodyComponent::GetLinearVelocity() const {
    if (physicsBody_) {
        // return physicsBody_->GetLinearVelocity();
    }
    return glm::vec3(0.0f);
}

void RigidbodyComponent::SetLinearVelocity(const glm::vec3& velocity) {
    if (physicsBody_) {
        // physicsBody_->SetLinearVelocity(velocity);
    }
}

glm::vec3 RigidbodyComponent::GetAngularVelocity() const {
    if (physicsBody_) {
        // return physicsBody_->GetAngularVelocity();
    }
    return glm::vec3(0.0f);
}

void RigidbodyComponent::SetAngularVelocity(const glm::vec3& velocity) {
    if (physicsBody_) {
        // physicsBody_->SetAngularVelocity(velocity);
    }
}

glm::vec3 RigidbodyComponent::GetPhysicsPosition() const {
    if (physicsBody_) {
        // return physicsBody_->GetPosition();
    }
    if (owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) return transform->GetPosition();
    }
    return glm::vec3(0.0f);
}

void RigidbodyComponent::SetPhysicsPosition(const glm::vec3& pos) {
    if (physicsBody_) {
        // physicsBody_->SetPosition(pos);
    }
    if (owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) transform->SetPosition(pos);
    }
}

glm::quat RigidbodyComponent::GetPhysicsRotation() const {
    if (physicsBody_) {
        // return physicsBody_->GetRotation();
    }
    if (owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) return transform->GetRotation();
    }
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void RigidbodyComponent::SetPhysicsRotation(const glm::quat& rot) {
    if (physicsBody_) {
        // physicsBody_->SetRotation(rot);
    }
    if (owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) transform->SetRotation(rot);
    }
}

bool RigidbodyComponent::IsSleeping() const {
    if (physicsBody_) {
        // return physicsBody_->IsSleeping();
    }
    return false;
}

void RigidbodyComponent::WakeUp() {
    if (physicsBody_) {
        // physicsBody_->WakeUp();
    }
}

void RigidbodyComponent::PutToSleep() {
    if (physicsBody_) {
        // physicsBody_->PutToSleep();
    }
}

void RigidbodyComponent::SetCollisionLayer(CollisionLayer layer) {
    collisionLayer_ = layer;
    if (physicsBody_) {
        // physicsBody_->SetCollisionGroup(layer);
    }
}

// ============================================================================
// ColliderComponent Implementation
// ============================================================================

ColliderComponent::ColliderComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(ColliderComponent, Component)
}

ColliderComponent::~ColliderComponent() {
    ShutdownPhysics();
}

void ColliderComponent::SetColliderType(ColliderType type) {
    type_ = type;
    if (initialized_) {
        ShutdownPhysics();
        InitializePhysics();
    }
}

void ColliderComponent::SetConvexMesh(const std::vector<glm::vec3>& vertices) {
    meshVertices_ = vertices;
    type_ = ColliderType::Mesh;
    if (initialized_) {
        ShutdownPhysics();
        InitializePhysics();
    }
}

void ColliderComponent::InitializePhysics() {
    if (initialized_) return;
    
    // Создание физической формы на основе типа
    // switch (type_) {
    //     case ColliderType::Box:
    //         physicsShape_ = new Jolt::BoxShape(boxSize_ * 0.5f);
    //         break;
    //     case ColliderType::Sphere:
    //         physicsShape_ = new Jolt::SphereShape(sphereRadius_);
    //         break;
    //     case ColliderType::Capsule:
    //         physicsShape_ = new Jolt::CapsuleShape(capsuleHeight_, capsuleRadius_);
    //         break;
    //     case ColliderType::Mesh:
    //         physicsShape_ = new Jolt::ConvexHullShape(meshVertices_);
    //         break;
    // }
    
    initialized_ = true;
}

void ColliderComponent::ShutdownPhysics() {
    if (!initialized_) return;
    
    // delete physicsShape_;
    physicsShape_ = nullptr;
    initialized_ = false;
}

// ============================================================================
// CharacterControllerComponent Implementation
// ============================================================================

CharacterControllerComponent::CharacterControllerComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(CharacterControllerComponent, Component)
}

CharacterControllerComponent::~CharacterControllerComponent() {
    ShutdownPhysics();
}

void CharacterControllerComponent::Move(const glm::vec3& deltaPosition) {
    if (!initialized_ || !characterController_) return;
    
    // Перемещение контроллера
    // characterController_->Move(deltaPosition);
}

void CharacterControllerComponent::SetMoveInput(const glm::vec2& input) {
    moveInput_ = input;
}

void CharacterControllerComponent::Jump() {
    if (CanJump()) {
        isJumping_ = true;
        velocity_.y = jumpForce_;
        isGrounded_ = false;
    }
}

void CharacterControllerComponent::StopJumping() {
    if (velocity_.y > 0) {
        velocity_.y *= 0.5f;
    }
}

bool CharacterControllerComponent::IsGrounded() const {
    return isGrounded_;
}

bool CharacterControllerComponent::CanJump() const {
    return isGrounded_ && !isJumping_;
}

HitResult CharacterControllerComponent::GroundProbe() const {
    HitResult result;
    
    if (!owner_) return result;
    
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return result;
    
    glm::vec3 origin = transform->GetPosition();
    origin.y += radius_;
    
    // Raycast вниз
    result = PhysicsWorld::Get().Raycast(origin, glm::vec3(0, -1, 0), 
                                         height_ * 0.5f + radius_);
    
    return result;
}

void CharacterControllerComponent::InitializePhysics() {
    if (initialized_) return;
    
    // Создание контроллера персонажа
    // characterController_ = new Jolt::CharacterController(...);
    
    initialized_ = true;
}

void CharacterControllerComponent::ShutdownPhysics() {
    if (!initialized_) return;
    
    // delete characterController_;
    characterController_ = nullptr;
    initialized_ = false;
}

void CharacterControllerComponent::Tick(float deltaTime) {
    if (!initialized_ || !active_ || !owner_) return;
    
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return;
    
    // Проверка земли
    HitResult ground = GroundProbe();
    isGrounded_ = ground.hit;
    
    if (isGrounded_ && isJumping_) {
        isJumping_ = false;
    }
    
    // Применяем гравитацию
    if (!isGrounded_) {
        velocity_.y += PhysicsWorld::Get().GetGravity().y * deltaTime;
    }
    
    // Движение по горизонтали
    if (moveInput_ != glm::vec2(0)) {
        glm::vec3 forward = transform->Forward();
        forward.y = 0;
        forward = glm::normalize(forward);
        
        glm::vec3 right = transform->Right();
        right.y = 0;
        right = glm::normalize(right);
        
        glm::vec3 moveDir = forward * moveInput_.y + right * moveInput_.x;
        moveDir = glm::normalize(moveDir);
        
        // Плавное ускорение/замедление
        float targetSpeed = glm::length(moveInput_) > 0 ? moveSpeed_ : 0;
        float currentSpeed = glm::length(glm::vec2(velocity_.x, velocity_.z));
        
        if (currentSpeed < targetSpeed) {
            currentSpeed += acceleration_ * deltaTime;
        } else {
            currentSpeed -= deceleration_ * deltaTime;
        }
        currentSpeed = glm::clamp(currentSpeed, 0.0f, targetSpeed);
        
        velocity_.x = moveDir.x * currentSpeed;
        velocity_.z = moveDir.z * currentSpeed;
    } else {
        // Замедление когда нет ввода
        velocity_.x *= (1.0f - deceleration_ * deltaTime);
        velocity_.z *= (1.0f - deceleration_ * deltaTime);
    }
    
    // Применяем движение
    glm::vec3 deltaPos = velocity_ * deltaTime;
    Move(deltaPos);
}

// ============================================================================
// VehicleComponent Implementation
// ============================================================================

VehicleComponent::VehicleComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(VehicleComponent, Component)
}

VehicleComponent::~VehicleComponent() {
    ShutdownPhysics();
}

void VehicleComponent::SetThrottle(float throttle) {
    throttle_ = glm::clamp(throttle, -1.0f, 1.0f);
}

void VehicleComponent::SetSteering(float steering) {
    steering_ = glm::clamp(steering, -1.0f, 1.0f);
}

void VehicleComponent::SetBrake(float brake) {
    brake_ = glm::clamp(brake, 0.0f, 1.0f);
}

void VehicleComponent::SetHandbrake(bool handbrake) {
    handbrake_ = handbrake;
}

float VehicleComponent::GetSpeed() const {
    if (vehicle_ && owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) {
            // return physicsBody_->GetLinearVelocity().Length();
        }
    }
    return 0.0f;
}

bool VehicleComponent::IsInAir() const {
    // Проверка, все ли колеса на земле
    return false;
}

void VehicleComponent::InitializePhysics() {
    if (initialized_) return;
    
    // Создание транспортного средства
    // vehicle_ = new Jolt::Vehicle(...);
    
    initialized_ = true;
}

void VehicleComponent::ShutdownPhysics() {
    if (!initialized_) return;
    
    // delete vehicle_;
    vehicle_ = nullptr;
    initialized_ = false;
}

void VehicleComponent::Tick(float deltaTime) {
    if (!initialized_ || !vehicle_ || !owner_) return;
    
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return;
    
    // Применение управления к транспортному средству
    // vehicle_->SetThrottle(throttle_);
    // vehicle_->SetSteering(steering_ * maxSteeringAngle_);
    // vehicle_->SetBrake(brake_ * brakeForce_);
    // vehicle_->SetHandbrake(handbrake_);
    
    // Обновление RPM двигателя
    engineRPM_ = throttle_ * 6000.0f; // Упрощённо
}

// ============================================================================
// PhysicsWorld Implementation
// ============================================================================

PhysicsWorld::~PhysicsWorld() {
    Shutdown();
}

void PhysicsWorld::Initialize() {
    // Инициализация физического движка (Jolt Physics)
    // physicsSystem_ = new Jolt::PhysicsSystem();
    // physicsScene_ = new Jolt::PhysicsScene();
    
    // Инициализация матрицы коллизий
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            collisionMatrix_[i][j] = true;
        }
    }
}

void PhysicsWorld::Shutdown() {
    // Удаление физического мира
    // delete physicsScene_;
    // delete physicsSystem_;
    physicsScene_ = nullptr;
    physicsSystem_ = nullptr;
}

void PhysicsWorld::Simulate(float deltaTime) {
    if (!physicsSystem_) return;
    
    simulationTime_ += deltaTime;
    
    // Шаг симуляции физики
    // physicsSystem_->Update(deltaTime);
}

void PhysicsWorld::CreateRigidbody(RigidbodyComponent* rb) {
    if (!rb) return;
    rb->InitializePhysics();
}

void PhysicsWorld::RemoveRigidbody(RigidbodyComponent* rb) {
    if (!rb) return;
    rb->ShutdownPhysics();
}

void PhysicsWorld::CreateCollider(ColliderComponent* collider) {
    if (!collider) return;
    collider->InitializePhysics();
}

void PhysicsWorld::RemoveCollider(ColliderComponent* collider) {
    if (!collider) return;
    collider->ShutdownPhysics();
}

void PhysicsWorld::CreateCharacterController(CharacterControllerComponent* cc) {
    if (!cc) return;
    cc->InitializePhysics();
}

void PhysicsWorld::RemoveCharacterController(CharacterControllerComponent* cc) {
    if (!cc) return;
    cc->ShutdownPhysics();
}

HitResult PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction,
                                 float maxDistance, uint32_t collisionMask) {
    HitResult result;
    
    // Raycast реализация через физический движок
    // Jolt::RayCast ray(origin, origin + direction * maxDistance);
    // ... проверка коллизий
    
    return result;
}

HitResult PhysicsWorld::RaycastClosest(const glm::vec3& origin, const glm::vec3& direction,
                                        float maxDistance, uint32_t collisionMask) {
    return Raycast(origin, direction, maxDistance, collisionMask);
}

std::vector<HitResult> PhysicsWorld::RaycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                                 float maxDistance, uint32_t collisionMask) {
    std::vector<HitResult> results;
    
    // GetAllHits реализация
    // ...
    
    return results;
}

bool PhysicsWorld::OverlapSphere(const glm::vec3& center, float radius,
                                  std::vector<Actor*>& results, uint32_t collisionMask) {
    // Sphere overlap тест
    // ...
    return !results.empty();
}

bool PhysicsWorld::OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                               const glm::quat& rotation, std::vector<Actor*>& results,
                               uint32_t collisionMask) {
    // Box overlap тест
    // ...
    return !results.empty();
}

void PhysicsWorld::SetCollisionEnabled(CollisionLayer layer1, CollisionLayer layer2, bool enabled) {
    collisionMatrix_[static_cast<int>(layer1)][static_cast<int>(layer2)] = enabled;
    collisionMatrix_[static_cast<int>(layer2)][static_cast<int>(layer1)] = enabled;
}

bool PhysicsWorld::IsCollisionEnabled(CollisionLayer layer1, CollisionLayer layer2) const {
    return collisionMatrix_[static_cast<int>(layer1)][static_cast<int>(layer2)];
}

int PhysicsWorld::GetActiveBodiesCount() const {
    // Вернуть количество активных тел
    return 0;
}

int PhysicsWorld::GetCollidersCount() const {
    // Вернуть количество коллайдеров
    return 0;
}

// ============================================================================
// PhysicsUtils Implementation
// ============================================================================

namespace PhysicsUtils {

ColliderType StringToColliderType(const std::string& str) {
    if (str == "Box") return ColliderType::Box;
    if (str == "Sphere") return ColliderType::Sphere;
    if (str == "Capsule") return ColliderType::Capsule;
    if (str == "Mesh") return ColliderType::Mesh;
    if (str == "Heightfield") return ColliderType::Heightfield;
    return ColliderType::Box;
}

std::string ColliderTypeToString(ColliderType type) {
    switch (type) {
        case ColliderType::Box: return "Box";
        case ColliderType::Sphere: return "Sphere";
        case ColliderType::Capsule: return "Capsule";
        case ColliderType::Mesh: return "Mesh";
        case ColliderType::Heightfield: return "Heightfield";
        default: return "Box";
    }
}

float CalculateBoxMass(const glm::vec3& size, float density) {
    float volume = size.x * size.y * size.z;
    return volume * density;
}

float CalculateSphereMass(float radius, float density) {
    float volume = (4.0f / 3.0f) * glm::pi<float>() * radius * radius * radius;
    return volume * density;
}

float CalculateCapsuleMass(float radius, float height, float density) {
    // Объём капсулы = цилиндр + 2 полусферы = цилиндр + сфера
    float cylinderVol = glm::pi<float>() * radius * radius * height;
    float sphereVol = (4.0f / 3.0f) * glm::pi<float>() * radius * radius * radius;
    return (cylinderVol + sphereVol) * density;
}

glm::vec3 CalculateBoxInertia(const glm::vec3& size, float mass) {
    glm::vec3 inertia;
    inertia.x = (mass / 12.0f) * (size.y * size.y + size.z * size.z);
    inertia.y = (mass / 12.0f) * (size.x * size.x + size.z * size.z);
    inertia.z = (mass / 12.0f) * (size.x * size.x + size.y * size.y);
    return inertia;
}

glm::vec3 CalculateSphereInertia(float radius, float mass) {
    float inertia = (2.0f / 5.0f) * mass * radius * radius;
    return glm::vec3(inertia, inertia, inertia);
}

} // namespace PhysicsUtils

} // namespace eoa
