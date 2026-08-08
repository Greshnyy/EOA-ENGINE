#include "UI/UI.h"
#include "Core/Logger.h"
#include "Render/renderer.h"
#include <algorithm>

namespace eoa {

// ============================================================================
// UIBase
// ============================================================================

UIBase::UIBase(const std::string& name) : name_(name) {}

UIBase::~UIBase() = default;

void UIBase::AddChild(std::unique_ptr<UIBase> child) {
    if (!child) return;
    
    child->SetParent(this);
    children_.push_back(std::move(child));
}

void UIBase::RemoveChild(UIBase* child) {
    if (!child) return;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<UIBase>& c) {
            return c.get() == child;
        });
    
    if (it != children_.end()) {
        children_.erase(it);
    }
}

UIBase* UIBase::FindChildByName(const std::string& name) const {
    for (const auto& child : children_) {
        if (child && child->GetName() == name) {
            return child.get();
        }
    }
    
    // Рекурсивный поиск в детях
    for (const auto& child : children_) {
        if (child) {
            auto* found = child->FindChildByName(name);
            if (found) return found;
        }
    }
    
    return nullptr;
}

bool UIBase::ContainsPoint(float x, float y) const {
    if (!visible_) return false;
    
    // Простая проверка попадания в прямоугольник
    // С учётом anchor
    float left = posX_;
    float top = posY_;
    float right = posX_ + sizeX_;
    float bottom = posY_ + sizeY_;
    
    // Коррекция по anchor
    switch (anchor_) {
        case UIAnchor::TopLeft:
            break;
        case UIAnchor::TopCenter:
            left -= sizeX_ / 2;
            right += sizeX_ / 2;
            break;
        case UIAnchor::TopRight:
            left -= sizeX_;
            right -= sizeX_;
            break;
        case UIAnchor::MiddleLeft:
            top -= sizeY_ / 2;
            bottom += sizeY_ / 2;
            break;
        case UIAnchor::MiddleCenter:
            left -= sizeX_ / 2;
            right += sizeX_ / 2;
            top -= sizeY_ / 2;
            bottom += sizeY_ / 2;
            break;
        case UIAnchor::MiddleRight:
            left -= sizeX_;
            right -= sizeX_;
            top -= sizeY_ / 2;
            bottom += sizeY_ / 2;
            break;
        case UIAnchor::BottomLeft:
            top -= sizeY_;
            bottom -= sizeY_;
            break;
        case UIAnchor::BottomCenter:
            left -= sizeX_ / 2;
            right += sizeX_ / 2;
            top -= sizeY_;
            bottom -= sizeY_;
            break;
        case UIAnchor::BottomRight:
            left -= sizeX_;
            right -= sizeX_;
            top -= sizeY_;
            bottom -= sizeY_;
            break;
    }
    
    return x >= left && x <= right && y >= top && y <= bottom;
}

// ============================================================================
// UIPanel
// ============================================================================

UIPanel::UIPanel(const std::string& name) : UIBase(name) {}

void UIPanel::Render() {
    if (!visible_) return;
    
    // Рендер фона панели
    // В реальной реализации здесь был бы вызов Vulkan для отрисовки quad
    
    // Рендер детей
    for (auto& child : children_) {
        if (child && child->IsVisible()) {
            child->Render();
        }
    }
}

// ============================================================================
// UIButton
// ============================================================================

UIButton::UIButton(const std::string& name) : UIPanel(name) {
    sizeX_ = 120;
    sizeY_ = 40;
}

bool UIButton::OnMouseDown(float x, float y, int button) {
    if (!interactable_ || !visible_) return false;
    if (button != 0) return false; // Только левая кнопка
    
    if (ContainsPoint(x, y)) {
        isPressed_ = true;
        return true;
    }
    return false;
}

bool UIButton::OnMouseUp(float x, float y, int button) {
    if (!interactable_ || !visible_ || !isPressed_) return false;
    
    isPressed_ = false;
    
    if (ContainsPoint(x, y) && onClick_) {
        onClick_();
        return true;
    }
    
    return false;
}

