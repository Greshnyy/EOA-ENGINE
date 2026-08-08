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
=======
# EOA Engine — движок Echoes of Ash

Собственный движок на C++/Vulkan под сокращённый (не-Unreal) скоуп проекта. Контекст решения и
компромиссов — в файлах `EOA_CustomEngine_Plan.md` / `EOA_CustomEngine_LargeMap.md` из чата, не
здесь; этот README — только про сборку и текущее состояние кода.

---

## Сборка на Windows с нуля — подробно

### 1. Поставь компилятор

Нужен MSVC (Visual Studio) **или** MinGW. Проще всего:
1. Скачай **Visual Studio Community** (бесплатно): https://visualstudio.microsoft.com/
2. При установке выбери workload **"Desktop development with C++"** — это поставит MSVC-компилятор,
   Windows SDK и CMake-интеграцию за один клик.

### 2. Поставь Vulkan SDK

1. Скачай с https://vulkan.lunarg.com/ (LunarG Vulkan SDK, последняя версия под Windows).
2. Запусти установщик, оставь всё по умолчанию.
3. После установки перезапусти терминал/IDE — установщик сам добавляет переменную окружения
   `VULKAN_SDK`, но текущая открытая консоль её ещё не увидит.
4. Проверь в новом терминале:
   ```
   echo %VULKAN_SDK%
   ```
   Должен вывести путь вроде `C:\VulkanSDK\1.3.xxx.x`. Если пусто — переустанови SDK или
   перезагрузи компьютер.

### 3. Поставь vcpkg (менеджер C++ зависимостей)

```
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### 4. Поставь зависимости через vcpkg

```
cd C:\vcpkg
.\vcpkg install glfw3:x64-windows glm:x64-windows
```

Это займёт несколько минут — vcpkg сам качает и собирает исходники.

### 5. Собери проект

Распакуй архив с движком, например в `C:\dev\eoa-engine`. Из терминала:

```
cd C:\dev\eoa-engine
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

Если CMake ругается, что не находит компилятор — открой **"Developer Command Prompt for VS"**
(есть в меню Пуск после установки Visual Studio) и запускай команды из него, а не из обычного
терминала: он настраивает переменные окружения под MSVC.
### 6. Запусти

```
cd Debug
eoa_engine.exe
```

(Путь до exe зависит от генератора CMake — Visual Studio генератор кладёт бинарник в подпапку
`Debug/` или `Release/` внутри `build/`, в отличие от Linux, где он прямо в `build/`.)

**Важно:** запускай exe из той же директории, где лежит папка `shaders/` и `assets/` (CMake
копирует их туда автоматически при сборке) — движок ищет их по относительному пути.

### Если что-то не собирается

- **"Could not find glfw3" / "Could not find glm"** — забыл `-DCMAKE_TOOLCHAIN_FILE=...` в команде
  cmake, либо vcpkg install отработал не для той архитектуры (`x64-windows`, не `x86-windows`).
- **"VULKAN_SDK not found"** — переменная окружения не подхватилась, перезапусти терминал/IDE
  после установки SDK.
- **Линковочные ошибки про glslc** — Vulkan SDK ставит `glslc.exe` сам, но если CMake его не
  находит, добавь `%VULKAN_SDK%\Bin` в PATH вручную.

---

## Сборка на Linux (для сверки/CI)

```
sudo apt install cmake g++ libvulkan-dev libglfw3-dev glslc vulkan-validationlayers libglm-dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
./eoa_engine
```

---

## Debug-сборка по умолчанию

`CMAKE_BUILD_TYPE=Debug` включает ASan+UBSan санитайзеры и Vulkan validation layers. Это
единственная реальная страховка от тихих крашей на этапе, когда код не читается вручную — любая
ошибка памяти или неправильный Vulkan-вызов даёт явное сообщение с файлом и строкой вместо
непонятного зависания. Не убирай `Debug`, пока явно не понадобится `Release` для замера
производительности.

---

## Управление

- **WASD** — движение камеры
- **Зажатая правая кнопка мыши + движение мыши** — обзор (курсор захватывается только пока зажата
  кнопка, чтобы не мешать при отладке)
