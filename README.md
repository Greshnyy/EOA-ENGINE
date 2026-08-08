# 🎮 EOA Engine

**EOA Engine** — это современный игровой движок общего назначения на C++ с использованием Vulkan API, разработанный для создания высокопроизводительных 2D/3D игр. Движок предоставляет полный набор систем: рендеринг, физику, AI, аудио, UI, скриптинг, анимацию и сетевое взаимодействие.

## ✨ Особенности

### 🎨 Рендеринг (Vulkan)
- **PBR Deferred Rendering** — физически корректный рендеринг с GBuffer
- **Динамическое освещение** — point lights, spot lights, directional lights
- **Тени** — shadow mapping с Cascaded Shadow Maps (CSM)
- **Post-processing** — Bloom, SSAO, Depth of Field, Color Grading, Tonemapping
- **Загрузка glTF** — поддержка PBR материалов, текстур, скелетов
- **Instancing** — оптимизированная отрисовка множества объектов
- **Particle System** — GPU частицы с эмиттерами и силами

### 🏗️ Архитектура
- **Actor-Component система** — гибкая композиция объектов (как в Unreal Engine)
- **Система событий** — decoupled communication между системами
- **Менеджмент миров/уровней** — загрузка и выгрузка уровней
- **ImGui Editor** — встроенный редактор для отладки и прототипирования

### 🧱 Базовые компоненты
- `TransformComponent` — позиция, вращение, масштаб
- `MeshComponent` — статические и скелетные меши
- `CameraComponent` — перспективные и ортографические камеры
- `LightComponent` — источники света разных типов
- `ParticleSystemComponent` — системы частиц

---

## 🔧 Расширенные системы

### ⚙️ Физическая система
Полноценная физика с поддержкой динамических тел, коллизий и транспортных средств.

**Компоненты:**
- `RigidbodyComponent` — динамические тела с массой, силами, импульсами
- `ColliderComponent` — коллайдеры (Box, Sphere, Capsule, Mesh)
- `CharacterControllerComponent` — контроллер персонажа с прыжками и движением
- `VehicleComponent` — транспорт с газом, тормозом, рулением
- `PhysicsWorld` — менеджер физического мира с raycast и слоями коллизий

**Пример:**
```cpp
auto* actor = world->CreateActor("Player");
auto* rb = actor->AddComponent<RigidbodyComponent>();
rb->SetMass(70.0f);
rb->SetUseGravity(true);
rb->AddForce(glm::vec3(0, 10, 0)); // Прыжок

auto* collider = actor->AddComponent<ColliderComponent>();
collider->SetType(ColliderType::Capsule);
```

---

### 🤖 AI Система
Продвинутый искусственный интеллект с навигацией, восприятием и деревьями поведения.

**Компоненты:**
- `NavMeshComponent` — навигационная сетка с A* pathfinding
- `AIPerceptionComponent` — восприятие (зрение, слух, поле зрения)
- `BehaviorTree` — дерево поведения (Selector, Sequence, Parallel, Inverter, Repeater)
- `Blackboard` — хранилище данных AI
- `AIControllerComponent` — контроллер AI
- `NavMeshSystem` — глобальная система навигации

**Пример:**
```cpp
auto* enemy = world->CreateActor("Enemy");
auto* aiController = enemy->AddComponent<AIControllerComponent>();
auto* perception = enemy->AddComponent<AIPerceptionComponent>();
perception->GetConfig().sightRange = 50.0f;
perception->GetConfig().hearingRange = 20.0f;

// Behavior Tree
auto bt = std::make_unique<BehaviorTree>();
auto selector = std::make_unique<BTSelector>();
selector->AddChild(std::make_unique<BTAttack>());
selector->AddChild(std::make_unique<BTPatrol>());
bt->SetRoot(std::move(selector));
aiController->SetBehaviorTree(std::move(bt));
```

---

### 🔊 Аудио система
3D аудио с позиционированием, затуханием и пространственным звуком.

**Компоненты:**
- `AudioComponent` — воспроизведение звуков с 3D позиционированием
- `AudioListenerComponent` — слушатель (камера/игрок)
- `AudioManager` — глобальный менеджер с volume control

**Пример:**
```cpp
auto* audio = actor->AddComponent<AudioComponent>();
audio->LoadSound("sounds/footstep.wav");
audio->SetSpatial(true);
audio->SetMinDistance(1.0f);
audio->SetMaxDistance(50.0f);
audio->Play();

// Громкость
AudioManager::GetInstance().SetMasterVolume(0.8f);
AudioManager::GetInstance().SetMusicVolume(0.5f);
```

---

### 🖼️ UI система
Canvas-based UI система с виджетами и событиями.

**Виджеты:**
- `UICanvas` — корневой холст
- `UIPanel` — контейнер
- `UIButton` — кнопка с событиями клика
- `UILabel` — текст
- `UISlider` — ползунок
- `UIImage` — изображение