void UIButton::Render() {
    if (!visible_) return;
    
    // Определение цвета кнопки
    Color32 renderColor = bgColor_;
    
    if (isPressed_) {
        // Цвет при нажатии (темнее)
        renderColor.r = static_cast<uint8_t>(renderColor.r * 0.7f);
        renderColor.g = static_cast<uint8_t>(renderColor.g * 0.7f);
        renderColor.b = static_cast<uint8_t>(renderColor.b * 0.7f);
    } else if (isHovered_) {
        // Цвет при наведении (светлее)
        renderColor.r = static_cast<uint8_t>(glm::min(255, renderColor.r * 1.1f));
        renderColor.g = static_cast<uint8_t>(glm::min(255, renderColor.g * 1.1f));
        renderColor.b = static_cast<uint8_t>(glm::min(255, renderColor.b * 1.1f));
    }
    
    // Рендер фона кнопки
    // В реальной реализации Vulkan draw call
    
    // Рендер текста
    // В реальной реализации отрисовка текста шрифтом
    
    // Рендер детей
    for (auto& child : children_) {
        if (child && child->IsVisible()) {
            child->Render();
        }
    }
}

// ============================================================================
// UILabel
// ============================================================================

UILabel::UILabel(const std::string& name) : UIBase(name) {
    sizeX_ = 100;
    sizeY_ = 20;
}

void UILabel::Render() {
    if (!visible_) return;
    
    // Рендер текста
    // В реальной реализации отрисовка текста с использованием шрифта
    
    // Рендер детей (если есть)
    for (auto& child : children_) {
        if (child && child->IsVisible()) {
            child->Render();
        }
    }
}

// ============================================================================
// UISlider
// ============================================================================

UISlider::UISlider(const std::string& name) : UIPanel(name) {
    sizeX_ = 200;
    sizeY_ = 20;
}

void UISlider::SetValue(float value) {
    value_ = glm::clamp(value, minValue_, maxValue_);
    
    if (onValueChanged_) {
        onValueChanged_(value_);
    }
}

bool UISlider::OnMouseDown(float x, float y, int button) {
    if (!interactable_ || !visible_) return false;
    if (button != 0) return false;
    
    if (ContainsPoint(x, y)) {
        isDragging_ = true;
        
        // Обновление значения на основе позиции клика
        float clickPos = x - posX_;
        if (!horizontal_) {
            clickPos = posY_ + sizeY_ - y;
        }
        
        float ratio = glm::clamp(clickPos / (horizontal_ ? sizeX_ : sizeY_), 0.0f, 1.0f);
        SetValue(minValue_ + ratio * (maxValue_ - minValue_));
        
        return true;
    }
    
    return false;
}

bool UISlider::OnMouseMove(float x, float y) {
    if (!isDragging_ || !interactable_ || !visible_) return false;
    
    // Обновление значения на основе позиции мыши
    float mousePos = horizontal_ ? (x - posX_) : (posY_ + sizeY_ - y);
    float ratio = glm::clamp(mousePos / (horizontal_ ? sizeX_ : sizeY_), 0.0f, 1.0f);
    SetValue(minValue_ + ratio * (maxValue_ - minValue_));
    
    return true;
}

bool UISlider::OnMouseUp(float x, float y, int button) {
    if (!isDragging_) return false;
    
    isDragging_ = false;
    return true;
}

void UISlider::Render() {
    if (!visible_) return;
    
    // Рендер фона слайдера (track)
    // В реальной реализации Vulkan draw
    
    // Расчёт позиции handle
    float ratio = (value_ - minValue_) / (maxValue_ - minValue_);
    float handlePos = ratio * (horizontal_ ? sizeX_ : sizeY_);
    float handleSize = horizontal_ ? sizeY_ : sizeX_;
    
    // Рендер handle (ползунка)
    // В реальной реализации Vulkan draw
    
    // Рендер детей
    for (auto& child : children_) {
        if (child && child->IsVisible()) {
            child->Render();
        }
    }
}

// ============================================================================
// UIImage
// ============================================================================

UIImage::UIImage(const std::string& name) : UIPanel(name) {}

bool UIImage::LoadImage(const std::string& filename) {
    imageFile_ = filename;
    
    // В реальной реализации загрузка текстуры
    EOA_LOG_INFO("UIImage: Loading image '{}'", filename);
    
    // Заглушка - считаем что изображение загружено
    textureHandle_ = reinterpret_cast<void*>(1);
    return true;
}

void UIImage::UnloadImage() {
    if (textureHandle_) {
        // Выгрузка текстуры
        textureHandle_ = nullptr;
    }
    imageFile_.clear();
}

void UIImage::Render() {
    if (!visible_) return;
    
    // Рендер изображения с текстурой
    // В реальной реализации Vulkan draw textured quad
    
    // Рендер детей
    for (auto& child : children_) {
        if (child && child->IsVisible()) {
            child->Render();
        }
    }
}

// ============================================================================
// UICanvas
// ============================================================================

UICanvas::UICanvas() = default;