- **Space / Left Ctrl** — вверх/вниз
- Закрыть окно — выход

---

## Что уже работает (по milestones)

**Milestone 1 — bootstrap:** окно (GLFW), Vulkan instance с validation layers, автовыбор GPU,
логическое устройство.

**Milestone 2 — swapchain + render loop:** swapchain, render pass, framebuffers, double buffering,
resize/минимизация окна обработаны без креша.

**Milestone 3 — первый треугольник:** шейдеры (SPIR-V, автокомпиляция при сборке), graphics
pipeline с динамическим viewport/scissor, `vkCmdDraw`.

**Milestone 4 — vertex buffer, depth buffer, камера (данные):** реальные vertex/index буферы
(staging → DEVICE_LOCAL), depth testing, UBO с MVP-матрицами (GLM), тестовая сцена с перекрытием
объектов для проверки depth test.

**Milestone 5 — текстуры + управляемая камера:** загрузка текстур через `stb_image`, placeholder
`checker.png`, свободная FPS-камера (WASD + мышь), descriptor set с combined image sampler.

**Milestone 6-7 — сцена, glTF, редактор, материалы:**

- Абстракция сцены, per-object push constants, glTF-загрузчик (tiny_gltf), ImGui-редактор
  (Hierarchy/Inspector/AssetBrowser/Console/Stats).
- Материалы из glTF с реальными текстурами, descriptor sets разделены на set=0 (камера)
  и set=1 (материал). Asset Browser → двойной клик загружает модель в сцену.

---

## Milestone 8 (Phase 1) — Actor-Component + PBR Deferred Rendering

### Архитектура Actor-Component (аналог AActor/UActorComponent из UE)

| Класс | Файл | Аналог в UE |
|---|---|---|
| `Object` | `core/object.h/.cpp` | `UObject` — RTTI, InstanceID, именование |
| `Actor` | `core/actor.h/.cpp` | `AActor` — `AddComponent<T>()`, `GetComponent<T>()`, BeginPlay/Tick/EndPlay |
| `Component` | `core/component.h/.cpp` | `UActorComponent` — привязка к Actor, active-флаг |
| `TransformComponent` | `core/transform_component.h/.cpp` | `USceneComponent` — позиция/поворот(quat)/масштаб |
| `MeshComponent` | `renderer/mesh_component.h/.cpp` | `UStaticMeshComponent` — Mesh + MaterialData, BindDraw |
| `CameraComponent` | `renderer/camera_component.h/.cpp` | `UCameraComponent` — FPS-камера, перспектива |
| `LightComponent` | `renderer/light_component.h/.cpp` | `ULightComponent` — Directional/Point/Spot |
| `World` | `renderer/world.h` | `UWorld` — `SpawnActor<T>()`, DestroyActor, итерация |

### PBR Deferred Rendering

**Два прохода вместо прямого forward:**

1. **GBuffer Pass** (`gbuffer.vert/.frag`, `renderer/gbuffer.h/.cpp`) — 4 MRT:
   - Attachment 0: Albedo (RGBA8_SRGB)
   - Attachment 1: World-space Normal (RGBA16F, энкодинг `N*0.5+0.5`)
   - Attachment 2: ORM — Roughness R, Metallic G (RGBA8)
   - Depth: D32_SFLOAT

