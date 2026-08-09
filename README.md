# 🎮 EOA Engine — движок Echoes of Ash

Собственный игровой движок на **C++ / Vulkan** с Actor-Component архитектурой, PBR Deferred Rendering и встроенным ImGui-редактором. Разрабатывается под сокращённый (не-Unreal) скоуп проекта.

---

## ✨ Текущие возможности

| Система | Статус |
|---------|--------|
| 🎨 **PBR Deferred Rendering** | ✅ GBuffer + Cook-Torrance BRDF |
| 🏗️ **Actor-Component** | ✅ Аналог AActor/UActorComponent (UE) |
| 🖼️ **glTF загрузка** | ✅ Модели, материалы, текстуры |
| 🌑 **Освещение** | ✅ Directional / Point / Spot lights |
| 🎥 **FPS-камера** | ✅ WASD + мышь |
| 🖊️ **ImGui Editor** | ✅ Hierarchy, Inspector, AssetBrowser, Console |
| ⚡ **Vulkan Backend** | ✅ Swapchain, MRT, UBO/SSBO, Push Constants |
| 🌑 **Тени** | 🔄 Шейдеры скомпилированы, не подключены |
| ⚙️ **Физика** | 📋 Спроектирована (не реализована) |
| 🤖 **AI** | 📋 Спроектирована (не реализована) |
| 🔊 **Аудио** | 📋 Спроектирована (не реализована) |
| 🌐 **Networking** | 📋 Спроектирована (не реализована) |
| 📜 **Lua Scripting** | 📋 Спроектирована (не реализована) |

---

## 🚀 Быстрый старт (Windows)

### 1. Требования

