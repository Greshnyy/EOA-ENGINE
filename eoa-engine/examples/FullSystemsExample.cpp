// ============================================
// EOA ENGINE - Расширенный пример использования всех систем
// ============================================

#include "EoaEngine.h"
#include "Physics/Physics.h"
#include "AI/AI.h"
#include "Audio/Audio.h"
#include "UI/UI.h"
#include "Scripting/Scripting.h"
#include <iostream>

using namespace eoa;

// ============================================
// ПРИМЕР: Игрок с физикой, аудио и UI
// ============================================

class PlayerActor : public Actor {
public:
    void Initialize() override {
        SetName("Player");
        
        // Добавляем компонент трансформации (автоматически)
        
        // Физика
        auto* rigidbody = AddComponent<RigidbodyComponent>();
        rigidbody->SetMass(70.0f);
        rigidbody->SetUseGravity(true);
        rigidbody->SetCollisionLayer(CollisionLayer::Character);
        
        // Коллайдер
        auto* collider = AddComponent<ColliderComponent>();
        collider->SetColliderType(ColliderType::Capsule);
        collider->SetCapsuleRadius(0.5f);
        collider->SetCapsuleHeight(1.8f);
        collider->SetIsTrigger(false);
        
        // Аудио слушатель (для 3D звука)
        auto* audioListener = AddComponent<AudioListenerComponent>();
        
        // Меш для визуализации
        auto* mesh = AddComponent<MeshComponent>();
        
        EOA_LOG_INFO("PlayerActor spawned with physics and audio");
    }
    
    void Tick(float deltaTime) override {
        Actor::Tick(deltaTime);
        
        // Управление через Input
        auto* input = Input::GetInstance();
        auto* transform = GetTransform();
        
        glm::vec3 moveDirection(0.0f);
        
        if (input->IsKeyDown(KeyCode::W)) {
            moveDirection.z += 1.0f;
        }
        if (input->IsKeyDown(KeyCode::S)) {
            moveDirection.z -= 1.0f;
        }
        if (input->IsKeyDown(KeyCode::A)) {
            moveDirection.x -= 1.0f;
        }
        if (input->IsKeyDown(KeyCode::D)) {
            moveDirection.x += 1.0f;
        }
        
        // Применение движения через физику
        auto* rb = GetComponent<RigidbodyComponent>();
        if (rb && glm::length(moveDirection) > 0.0f) {
            moveDirection = glm::normalize(moveDirection);
            rb->AddForce(moveDirection * 500.0f * deltaTime);
        }
        
        // Прыжок
        if (input->IsKeyJustPressed(KeyCode::Space)) {
            auto* characterCtrl = GetComponent<CharacterControllerComponent>();
            if (characterCtrl && characterCtrl->CanJump()) {
                characterCtrl->Jump();
            }
        }
    }
};

// ============================================
// ПРИМЕР: AI враг с Behavior Tree
// ============================================

