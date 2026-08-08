#pragma once

// ============================================
// EOA ENGINE - Core Module
// ============================================
// Единая точка входа для всех core-модулей
// ============================================

#include "Core/Platform.h"
#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "Core/Events.h"
#include "Core/Engine.h"
#include "Core/World.h"
#include "Core/ResourceManager.h"

namespace EOA {

// Краткая справка по архитектуре движка:
// 
// 1. Engine - главный класс, управляет жизненным циклом приложения
// 2. World - контейнер для всех акторов и компонентов
// 3. Actor - игровой объект с трансформацией и компонентами
// 4. Component - функциональные модули актора (рендер, физика, AI)
// 5. ResourceManager - загрузка и кэширование ресурсов
// 6. InputSystem - обработка ввода (клавиатура, мышь)
// 7. TimeSystem - управление временем и FPS
// 8. EventSystem - система событий для слабой связности
// 9. Logger - логирование с разными уровнями
// 10. IRenderer - интерфейс рендерера (Vulkan/DX12/OpenGL)
//
// Глобальные указатели (в стиле UE):
// - gEngine   : доступ к главному классу движка
// - gWorld    : доступ к текущему миру
// - gResources: доступ к менеджеру ресурсов
// - gInput    : доступ к системе ввода
// - gTime     : доступ к системе времени
//
// Макросы для быстрого доступа:
// - DeltaTime()       : время последнего кадра
// - TotalTime()       : общее время игры
// - GetFPS()          : текущий FPS
// - IsKeyPressed(key) : проверка нажатия клавиши
// - SpawnActor<T>()   : создание актора в мире
// - LoadResource<T>() : загрузка ресурса
//
// Пример создания игры:
// см. examples/GameExample.cpp

} // namespace EOA
