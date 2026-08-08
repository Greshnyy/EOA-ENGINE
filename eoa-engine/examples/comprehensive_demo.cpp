// ============================================================================
// EOA Engine - Comprehensive Demo
// Демонстрация всех систем движка
// ============================================================================

#include "EoaEngine.h"
#include <iostream>

using namespace eoa;

int main() {
    // =========================================================================
    // 1. Инициализация движка
    // =========================================================================
    
    EngineConfig config;
    config.windowTitle = "EOA Engine - Full Feature Demo";
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    
    auto& engine = Engine::Get();
    if (!engine.Initialize(config)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }
    
    auto* world = engine.GetWorld();
    
    // =========================================================================
    // 2. Создание игрока с физикой, аудио и управлением
    // =========================================================================
    
    auto* player = world->CreateActor("Player");
    
    // Transform
    auto* transform = player->GetComponent<TransformComponent>();
    transform->SetPosition(glm::vec3(0, 1, 5));
    
    // Mesh
    auto* meshComp = player->AddComponent<MeshComponent>();
    meshComp->LoadMesh("models/player.glb");
    
    // Physics - Rigidbody
    auto* rigidbody = player->AddComponent<RigidbodyComponent>();
    rigidbody->SetMass(70.0f);
    rigidbody->SetUseGravity(true);
    rigidbody->SetLinearDamping(0.1f);
    
    // Physics - Collider
    auto* collider = player->AddComponent<ColliderComponent>();
    collider->SetShape(ColliderShape::Capsule);
    collider->SetSize(glm::vec3(0.5f, 1.0f, 0.5f));
    
    // Character Controller
    auto* characterCtrl = player->AddComponent<CharacterControllerComponent>();
    characterCtrl->SetMoveSpeed(5.0f);
    characterCtrl->SetJumpForce(7.0f);
    
    // Audio Listener (игрок слушает звуки)
    auto* audioListener = player->AddComponent<AudioListenerComponent>();
    
    // Аудио шаги
    auto* footstepAudio = player->AddComponent<AudioComponent>("FootstepAudio");
    footstepAudio->LoadSound("sounds/footstep.wav");
    footstepAudio->SetSpatial(true);
    footstepAudio->SetVolume(0.5f);
    
    // Camera на игроке
    auto* camera = player->AddComponent<CameraComponent>();
    camera->SetFOV(75.0f);
    camera->SetNearPlane(0.1f);
    camera->SetFarPlane(1000.0f);
    
    // =========================================================================
    // 3. AI враг с NavMesh, Perception и Behavior Tree
    // =========================================================================
    
    auto* enemy = world->CreateActor("Enemy");
    enemy->GetComponent<TransformComponent>()->SetPosition(glm::vec3(10, 1, 5));
    
    // Mesh врага
    auto* enemyMesh = enemy->AddComponent<MeshComponent>();
    enemyMesh->LoadMesh("models/enemy.glb");
    
    // Animator для врага
    auto* animator = enemy->AddComponent<AnimatorComponent>();
    
    // Загрузка анимаций
    AnimationClip idleClip;
    idleClip.name = "Idle";
    idleClip.duration = 2.0f;
    animator->AddClip(idleClip);
    
    AnimationClip walkClip;
    walkClip.name = "Walk";
    walkClip.duration = 1.5f;
    animator->AddClip(walkClip);
    
    AnimationClip attackClip;
    attackClip.name = "Attack";
    attackClip.duration = 1.0f;
    animator->AddClip(attackClip);
    
    // State machine
    AnimationState idleState;
    idleState.name = "Idle";
    idleState.clipName = "Idle";
    idleState.looping = true;
    
    AnimationState walkState;
    walkState.name = "Walk";
    walkState.clipName = "Walk";
    walkState.looping = true;
    
    AnimationState attackState;
    attackState.name = "Attack";
    attackState.clipName = "Attack";
    attackState.looping = false;
    
    // Transitions
    walkState.transitions.push_back({
        "Idle",
        [enemy]() {
            // Условие: игрок далеко
            return glm::distance(
                enemy->GetTransform()->GetPosition(),
                player->GetTransform()->GetPosition()
            ) > 15.0f;
        },
        0.3f
    });
    
    attackState.transitions.push_back({
        "Walk",
        [enemy]() {
            // Условие: игрок вышел из радиуса атаки
            return glm::distance(
                enemy->GetTransform()->GetPosition(),
                player->GetTransform()->GetPosition()
            ) > 3.0f;
        },
        0.2f
    });
    
    animator->AddState(idleState);
    animator->AddState(walkState);
    animator->AddState(attackState);
    animator->SetCurrentState("Idle");
    
    // AI Controller
    auto* aiController = enemy->AddComponent<AIControllerComponent>();
    
    // Perception
    auto* perception = enemy->AddComponent<AIPerceptionComponent>();
    PerceptionConfig percConfig;
    percConfig.sightRange = 50.0f;
    percConfig.fieldOfView = 90.0f;
    percConfig.hearingRange = 30.0f;
    perception->SetConfig(percConfig);
    
    // Behavior Tree
    auto* behaviorTree = new BehaviorTree("EnemyBT");
    
    // Selector: Attack or Chase or Idle
    auto* rootSelector = new BTSelector("Root");
    
    // Attack sequence
    auto* attackSeq = new BTSequence("AttackSequence");
    attackSeq->AddChild(std::make_unique<BTCondition>("CanAttack", 
        [enemy, player]() {
            float dist = glm::distance(
                enemy->GetTransform()->GetPosition(),
                player->GetTransform()->GetPosition()
            );
            return dist < 3.0f;
        }));
    attackSeq->AddChild(std::make_unique<BTAction>("PerformAttack",
        [enemy](float dt) {
            // Выполнить атаку
            return BTNodeState::Success;
        }));
    
    // Chase sequence
    auto* chaseSeq = new BTSequence("ChaseSequence");
    chaseSeq->AddChild(std::make_unique<BTCondition>("CanSeePlayer",
        [perception, player]() {
            float visibility = 0;
            return perception->CanSee(player, visibility);
        }));
    chaseSeq->AddChild(std::make_unique<BTAction>("MoveToPlayer",
        [aiController, player](float dt) {
            aiController->MoveTo(player->GetTransform()->GetPosition(), 4.0f);
            return BTNodeState::Running;
        }));
    
    // Idle
    auto* idleAction = std::make_unique<BTAction>("Idle",
        [](float dt) { return BTNodeState::Success; });
    
    rootSelector->AddChild(std::unique_ptr<BTNode>(attackSeq));
    rootSelector->AddChild(std::unique_ptr<BTNode>(chaseSeq));
    rootSelector->AddChild(std::move(idleAction));
    
    behaviorTree->SetRoot(std::unique_ptr<BTNode>(rootSelector));
    aiController->SetBehaviorTree(std::unique_ptr<BehaviorTree>(behaviorTree));
    
    // NavMesh
    auto* navMesh = enemy->AddComponent<NavMeshComponent>();
    // NavMesh будет сгенерирован из геометрии уровня
    
    // =========================================================================
    // 4. UI интерфейс
    // =========================================================================
    
    auto* uiActor = world->CreateActor("UIRoot");
    auto* uiComponent = uiActor->AddComponent<UIComponent>();
    auto* canvas = uiComponent->GetCanvas();
    
    canvas->SetScreenSize(1920, 1080);
    
    // Health bar panel
    auto healthPanel = std::make_unique<UIPanel>("HealthPanel");
    healthPanel->SetPosition(50, 50);
    healthPanel->SetSize(300, 30);
    healthPanel->SetBackgroundColor(Color32(50, 50, 50, 200));
    
    // Health fill
    auto healthFill = std::make_unique<UIPanel>("HealthFill");
    healthFill->SetPosition(5, 5);
    healthFill->SetSize(290, 20);
    healthFill->SetBackgroundColor(Color32(200, 50, 50, 255));
    healthPanel->AddChild(std::move(healthFill));
    
    // Health text
    auto healthText = std::make_unique<UILabel>("HealthText");
    healthText->SetText("HP: 100/100");
    healthText->SetPosition(100, 5);
    healthText->SetTextColor(Color32::White);
    healthPanel->AddChild(std::move(healthText));
    
    canvas->AddRoot(std::move(healthPanel));
    
    // Minimap button
    auto minimapBtn = std::make_unique<UIButton>("MinimapButton");
    minimapBtn->SetText("Toggle Map");
    minimapBtn->SetPosition(1700, 50);
    minimapBtn->SetSize(150, 40);
    minimapBtn->SetOnClick([]() {
        std::cout << "Minimap toggled!\n";
    });
    canvas->AddRoot(std::move(minimapBtn));
    
    // Slider для громкости
    auto volumeSlider = std::make_unique<UISlider>("VolumeSlider");
    volumeSlider->SetPosition(1700, 100);
    volumeSlider->SetSize(150, 20);
    volumeSlider->SetValue(0.7f);
    volumeSlider->SetOnValueChanged([](float value) {
        AudioManager::Get().SetMasterVolume(value);
    });
    canvas->AddRoot(std::move(volumeSlider));
    
    // =========================================================================
    // 5. Scripting - Lua скрипт для логики игры
    // =========================================================================
    
    auto* scriptActor = world->CreateActor("GameLogic");
    auto* scriptComp = scriptActor->AddComponent<ScriptComponent>();
    
    // Загрузка скрипта
    scriptComp->LoadScript("scripts/game_logic.lua");
    
    // Установка callback
    scriptComp->SetOnInit([]() {
        LOG_INFO("Game logic initialized from Lua!");
    });
    
    scriptComp->SetOnTick([](float dt) {
        // Обновление игровой логики каждый кадр
    });
    
    // =========================================================================
    // 6. Networking - настройка мультиплеера
    // =========================================================================
    
    auto& networkMgr = NetworkManager::Get();
    
    NetworkConfig netConfig;
    netConfig.serverPort = 7777;
    netConfig.maxClients = 16;
    netConfig.updateRate = 0.02f; // 50 tick rate
    
    networkMgr.Initialize(netConfig);
    
    // Host game (сервер + клиент)
    if (networkMgr.HostGame()) {
        LOG_INFO("Hosting multiplayer game!");
        
        // Регистрация RPC
        networkMgr.RegisterGlobalRPC("OnPlayerJoin", RPCMode::All, 
            [](const std::vector<uint8_t>& params) {
                LOG_INFO("Player joined the game!");
            });
        
        networkMgr.SetOnClientConnected([](int clientID) {
            LOG_INFO("Client {} connected", clientID);
        });
    }
    
    // Репликация позиции игрока
    auto* networkPlayer = player->AddComponent<NetworkActorComponent>();
    networkPlayer->SetLocalRole(NetworkRole::AutonomousProxy);
    
    ReplicatedProperty<glm::vec3> positionProp(player->GetTransform()->GetPosition());
    networkPlayer->AddReplicatedProperty("Position", &positionProp);
    
    // =========================================================================
    // 7. Освещение и окружение
    // =========================================================================
    
    // Directional light (солнце)
    auto* sun = world->CreateActor("Sun");
    sun->GetComponent<TransformComponent>()->SetRotation(glm::vec3(-45, 30, 0));
    
    auto* dirLight = sun->AddComponent<LightComponent>();
    dirLight->SetType(LightType::Directional);
    dirLight->SetColor(glm::vec3(1.0f, 0.95f, 0.8f));
    dirLight->SetIntensity(1.5f);
    
    // Point lights
    auto* pointLight1 = world->CreateActor("PointLight1");
    pointLight1->GetComponent<TransformComponent>()->SetPosition(glm::vec3(5, 3, 5));
    
    auto* pl1 = pointLight1->AddComponent<LightComponent>();
    pl1->SetType(LightType::Point);
    pl1->SetColor(glm::vec3(1, 0.5, 0));
    pl1->SetRadius(10.0f);
    pl1->SetIntensity(2.0f);
    
    // =========================================================================
    // 8. Частицы (Particle System уже есть в движке)
    // =========================================================================
    
    auto* fireEmitter = world->CreateActor("FireEffect");
    fireEmitter->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0, 0, 0));
    
    // ParticleSystemComponent уже существует в renderer/particle_system.h
    
    // =========================================================================
    // 9. Запуск игрового цикла
    // =========================================================================
    
    LOG_INFO("Starting game loop...");
    
    engine.Run([world, player, enemy, &networkMgr](float deltaTime) {
        // Input handling
        auto& input = Input::Get();
        
        // Player movement
        glm::vec3 moveDir(0);
        if (input.IsKeyDown(KeyCode::W)) moveDir.z -= 1;
        if (input.IsKeyDown(KeyCode::S)) moveDir.z += 1;
        if (input.IsKeyDown(KeyCode::A)) moveDir.x -= 1;
        if (input.IsKeyDown(KeyCode::D)) moveDir.x += 1;
        
        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            
            auto* charCtrl = player->GetComponent<CharacterControllerComponent>();
            if (charCtrl) {
                charCtrl->Move(moveDir);
            }
        }
        
        // Jump
        if (input.IsKeyPressed(KeyCode::Space)) {
            auto* charCtrl = player->GetComponent<CharacterControllerComponent>();
            if (charCtrl && charCtrl->IsGrounded()) {
                charCtrl->Jump();
            }
        }
        
        // Network update
        networkMgr.Update(deltaTime);
        
        // Пример отправки позиции по сети
        if (networkMgr.IsServer()) {
            glm::vec3 pos = player->GetTransform()->GetPosition();
            std::vector<uint8_t> data;
            NetworkSerializer::WriteVector3(data, pos);
            networkMgr.SendToAll(data, ChannelType::UnreliableUnordered);
        }
        
        return true; // Продолжать цикл
    });
    
    // =========================================================================
    // 10. Очистка
    // =========================================================================
    
    networkMgr.Shutdown();
    engine.Shutdown();
    
    return 0;
}