**Пример:**
```cpp
auto* uiComp = actor->AddComponent<UIComponent>();
auto canvas = uiComp->GetCanvas();

auto button = std::make_unique<UIButton>("StartButton");
button->SetText("Start Game");
button->SetOnClick([]() {
    std::cout << "Game Started!" << std::endl;
});
canvas->AddRoot(std::move(button));

auto healthBar = std::make_unique<UISlider>("HealthBar");
healthBar->SetValue(100.0f);
healthBar->SetRange(0.0f, 100.0f);
canvas->AddRoot(std::move(healthBar));
```

---

### 📜 Scripting (Lua)
Интеграция Lua для скриптов игровой логики с hot reload.

**Компоненты:**
- `ScriptComponent` — компонент скрипта для Actor
- `ScriptManager` — глобальный менеджер скриптов
- `ScriptClass` — регистрация C++ классов в Lua

**Пример:**
```lua
-- player.lua
function OnStart()
    print("Player spawned!")
    self.health = 100
end

function OnUpdate(dt)
    if Input:IsKeyPressed("W") then
        self.actor:MoveForward(10.0 * dt)
    end
end

function OnDamage(amount)
    self.health = self.health - amount
    if self.health <= 0 then
        self.actor:Destroy()
    end
end
```

```cpp
// C++ регистрация
auto* scriptComp = actor->AddComponent<ScriptComponent>();
scriptComp->LoadScript("scripts/player.lua");

// Регистрация C++ класса в Lua
ScriptManager::RegisterClass<TransformComponent>("Transform")
    .Method("GetPosition", &TransformComponent::GetPosition)
    .Method("SetPosition", &TransformComponent::SetPosition);
```

---

### 🎭 Анимация
Скелетная анимация с blend trees и state machines.

**Компоненты:**
- `AnimatorComponent` — скелетная анимация
- `Skeleton` — система костей
- `AnimationClip` — клипы с keyframe interpolation
- `BlendTree` — деревья блендинга (1D, 2D)
- `AnimationStateMachine` — машина состояний с transitions

**Пример:**
```cpp
auto* animator = actor->AddComponent<AnimatorComponent>();
animator->LoadSkeleton("models/character.skeleton");
animator->LoadClip("Idle", "animations/idle.anim");
animator->LoadClip("Walk", "animations/walk.anim");
animator->LoadClip("Run", "animations/run.anim");

// Blend Tree
BlendTree blendTree;
blendTree.AddClip("Idle", 0.0f);
blendTree.AddClip("Walk", 2.0f);
blendTree.AddClip("Run", 6.0f);
blendTree.SetParameter("Speed", 4.5f);
animator->SetBlendTree("Movement", blendTree);

// State Machine
auto* stateMachine = animator->GetStateMachine();
stateMachine->AddState("Idle");
stateMachine->AddState("Walk");
stateMachine->AddTransition("Idle", "Walk", [](float speed) { return speed > 0.5f; });
```

---

### 🌐 Networking
Клиент-сервер архитектура с репликацией свойств и RPC.

**Компоненты:**
- `NetworkManager` — клиент-сервер менеджмент
- `NetworkActorComponent` — репликация свойств
- `ReplicatedProperty<T>` — template свойство с dirty detection
- RPC система (Server, Client, All, Owner, Others)

**Пример:**
```cpp
// Сервер
class PlayerServer : public NetworkActor {
public:
    ReplicatedProperty<glm::vec3> positionProp{glm::vec3(0)};
    ReplicatedProperty<float> healthProp{100.0f};

    void Setup() {
        AddReplicatedProperty("Position", &positionProp);
        AddReplicatedProperty("Health", &healthProp);
    }

    EOARPC(Server, Reliable, Ordered)
    void OnPlayerJoin(int playerId) {
        std::cout << "Player " << playerId << " joined!" << std::endl;
    }

    EOARPC(Server, Unreliable, Unordered)
    void OnPlayerMove(glm::vec3 pos) {
        positionProp = pos; // Автоматическая репликация клиентам
    }
};

// Клиент
class PlayerClient : public NetworkActor {
public:
    EOARPC(Client, Reliable, Ordered)
    void OnReceiveDamage(float damage) {
        health -= damage;
        UpdateHealthUI();
    }
};

// Запуск сервера
NetworkManager::InitServer(7777);

// Подключение клиента
NetworkManager::Connect("127.0.0.1", 7777);
```

---

## 📁 Структура проекта

```
EOAEngine/
├── Core/               # Ядро движка (Window, Time, Events, Memory)
├── Renderer/           # Vulkan рендерер (Pipeline, GBuffer, Lights)
├── Scene/              # Actor-Component система, World, Level
├── Components/         # Базовые компоненты (Transform, Mesh, Camera, Light)
├── Physics/            # Физическая система (Rigidbody, Collider, Character)
├── AI/                 # AI система (NavMesh, BehaviorTree, Perception)
├── Audio/              # Аудио система (3D звук, Manager)
├── UI/                 # UI система (Canvas, Widgets)
├── Scripting/          # Lua скриптинг
├── Animation/          # Скелетная анимация (Skeleton, BlendTree, StateMachine)
├── Networking/         # Сетевая система (RPC, Replication)
├── Assets/             # Загрузчики (glTF, Textures, Animations)
├── Editor/             # ImGui редактор
├── src/                # Исходный код реализации
├── examples/           # Примеры использования
└── tests/              # Юнит тесты
```