- **ОС:** Windows 10/11 (x64)
- **Компилятор:** Visual Studio 2022+ (или VS 18 2026)
- **Vulkan SDK:** 1.3+ ([скачать](https://vulkan.lunarg.com/))
- **CMake:** 3.20+ ([скачать](https://cmake.org/download/))
- **Git:** для клонирования vcpkg

### 2. Установка зависимостей

#### 2.1. Visual Studio

При установке выбери рабочую нагрузку **"Desktop development with C++"**. Это установит MSVC, Windows SDK и MSBuild.

> **Важно:** если у тебя Visual Studio 2026 (папка `18/`) — используй **Developer Command Prompt for VS 2026**, а не обычный CMD. Иначе CMake не найдёт компилятор.

#### 2.2. Vulkan SDK

Установи с [vulkan.lunarg.com](https://vulkan.lunarg.com/). После установки **перезапусти терминал** и проверь:

```cmd
echo %VULKAN_SDK%
```

Должен вывести путь вроде `C:\VulkanSDK.4.357.0`.

#### 2.3. vcpkg + GLFW3 + GLM

```cmd
cd C:git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.bootstrap-vcpkg.bat
.vcpkg integrate install

:: Установка зависимостей
.vcpkg install glfw3:x64-windows glm:x64-windows
```

### 3. Клонирование и сборка

```cmd
git clone https://github.com/Greshnyy/EOA-ENGINE.git
cd EOA-ENGINE
mkdir build && cd build

:: Из Developer Command Prompt for VS 2022/2026:
cmake -A x64 -DCMAKE_TOOLCHAIN_FILE=C:vcpkg\scriptsbuildsystemsvcpkg.cmake ..
cmake --build . --config Debug
```

> **Внимание:** не используй одну `build` папку из WSL и Windows одновременно — пути несовместимы. Если раньше собирал в WSL (`/workspace/eoa-engine/build`), полностью очисти `build` в Windows:
> ```cmd
> del /q CMakeCache.txt cmake_install.cmake Makefile
> rmdir /s /q CMakeFiles .cmake
> ```

### 4. Запуск

```cmd
cd Debug
.\eoa_engine.exe
```

> **Важно:** запускай `.exe` из папки, где лежат `shaders/` и `assets/` (CMake копирует их автоматически). Движок ищет ресурсы по относительному пути.

---

## 🐧 Сборка на Linux

```bash
sudo apt install cmake g++ libvulkan-dev libglfw3-dev glslc vulkan-validationlayers libglm-dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
./eoa_engine
```

---

## 🎮 Управление

| Клавиша | Действие |
|---------|----------|
| **W A S D** | Движение камеры |
| **Зажатая ПКМ + мышь** | Обзор (курсор захватывается только при зажатой кнопке) |
| **Space** | Вверх |
| **Left Ctrl** | Вниз |
| **Закрыть окно** | Выход |

---

## 📁 Структура проекта

```
EOAEngine/
├── CMakeLists.txt
├── assets/
│   ├── textures/checker.png          — placeholder (охра/тёмная)
│   ├── models/test_pyramid.gltf      — тестовый ассет без материала
│   ├── models/test_cube_textured.gltf — тест с материалом
│   └── models/cube_albedo.png        — текстура для теста
├── third_party/
│   ├── stb_image.h / stb_image_write.h  — загрузка/генерация изображений
│   ├── tiny_gltf.h / json.hpp           — парсинг glTF
│   └── imgui/                           — Dear ImGui (docking-ветка)
├── shaders/
│   ├── triangle.vert/.frag           — старый forward (сохранён)
│   ├── gbuffer.vert/.frag            — GBuffer fill pass (MRT)
│   ├── deferred_lighting.vert/.frag  — Deferred PBR fullscreen
│   └── shadow_depth.vert/.frag       — заготовка для shadow mapping
└── src/
    ├── main.cpp                      — точка входа, bootstrap, Editor
    ├── log.h                         — EOA_LOG, EOA_CHECK_VK
    ├── core/
    │   ├── object.h/.cpp             — UObject: RTTI, InstanceID
    │   ├── actor.h/.cpp              — AActor: компоненты, жизненный цикл
    │   ├── component.h/.cpp          — UActorComponent: база
    │   └── transform_component.h/.cpp — позиция/поворот/масштаб
    ├── platform/
    │   └── window.h/.cpp             — обёртка GLFW
    ├── rhi/
    │   ├── vk_instance.h/.cpp        — VkInstance + debug messenger
    │   ├── vk_device.h/.cpp          — выбор GPU + логическое устройство
    │   ├── vk_swapchain.h/.cpp       — swapchain + image views
    │   └── file_utils.h/.cpp         — чтение бинарников (.spv)
    ├── renderer/
    │   ├── vertex.h                  — Vertex struct + UBO
    │   ├── buffer_utils.h/.cpp       — CreateBuffer/CreateImage, staging
    │   ├── mesh.h/.cpp               — GPU vertex+index буферы
    │   ├── texture.h/.cpp            — VkImage/View/Sampler
    │   ├── pipeline.h/.cpp           — параметризованный graphics pipeline
    │   ├── gbuffer.h/.cpp            — GBuffer: 4 MRT
    │   ├── deferred_lighting.h/.cpp  — Deferred PBR pass
    │   ├── world.h                   — UWorld: контейнер Actor-ов
    │   ├── gltf_loader.h/.cpp        — парсинг .gltf/.glb
    │   ├── mesh_component.h/.cpp     — UStaticMeshComponent
    │   ├── camera_component.h/.cpp   — UCameraComponent (FPS)
    │   ├── light_component.h/.cpp    — ULightComponent
    │   └── renderer.h/.cpp           — двухпроходный рендерер
    ├── editor/
    │   └── editor.h/.cpp             — ImGui: Hierarchy/Inspector/Assets/Console/Stats
    └── vendor/
        └── stb_image_impl.cpp        — реализация stb_image
```

---

## 🏗️ Архитектура Actor-Component

| Класс | Файл | Аналог в UE |
|-------|------|-------------|
| `Object` | `core/object.h/.cpp` | `UObject` — RTTI, InstanceID, именование |
| `Actor` | `core/actor.h/.cpp` | `AActor` — `AddComponent<T>()`, `GetComponent<T>()`, BeginPlay/Tick/EndPlay |
| `Component` | `core/component.h/.cpp` | `UActorComponent` — привязка к Actor, active-флаг |
| `TransformComponent` | `core/transform_component.h/.cpp` | `USceneComponent` — позиция/поворот(quat)/масштаб |
| `MeshComponent` | `renderer/mesh_component.h/.cpp` | `UStaticMeshComponent` — Mesh + MaterialData |
| `CameraComponent` | `renderer/camera_component.h/.cpp` | `UCameraComponent` — FPS-камера, перспектива |
| `LightComponent` | `renderer/light_component.h/.cpp` | `ULightComponent` — Directional/Point/Spot |
| `World` | `renderer/world.h` | `UWorld` — `SpawnActor<T>()`, DestroyActor, итерация |

---

## 🎨 PBR Deferred Rendering

### Два прохода:

1. **GBuffer Pass** (`gbuffer.vert/.frag`) — 4 MRT:
   - **Attachment 0:** Albedo (RGBA8_SRGB)
   - **Attachment 1:** World Normal (RGBA16F)
   - **Attachment 2:** ORM — Roughness R, Metallic G (RGBA8)
   - **Depth:** D32_SFLOAT

2. **Deferred Lighting Pass** (`deferred_lighting.vert/.frag`):
   - Fullscreen triangle (без VB, через `gl_VertexIndex`)
   - Cook-Torrance BRDF:
     - Normal Distribution: GGX
     - Geometry: Smith (Schlick-GGX)
     - Fresnel: Schlick approximation
   - Directional + Point lights (Spot — заготовка)
   - Reinhard tone mapping + gamma correction

**Light buffers:** UBO (camera pos + ambient + light count) + SSBO (до 64 источников, авто-сбор из `LightComponent`). Если ни одного LightComponent нет — создаётся дефолтный directional sun.

---

## 🗺️ Что уже работает (Milestones)

| Milestone | Описание |
|-----------|----------|
| **M1 — Bootstrap** | Окно (GLFW), Vulkan instance, validation layers, GPU, логическое устройство |
| **M2 — Swapchain** | Swapchain, render pass, framebuffers, double buffering, resize без креша |
| **M3 — Треугольник** | SPIR-V шейдеры (автокомпиляция), graphics pipeline, `vkCmdDraw` |
| **M4 — Буферы + Depth** | Vertex/Index буферы (staging → DEVICE_LOCAL), depth test, UBO с MVP (GLM) |
| **M5 — Текстуры + Камера** | `stb_image`, placeholder текстура, FPS-камера (WASD + мышь), combined image sampler |
| **M6-7 — Сцена + glTF + Editor** | Per-object push constants, glTF-загрузчик, ImGui-редактор, материалы |
| **M8 — Actor-Component + PBR** | `World`/`Actor`/`Component`, Deferred PBR (GBuffer + DeferredLighting), `CameraComponent`, `LightComponent` |

---

## 🛠️ Расширение движка

### Добавление собственного компонента

```cpp
// MyComponent.h
#pragma once
#include "core/component.h"

namespace EOA {
    class MyComponent : public Component {
    public:
        void OnInit() override;
        void OnUpdate(float deltaTime) override;
        void SetValue(float v) { m_Value = v; }
    private:
        float m_Value = 0.0f;
    };
}

// MyComponent.cpp
#include "MyComponent.h"
namespace EOA {
    void MyComponent::OnInit() { EOA_LOG_INFO("MyComponent initialized"); }
    void MyComponent::OnUpdate(float dt) { m_Value += dt; }
}
```

---

## ⚠️ Если что-то не собирается

| Ошибка | Решение |
|--------|---------|
| `Could not find glfw3` / `glm` | Проверь `-DCMAKE_TOOLCHAIN_FILE=...` и архитектуру `x64-windows` |
| `VULKAN_SDK not found` | Перезапусти терминал после установки SDK, проверь `echo %VULKAN_SDK%` |
| `Generator could not find any instance of Visual Studio` | Открывай **Developer Command Prompt for VS**, а не обычный CMD |
| `The build tools for v143 cannot be found` | В Visual Studio Installer добавь workload **"Desktop development with C++"** |
| `CMakeCache.txt is different than the directory` | Очисти `build` полностью (`del CMakeCache.txt` + `rmdir /s /q CMakeFiles`) — кеш создан в другой среде (WSL/Linux) |
| `glslc not found` | Добавь `%VULKAN_SDK%\Bin` в PATH вручную |

---

## 📄 Лицензия

EOA Engine распространяется под лицензией **PROPRIETARY SOFTWARE LICENSE**. См. файл [LICENSE](LICENSE).

---

**Создано с ❤️ для Echoes of Ash**