class EnemyAIActor : public Actor {
public:
    void Initialize() override {
        SetName("EnemyAI");
        
        // Физика
        auto* rigidbody = AddComponent<RigidbodyComponent>();
        rigidbody->SetMass(80.0f);
        rigidbody->SetUseGravity(true);
        rigidbody->SetCollisionLayer(CollisionLayer::Dynamic);
        
        // Коллайдер
        auto* collider = AddComponent<ColliderComponent>();
        collider->SetColliderType(ColliderType::Box);
        collider->SetBoxSize(glm::vec3(0.5f, 1.0f, 0.5f));
        
        // AI Perception
        auto* perception = AddComponent<AIPerceptionComponent>();
        PerceptionConfig config;
        config.sightRange = 30.0f;
        config.hearingRange = 20.0f;
        config.fieldOfView = 90.0f;
        perception->SetConfig(config);
        
        // AI Controller
        auto* aiController = AddComponent<AIControllerComponent>();
        
        // Создание Behavior Tree программно
        auto behaviorTree = std::make_unique<BehaviorTree>("EnemyBT");
        
        // Корневой Selector: Patrol или Chase
        auto rootSelector = std::make_unique<BTSelector>("Root");
        
        // Ветка Chase (преследование)
        auto chaseSequence = std::make_unique<BTSequence>("Chase");
        
        // Условие: видим ли игрока?
        auto canSeeCondition = std::make_unique<BTCondition>(
            "CanSeePlayer",
            [this, perception]() -> bool {
                Actor* target = nullptr; // В реальности поиск цели
                float visibility = 0.0f;
                return perception->CanSee(target, visibility);
            }
        );
        chaseSequence->AddChild(std::move(canSeeCondition));
        
        // Действие: Движение к игроку
        auto moveToPlayer = std::make_unique<BTAction>(
            "MoveToPlayer",
            [this, aiController](float deltaTime) -> BTNodeState {
                if (aiController) {
                    // aiController->MoveTo(targetPosition);
                }
                return BTNodeState::Running;
            }
        );
        chaseSequence->AddChild(std::move(moveToPlayer));
        
        rootSelector->AddChild(std::move(chaseSequence));
        
        // Ветка Patrol (патрулирование)
        auto patrolSequence = std::make_unique<BTSequence>("Patrol");
        
        auto patrolAction = std::make_unique<BTAction>(
            "Patrol",
            [this](float deltaTime) -> BTNodeState {
                // Логика патрулирования
                return BTNodeState::Running;
            }
        );
        patrolSequence->AddChild(std::move(patrolAction));
        
        rootSelector->AddChild(std::move(patrolSequence));
        
        behaviorTree->SetRoot(std::move(rootSelector));
        aiController->SetBehaviorTree(std::move(behaviorTree));
        
        EOA_LOG_INFO("EnemyAIActor spawned with AI system");
    }
};

// ============================================
// ПРИМЕР: UI интерфейс
// ============================================

void CreateGameUI() {
    auto* uiActor = new Actor("UIRoot");
    auto* uiComp = uiActor->AddComponent<UIComponent>();
    UICanvas* canvas = uiComp->GetCanvas();
    
    // Настройка размера экрана
    canvas->SetScreenSize(1920.0f, 1080.0f);
    
    // Главная панель
    auto mainPanel = std::make_unique<UIPanel>("MainPanel");
    mainPanel->SetPosition(0, 0);
    mainPanel->SetSize(1920, 1080);
    mainPanel->SetAnchor(UIAnchor::MiddleCenter);
    
    // Кнопка старта
    auto startButton = std::make_unique<UIButton>("StartButton");
    startButton->SetPosition(0, 100);
    startButton->SetSize(200, 60);
    startButton->SetAnchor(UIAnchor::MiddleCenter);
    startButton->SetText("Start Game");
    startButton->SetTextColor(Color32::White);
    startButton->SetBackgroundColor(Color32(0, 128, 0, 255));
    startButton->SetOnClick([]() {
        EOA_LOG_INFO("Start button clicked!");
    });
    
    // Слайдер громкости
    auto volumeSlider = std::make_unique<UISlider>("VolumeSlider");
    volumeSlider->SetPosition(-100, -50);
    volumeSlider->SetSize(200, 20);
    volumeSlider->SetAnchor(UIAnchor::MiddleCenter);
    volumeSlider->SetValue(0.7f);
    volumeSlider->SetOnValueChanged([](float value) {
        AudioManager::Get().SetMasterVolume(value);
        EOA_LOG_INFO("Volume changed to: {}", value);
    });
    
    // Текстовая метка
    auto scoreLabel = std::make_unique<UILabel>("ScoreLabel");
    scoreLabel->SetPosition(-200, 200);
    scoreLabel->SetSize(400, 50);
    scoreLabel->SetAnchor(UIAnchor::MiddleCenter);
    scoreLabel->SetText("Score: 0");
    scoreLabel->SetFontSize(24);
    scoreLabel->SetTextColor(Color32::White);
    
    // Добавление элементов в панель
    mainPanel->AddChild(std::move(startButton));
    mainPanel->AddChild(std::move(volumeSlider));
    mainPanel->AddChild(std::move(scoreLabel));
    
    // Добавление панели в canvas
    canvas->AddRoot(std::move(mainPanel));
    
    EOA_LOG_INFO("Game UI created");
}

// ============================================
// ПРИМЕР: Аудио система
// ============================================

