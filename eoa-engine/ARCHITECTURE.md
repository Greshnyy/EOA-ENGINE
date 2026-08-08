# EOA Engine Architecture

## Обзор архитектуры (в стиле Unreal Engine)

EOA Engine — это модульный игровой движок, вдохновленный архитектурой Unreal Engine. Он предоставляет все необходимые системы для создания современных 3D-игр.

## Структура проекта

```
eoa-engine/
├── include/
│   ├── EoaEngine.h          # Главный заголовочный файл (все-в-одном)
│   ├── Core.h               # Core модуль
│   ├── Core/
│   │   ├── Platform.h       # Платформенные макросы и API экспорт
│   │   ├── Types.h          # Базовые типы (int8, uint32, EntityID...)
│   │   ├── Logger.h         # Система логирования
│   │   ├── Time.h           # Система времени (DeltaTime, FPS)
│   │   ├── Input.h          # Система ввода (клавиатура, мышь)
│   │   ├── Events.h         # Система событий
│   │   ├── Engine.h         # Главный класс движка
│   │   ├── World.h          # Мир, акторы, компоненты
│   │   └── ResourceManager.h # Менеджер ресурсов
│   ├── Math/
│   │   ├── Vector.h         # Векторы (Vec2, Vec3, Vec4)
│   │   └── Matrix.h         # Матрицы 4x4
│   ├── Render/
│   │   └── Renderer.h       # Интерфейс рендерера
│   └── Resources/
│       └── Resource.h       # Базовый класс ресурса
├── src/                     # Исходный код реализации
├── examples/
│   └── GameExample.cpp      # Пример игры
├── assets/                  # Ресурсы игры
└── shaders/                 # Шейдеры
```

## Ключевые компоненты

### 1. Engine (Ядро движка)
Главный класс `EOA::Engine` управляет жизненным циклом приложения:
- Инициализация систем
- Главный цикл (ProcessEvents → Update → Render)
- Управление системами через `AddSystem<T>()`

**Глобальный доступ:** `gEngine`

### 2. World (Мир)
`EOA::World` — контейнер для всех игровых объектов:
- Спавн/удаление акторов через `SpawnActor<T>()`
- Поиск акторов по типу
- Обновление всех акторов и компонентов

**Глобальный доступ:** `gWorld`

### 3. Actor (Актор)
Базовый класс для всех игровых объектов:
- Трансформация (Position, Rotation, Scale)
- Компоненты через `AddComponent<T>()`
- Жизненный цикл (Initialize, Update, Render)

### 4. Component (Компонент)
Функциональные модули актора:
- Наследуются от `EOA::Component`
- Имеют доступ к владельцу через `GetOwner()`
- Методы: Initialize(), Update(), Render()

### 5. ResourceManager (Ресурсы)
Управление ассетами:
- Загрузка/выгрузка ресурсов
- Кэширование
- Асинхронная загрузка

**Глобальный доступ:** `gResources`

### 6. InputSystem (Ввод)
Обработка ввода:
- Состояние клавиш: `IsKeyPressed()`, `IsKeyJustPressed()`
- Состояние мыши: позиция, кнопки, скролл
- Событийная модель

**Глобальный доступ:** `gInput`

### 7. TimeSystem (Время)
Управление временем:
- `DeltaTime()` — время последнего кадра
- `TotalTime()` — общее время игры
- `FPS()` — кадры в секунду
- Масштаб времени (slow-mo, пауза)

**Глобальный доступ:** `gTime`

### 8. EventSystem (События)
Слабая связность между системами:
- Типы событий: Window, Input, Actor, Scene
- Подписка через `Subscribe()`
- Диспетчеризация через `Dispatch()`

### 9. Logger (Логирование)
Система логирования с уровнями:
- Trace, Debug, Info, Warning, Error, Critical
- Вывод в консоль и файл
- Макросы: `EOA_LOG_INFO()`, `EOA_LOG_ERROR()` и т.д.

### 10. IRenderer (Рендерер)
Интерфейс для графических API:
- Создание буферов, текстур, шейдеров
- Рендеринг мешей
- Статистика (draw calls, triangles, GPU memory)

## Глобальные макросы (в стиле UE)

| Макрос | Описание |
|--------|----------|
| `gEngine` | Доступ к главному классу движка |
| `gWorld` | Доступ к текущему миру |
| `gResources` | Доступ к менеджеру ресурсов |
| `gInput` | Доступ к системе ввода |
| `gTime` | Доступ к системе времени |
| `DeltaTime()` | Время последнего кадра |
| `TotalTime()` | Общее время игры |
| `GetFPS()` | Текущий FPS |
| `IsKeyPressed(key)` | Проверка нажатия клавиши |
| `SpawnActor<T>()` | Создание актора |
| `LoadResource<T>(path)` | Загрузка ресурса |

## Пример использования

```cpp
#include "EoaEngine.h"

using namespace EOA;

// Компонент вращения
class RotatorComponent : public Component {
    void Update(float deltaTime) override {
        auto* owner = GetOwner();
        Vector3 rot = owner->GetRotation();
        rot.Y += 45.0f * deltaTime;
        owner->SetRotation(rot);
    }
};

// Игрок
class PlayerActor : public Actor {
    void Initialize() override {
        SetName("Player");
        AddComponent<RotatorComponent>();
    }
    
    void Update(float deltaTime) override {
        if (IsKeyPressed(KeyCode::W)) {
            Vector3 pos = GetPosition();
            pos.Z += 5.0f * deltaTime;
            SetPosition(pos);
        }
    }
};

// Конфигурация
class MyGameConfig {
public:
    static void Configure(EngineConfig& config) {
        config.Title = "My Game";
        config.Width = 1920;
        config.Height = 1080;
    }
};

// Главная функция
int main(int argc, char** argv) {
    EngineConfig config;
    MyGameConfig::Configure(config);
    return Application::Run(config);
}
```

## Системные требования

- **Компилятор:** C++17 или выше
- **Платформы:** Windows, Linux, macOS
- **Графические API:** Vulkan, DirectX 12, OpenGL 4.5 (планируется)

## Планы развития

1. **Физический движок** — коллизии, raycasting, rigidbody
2. **Скриптинг** — интеграция Lua/Python
3. **Анимация** — скелетная анимация, blend trees
4. **Audio** — 3D звук, микшер
5. **UI** — система интерфейсов
6. **Terrain** — ландшафт на основе heightmap
7. **AI** — навигация, behavior trees
8. **Редактор** — визуальный редактор уровней

## Лицензия

См. файл LICENSE