2. **Deferred Lighting Pass** (`deferred_lighting.vert/.frag`, `renderer/deferred_lighting.h/.cpp`):
   - Fullscreen triangle (без vertex buffer'а, через `gl_VertexIndex`)
   - Читает GBuffer текстуры → Cook-Torrance BRDF:
   - Normal Distribution: GGX
   - Geometry: Smith (Schlick-GGX)
   - Fresnel: Schlick approximation
   - Поддержка directional + point lights (spot в шейдере заготовлен)
   - Reinhard tone mapping + gamma correction

**Light buffers:** UBO (camera pos + ambient + light count) + SSBO (до 64 источников,
автосбор из `LightComponent`-ов World'а). Если ни одного LightComponent нет — создаётся
дефолтный directional sun.

**Pipeline** (`pipeline.h/.cpp`) параметризован: push constant size/stage, cull mode,
depth test/write. GBuffer fill использует 96-байтный push (model + baseColor +
roughness/metallic), старый forward — 64-байтный (только model).

### Шейдеры для будущих этапов

- `shadow_depth.vert/.frag` — depth-only pass для shadow mapping (directional light).
  Скомпилированы CMake, не подключены в рендерер.

### Что изменилось по сравнению с Milestone 7

- `Scene`/`GameObject`/`Material` → `World`/`Actor`/`Component` + `MaterialData`
- Forward Lambert → Deferred PBR (GBuffer + DeferredLighting)
- `RecordCommandBuffer()` → `RecordGBufferPass()` + `RecordDeferredPass()`
- `platform/camera.h` → `CameraComponent` (в компонентной модели)
- Старый `scene.h` сохранён, больше не используется

---

## Структура

```
CMakeLists.txt
assets/
  textures/checker.png          — placeholder-текстура (охра/тёмная, под палитру ДД)
  models/test_pyramid.gltf       — тестовый ассет без материала
  models/test_cube_textured.gltf — тестовый ассет с материалом (зелёная текстура)
  models/cube_albedo.png         — текстура для теста материалов
third_party/
  stb_image.h                    — загрузка изображений (nothings/stb, public domain)
  stb_image_write.h              — генерация текстур (использовался для checker.png)
  tiny_gltf.h                    — парсинг .gltf/.glb (syoyo/tinygltf, MIT)
  json.hpp                       — зависимость tiny_gltf (nlohmann/json, MIT)
  imgui/                         — UI редактора (Dear ImGui, MIT, docking-ветка)
shaders/
  triangle.vert/.frag            — старый forward pipeline (сохранён)
  gbuffer.vert/.frag             — GBuffer fill pass (MRT: albedo + normal + ORM + depth)
  deferred_lighting.vert/.frag   — Deferred PBR fullscreen pass
  shadow_depth.vert/.frag        — depth-only pass для shadow mapping (заготовка)
src/
  log.h                          — EOA_LOG/WARN/ERROR/FATAL, EOA_CHECK_VK
  main.cpp                       — точка входа, bootstrap + render loop + Editor
  core/
    object.h/.cpp                — UObject-база: RTTI, InstanceID
    actor.h/.cpp                 — Actor: компоненты, жизненный цикл
    component.h/.cpp             — Component: база, привязка к Actor
    transform_component.h/.cpp   — Transform: позиция/поворот/масштаб, model-матрица
  platform/
    window.h/.cpp                — обёртка GLFW-окна
    camera.h/.cpp                — (устарел) старая камера, заменена CameraComponent
  rhi/
    vk_instance.h/.cpp           — VkInstance + debug messenger
    vk_device.h/.cpp             — выбор GPU + логическое устройство
    vk_swapchain.h/.cpp          — swapchain + image views
    file_utils.h/.cpp            — чтение бинарных файлов (.spv)
  renderer/
    vertex.h                     — Vertex struct + UniformBufferObject
    buffer_utils.h/.cpp          — CreateBuffer/CreateImage/CreateImageView, staging
    mesh.h/.cpp                  — GPU vertex+index буферы
    texture.h/.cpp               — загрузка изображения → VkImage/View/Sampler
    scene.h                      — (устарел) старый Scene/GameObject/Material
    world.h                      — World: контейнер Actor-ов
    gltf_loader.h/.cpp           — парсинг .gltf/.glb
    pipeline.h/.cpp              — параметризованный graphics pipeline
    gbuffer.h/.cpp               — GBuffer: 4 MRT, render pass
    deferred_lighting.h/.cpp     — Deferred PBR: fullscreen pass, light UBO+SSBO
    mesh_component.h/.cpp        — MeshComponent: Mesh + MaterialData, BindDraw
    camera_component.h/.cpp      — CameraComponent: FPS-камера, перспектива
    light_component.h/.cpp       — LightComponent: Directional/Point/Spot
    renderer.h/.cpp              — двухпроходный рендерер: GBuffer → Deferred → swapchain
  editor/
    editor.h/.cpp                — ImGui-редактор (Hierarchy/Inspector/Assets/Console/Stats)
  vendor/
    stb_image_impl.cpp           — реализация stb_image
```