UICanvas::~UICanvas() = default;

void UICanvas::AddRoot(std::unique_ptr<UIBase> element) {
    if (!element) return;
    roots_.push_back(std::move(element));
}

void UICanvas::RemoveRoot(UIBase* element) {
    if (!element) return;
    
    auto it = std::find_if(roots_.begin(), roots_.end(),
        [element](const std::unique_ptr<UIBase>& e) {
            return e.get() == element;
        });
    
    if (it != roots_.end()) {
        roots_.erase(it);
    }
}

void UICanvas::Update(float deltaTime) {
    if (!enabled_) return;
    
    for (auto& root : roots_) {
        if (root && root->IsVisible()) {
            root->Update(deltaTime);
        }
    }
}

void UICanvas::Render() {
    if (!enabled_) return;
    
    // Установка orthographic projection для UI
    // В реальной реализации настройка viewport и projection matrix
    
    for (auto& root : roots_) {
        if (root && root->IsVisible()) {
            root->Render();
        }
    }
}

bool UICanvas::HandleMouseDown(float x, float y, int button) {
    if (!enabled_) return false;
    
    // Конвертация координат в screen space
    float uiX = x;
    float uiY = screenHeight_ - y;
    
    // Поиск элемента под курсором (с конца - верхние элементы primero)
    for (auto it = roots_.rbegin(); it != roots_.rend(); ++it) {
        if (*it && (*it)->IsVisible() && (*it)->IsInteractable()) {
            if ((*it)->OnMouseDown(uiX, uiY, button)) {
                pressedElement_ = it->get();
                return true;
            }
        }
    }
    
    return false;
}

bool UICanvas::HandleMouseUp(float x, float y, int button) {
    if (!enabled_) return false;
    
    float uiX = x;
    float uiY = screenHeight_ - y;
    
    if (pressedElement_) {
        pressedElement_->OnMouseUp(uiX, uiY, button);
        pressedElement_ = nullptr;
        return true;
    }
    
    return false;
}

bool UICanvas::HandleMouseMove(float x, float y) {
    if (!enabled_) return false;
    
    float uiX = x;
    float uiY = screenHeight_ - y;
    
    // Поиск элемента под курсором
    UIBase* newHovered = nullptr;
    
    for (auto it = roots_.rbegin(); it != roots_.rend(); ++it) {
        if (*it && (*it)->IsVisible() && (*it)->IsInteractable()) {
            if ((*it)->ContainsPoint(uiX, uiY)) {
                newHovered = it->get();
                break;
            }
        }
    }
    
    if (newHovered != hoveredElement_) {
        // Mouse enter/leave логика могла бы быть здесь
        hoveredElement_ = newHovered;
    }
    
    if (hoveredElement_) {
        hoveredElement_->OnMouseMove(uiX, uiY);
    }
    
    return hoveredElement_ != nullptr;
}

bool UICanvas::HandleKeyDown(int key) {
    if (!enabled_) return false;
    
    // Фокус элемент мог бы получать ввод
    for (auto& root : roots_) {
        if (root && root->IsVisible() && root->IsInteractable()) {
            if (root->OnKeyDown(key)) {
                return true;
            }
        }
    }
    
    return false;
}

bool UICanvas::HandleKeyUp(int key) {
    if (!enabled_) return false;
    
    for (auto& root : roots_) {
        if (root && root->IsVisible() && root->IsInteractable()) {
            if (root->OnKeyUp(key)) {
                return true;
            }
        }
    }
    
    return false;
}

bool UICanvas::HandleCharTyped(unsigned int c) {
    if (!enabled_) return false;
    
    for (auto& root : roots_) {
        if (root && root->IsVisible() && root->IsInteractable()) {
            if (root->OnCharTyped(c)) {
                return true;
            }
        }
    }
    
    return false;
}

UIBase* UICanvas::FindElementByName(const std::string& name) const {
    for (const auto& root : roots_) {
        if (!root) continue;
        
        if (root->GetName() == name) {
            return root.get();
        }
        
        auto* found = root->FindChildByName(name);
        if (found) return found;
    }
    
    return nullptr;
}

void UICanvas::SetScreenSize(float width, float height) {
    screenWidth_ = width;
    screenHeight_ = height;
}

// ============================================================================
// UIComponent
// ============================================================================

UIComponent::UIComponent(const std::string& name) : Component(name) {}

UIComponent::~UIComponent() = default;

void UIComponent::Tick(float deltaTime) {
    canvas_.Update(deltaTime);
}

} // namespace eoa
