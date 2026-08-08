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