---

## 🚀 Быстрый старт

### Требования
- **ОС:** Windows 10/11, Linux
- **Компилятор:** MSVC 2019+, GCC 9+, Clang 10+
- **Vulkan SDK:** 1.3+
- **CMake:** 3.20+
- **Lua:** 5.4+ (включён в проект)

### Сборка

```bash
# Клонирование репозитория
git clone https://github.com/yourusername/eoa-engine.git
cd eoa-engine

# Создание build директории
mkdir build && cd build

# Конфигурация
cmake .. -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build . --config Release

# Запуск примера
./examples/comprehensive_demo
```

### Интеграция в проект

```cpp
#include "EOA/Engine.h"

int main() {
    // Инициализация движка
    EOA::EngineConfig config;
    config.windowTitle = "My Game";
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    config.vsync = true;
    
    EOA::Engine::Init(config);
    
    // Создание мира
    auto* world = EOA::WorldManager::CreateWorld("MainLevel");
    
    // Добавление игрока
    auto* player = world->CreateActor("Player");
    player->AddComponent<EOA::TransformComponent>();
    player->AddComponent<EOA::MeshComponent>("models/player.glb");
    player->AddComponent<EOA::CameraComponent>();
    player->AddComponent<EOA::RigidbodyComponent>();
    player->AddComponent<EOA::AudioComponent>("sounds/player.wav");
    
    // Добавление AI врага
    auto* enemy = world->CreateActor("Enemy");
    enemy->AddComponent<EOA::AIControllerComponent>();
    enemy->AddComponent<EOA::AIPerceptionComponent>();
    
    // Создание UI
    auto* uiActor = world->CreateActor("UI");
    auto* uiComp = uiActor->AddComponent<EOA::UIComponent>();
    auto canvas = uiComp->GetCanvas();
    canvas->AddRoot(std::make_unique<EOA::UIButton>("Start"));
    
    // Игровой цикл
    while (EOA::Engine::IsRunning()) {
        EOA::Engine::Tick();
    }
    
    // Очистка
    EOA::Engine::Shutdown();
    return 0;
}
```

---

## 📚 Документация

- [API Reference](docs/API.md)
- [Руководство по архитектуре](docs/Architecture.md)
- [Гайд по Vulkan рендерингу](docs/VulkanRendering.md)
- [Физика и коллизии](docs/Physics.md)
- [AI и Behavior Trees](docs/AI.md)
- [Lua скриптинг](docs/Scripting.md)
- [Сетевое взаимодействие](docs/Networking.md)
- [Примеры](examples/)

---

## 🛠️ Расширение движка

### Добавление собственного компонента

```cpp
// MyComponent.h
#pragma once
#include "Core/Component.h"

namespace EOA {
    class MyComponent : public Component {
    public:
        void OnInit() override;
        void OnUpdate(float deltaTime) override;
        
        void SetCustomValue(float value) { m_Value = value; }
        float GetCustomValue() const { return m_Value; }
        
    private:
        float m_Value = 0.0f;
    };
}

// MyComponent.cpp
#include "MyComponent.h"

namespace EOA {
    void MyComponent::OnInit() {
        LOG_INFO("MyComponent initialized");
    }
    
    void MyComponent::OnUpdate(float deltaTime) {
        m_Value += deltaTime;
    }
}
```

### Регистрация в системе

```cpp
// В Engine.cpp или модуле инициализации
ComponentRegistry::Register<MyComponent>("MyComponent");
```

---

## 🧪 Тестирование

```bash
# Запуск всех тестов
cd build
ctest --output-on-failure

# Запуск конкретных тестов
ctest -R PhysicsTests
ctest -R AITests
```

---

## 📄 Лицензия

EOA Engine распространяется под лицензией **MIT**. См. файл [LICENSE](LICENSE) для деталей.

---

## 🤝 Вклад в проект

Мы приветствуем вклад в развитие движка! Пожалуйста, ознакомьтесь с [CONTRIBUTING.md](CONTRIBUTING.md) перед отправкой pull request.

### Как помочь:
- 🐛 Сообщить об ошибке
- 💡 Предложить новую функцию
- 📝 Улучшить документацию
- 🔧 Исправить баги
- 🎨 Добавить примеры

---

## 📬 Контакты

- **GitHub:** [github.com/yourusername/eoa-engine](https://github.com/yourusername/eoa-engine)
- **Discord:** [Присоединиться к серверу](https://discord.gg/yourserver)
- **Email:** dev@eoaengine.com

---

## 🙏 Благодарности

- **Vulkan** — низкий уровень графики
- **Dear ImGui** — редактор и UI
- **glTF** — формат 3D моделей
- **Lua** — скриптовый язык
- **Сообщество разработчиков** — за вдохновение и поддержку

---

**Создано с ❤️ для разработчиков игр**

*EOA Engine — ваш путь к созданию невероятных игровых миров*