void SetupAudio() {
    auto& audioManager = AudioManager::Get();
    
    // Инициализация
    AudioConfig config;
    config.sampleRate = 44100;
    config.maxSoundChannels = 64;
    config.masterVolume = 0.8f;
    config.enable3DAudio = true;
    
    audioManager.Initialize(config);
    
    // Загрузка звукового банка
    std::vector<std::string> sfxFiles = {
        "sounds/footstep.wav",
        "sounds/jump.wav",
        "sounds/shoot.wav"
    };
    audioManager.LoadSoundBank("PlayerSFX", sfxFiles);
    
    // Создание фонового звука
    AudioSourceData ambientData;
    ambientData.name = "AmbientForest";
    ambientData.filename = "sounds/forest_ambient.wav";
    ambientData.type = SoundType::Ambient;
    ambientData.loopMode = SoundLoopMode::Loop;
    ambientData.volume = 0.5f;
    ambientData.autoPlay = true;
    
    audioManager.CreateSource(ambientData);
    
    EOA_LOG_INFO("Audio system initialized");
}

// ============================================
// ПРИМЕР: Scripting (Lua)
// ============================================

void SetupScripting() {
    auto& scriptManager = ScriptManager::Get();
    
    // Инициализация Lua
    scriptManager.Initialize(true);
    
    // Регистрация класса Actor в Lua
    auto actorClass = std::make_unique<ScriptClass>("Actor");
    actorClass->AddMethod("GetName", [](lua_State* L) -> int {
        // Actor* self = reinterpret_cast<Actor*>(lua_touserdata(L, 1));
        // lua_pushstring(L, self->GetName().c_str());
        // return 1;
        return 0;
    });
    scriptManager.RegisterClass(std::move(actorClass));
    
    // Загрузка скрипта
    scriptManager.LoadScript("scripts/game_logic.lua");
    
    // Выполнение кода
    scriptManager.ExecuteCode("print('Hello from Lua!')");
    
    EOA_LOG_INFO("Scripting system initialized");
}

// ============================================
// ПРИМЕР: NavMesh для AI
// ============================================

void SetupNavMesh(World* world) {
    auto* navmeshActor = world->CreateActor("NavMesh");
    auto* navmeshComp = navmeshActor->AddComponent<NavMeshComponent>();
    
    // Генерация навмеша из геометрии уровня
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    
    // В реальности: загрузка геометрии уровня
    // Здесь: заглушка
    vertices.push_back(glm::vec3(-50, 0, -50));
    vertices.push_back(glm::vec3(50, 0, -50));
    vertices.push_back(glm::vec3(50, 0, 50));
    vertices.push_back(glm::vec3(-50, 0, 50));
    
    indices = {0, 1, 2, 0, 2, 3};
    
    navmeshComp->BuildFromGeometry(vertices, indices, 0.3f, 0.2f);
    
    // Регистрация в глобальной системе
    NavMeshSystem::Get().RegisterNavMesh(navmeshComp);
    
    EOA_LOG_INFO("NavMesh setup complete");
}

// ============================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================

int main(int argc, char** argv) {
    Logger::GetInstance()->SetLogLevel(LogLevel::Info);
    Logger::GetInstance()->OpenLogFile("full_game_example.log");
    
    EOA_LOG_INFO("=== EOA Engine Full Example ===");
    
    // Конфигурация
    EngineConfig config;
    config.Title = "EOA Engine - Full Systems Demo";
    config.Width = 1920;
    config.Height = 1080;
    config.VSync = true;
    config.ContentPath = "assets/";
    
    // Запуск приложения
    Application::Init(config);
    
    World* world = Application::GetInstance()->GetWorld();
    
    // Настройка всех систем
    SetupAudio();
    SetupScripting();
    SetupNavMesh(world);
    CreateGameUI();
    
    // Спавн игрока
    auto* player = world->CreateActor<PlayerActor>();
    player->GetTransform()->SetPosition(glm::vec3(0, 0, 5));
    
    // Спавн AI врагов
    for (int i = 0; i < 3; ++i) {
        auto* enemy = world->CreateActor<EnemyAIActor>();
        enemy->GetTransform()->SetPosition(glm::vec3((i - 1) * 5.0f, 0, -10));
    }
    
    // Игровой цикл
    while (!Application::GetInstance()->ShouldQuit()) {
        Application::GetInstance()->Tick();
        
        // Дополнительная логика игры
        // ...
    }
    
    // Очистка
    Application::Shutdown();
    
    Logger::GetInstance()->CloseLogFile();
    EOA_LOG_INFO("Game ended successfully");
    
    return 0;
}
