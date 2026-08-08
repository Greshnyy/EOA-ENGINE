#pragma once
#include "core/component.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace eoa {

// ============================================================================
// UI SYSTEM - Canvas-based интерфейс
// ============================================================================

enum class UIAnchor : uint8_t {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

enum class UIClipMode : uint8_t {
    None,
    ClipContents,
    ScrollHorizontal,
    ScrollVertical,
    ScrollBoth
};

struct Color32 {
    uint8_t r, g, b, a;
    Color32(uint8_t _r = 255, uint8_t _g = 255, uint8_t _b = 255, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}
    
    static const Color32 White;
    static const Color32 Black;
    static const Color32 Red;
    static const Color32 Green;
    static const Color32 Blue;
    static const Color32 Transparent;
};

struct Vector4 {
    float x, y, z, w;
    Vector4(float _x = 0, float _y = 0, float _z = 0, float _w = 0)
        : x(_x), y(_y), z(_z), w(_w) {}
};

// ============================================================================
// UIBase - базовый класс для всех UI элементов
// ============================================================================

class UIBase {
public:
    virtual ~UIBase() = default;
    
    // Позиция и размер
    void SetPosition(float x, float y) { posX_ = x; posY_ = y; }
    void SetSize(float width, float height) { sizeX_ = width; sizeY_ = height; }
    
    float GetX() const { return posX_; }
    float GetY() const { return posY_; }
    float GetWidth() const { return sizeX_; }
    float GetHeight() const { return sizeY_; }
    
    // Anchor
    void SetAnchor(UIAnchor anchor) { anchor_ = anchor; }
    UIAnchor GetAnchor() const { return anchor_; }
    
    // Видимость
    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }
    
    // Interactable
    void SetInteractable(bool interactable) { interactable_ = interactable; }
    bool IsInteractable() const { return interactable_; }
    
    // Parent/Children
    UIBase* GetParent() const { return parent_; }
    void SetParent(UIBase* parent) { parent_ = parent; }
    const std::vector<UIBase*>& GetChildren() const { return children_; }
    
    void AddChild(std::unique_ptr<UIBase> child);
    void RemoveChild(UIBase* child);
    
    // Имя и ID
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    int GetID() const { return id_; }
    
    // Поиск детей по имени
    UIBase* FindChildByName(const std::string& name) const;
    
    // Обновление
    virtual void Update(float deltaTime) {}
    virtual void Render() = 0;
    
    // События ввода
    virtual bool OnMouseDown(float x, float y, int button) { return false; }
    virtual bool OnMouseUp(float x, float y, int button) { return false; }
    virtual bool OnMouseMove(float x, float y) { return false; }
    virtual bool OnKeyDown(int key) { return false; }
    virtual bool OnKeyUp(int key) { return false; }
    virtual bool OnCharTyped(unsigned int c) { return false; }
    
    // Hit test
    virtual bool ContainsPoint(float x, float y) const;
    
protected:
    UIBase(const std::string& name = "UIElement");
    
    std::string name_;
    int id_ = 0;
    float posX_ = 0, posY_ = 0;
    float sizeX_ = 100, sizeY_ = 100;
    UIAnchor anchor_ = UIAnchor::MiddleCenter;
    bool visible_ = true;
    bool interactable_ = true;
    
    UIBase* parent_ = nullptr;
    std::vector<std::unique_ptr<UIBase>> children_;
};

// ============================================================================
// UIPanel - контейнер для UI элементов
// ============================================================================

class UIPanel : public UIBase {
public:
    explicit UIPanel(const std::string& name = "Panel");
    
    void SetBackgroundColor(const Color32& color) { bgColor_ = color; }
    const Color32& GetBackgroundColor() const { return bgColor_; }
    
    void SetClipMode(UIClipMode mode) { clipMode_ = mode; }
    UIClipMode GetClipMode() const { return clipMode_; }
    
    void Render() override;
    
private:
    Color32 bgColor_ = Color32::Transparent;
    UIClipMode clipMode_ = UIClipMode::None;
};

// ============================================================================
// UIButton - кнопка
// ============================================================================

class UIButton : public UIPanel {
public:
    explicit UIButton(const std::string& name = "Button");
    
    using OnClickCallback = std::function<void()>;
    
    void SetText(const std::string& text) { text_ = text; }
    const std::string& GetText() const { return text_; }
    
    void SetTextColor(const Color32& color) { textColor_ = color; }
    const Color32& GetTextColor() const { return textColor_; }
    
    void SetOnClick(OnClickCallback callback) { onClick_ = callback; }
    
    bool OnMouseDown(float x, float y, int button) override;
    bool OnMouseUp(float x, float y, int button) override;
    void Render() override;
    
private:
    std::string text_;
    Color32 textColor_ = Color32::White;
    OnClickCallback onClick_;
    bool isPressed_ = false;
    bool isHovered_ = false;
};

// ============================================================================
// UILabel - текстовая метка
// ============================================================================

class UILabel : public UIBase {
public:
    explicit UILabel(const std::string& name = "Label");
    
    void SetText(const std::string& text) { text_ = text; }
    const std::string& GetText() const { return text_; }
    
    void SetFontSize(int size) { fontSize_ = size; }
    int GetFontSize() const { return fontSize_; }
    
    void SetTextColor(const Color32& color) { textColor_ = color; }
    const Color32& GetTextColor() const { return textColor_; }
    
    void Render() override;
    
private:
    std::string text_;
    int fontSize_ = 16;
    Color32 textColor_ = Color32::White;
};

// ============================================================================
// UISlider - ползунок
// ============================================================================

class UISlider : public UIPanel {
public:
    explicit UISlider(const std::string& name = "Slider");
    
    using OnValueChangedCallback = std::function<void(float)>;
    
    void SetValue(float value);
    float GetValue() const { return value_; }
    
    void SetMinValue(float min) { minValue_ = min; }
    void SetMaxValue(float max) { maxValue_ = max; }
    
    void SetDirection(bool horizontal) { horizontal_ = horizontal; }
    bool IsHorizontal() const { return horizontal_; }
    
    void SetOnValueChanged(OnValueChangedCallback callback) { onValueChanged_ = callback; }
    
    bool OnMouseDown(float x, float y, int button) override;
    bool OnMouseMove(float x, float y) override;
    bool OnMouseUp(float x, float y, int button) override;
    void Render() override;
    
private:
    float value_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    bool horizontal_ = true;
    bool isDragging_ = false;
    OnValueChangedCallback onValueChanged_;
};

// ============================================================================
// UIImage - изображение
// ============================================================================

class UIImage : public UIPanel {
public:
    explicit UIImage(const std::string& name = "Image");
    
    bool LoadImage(const std::string& filename);
    void UnloadImage();
    
    void SetColor(const Color32& color) { color_ = color; }
    const Color32& GetColor() const { return color_; }
    
    void Render() override;
    
private:
    std::string imageFile_;
    Color32 color_ = Color32::White;
    void* textureHandle_ = nullptr;
};

// ============================================================================
// UICanvas - корневой холст UI
// ============================================================================

class UICanvas {
public:
    UICanvas();
    ~UICanvas();
    
    // Добавление корневых элементов
    void AddRoot(std::unique_ptr<UIBase> element);
    void RemoveRoot(UIBase* element);
    
    // Обновление и рендер
    void Update(float deltaTime);
    void Render();
    
    // Ввод
    bool HandleMouseDown(float x, float y, int button);
    bool HandleMouseUp(float x, float y, int button);
    bool HandleMouseMove(float x, float y);
    bool HandleKeyDown(int key);
    bool HandleKeyUp(int key);
    bool HandleCharTyped(unsigned int c);
    
    // Найти элемент по имени (рекурсивно)
    UIBase* FindElementByName(const std::string& name) const;
    
    // Настройки canvas
    void SetScreenSize(float width, float height);
    float GetScreenWidth() const { return screenWidth_; }
    float GetScreenHeight() const { return screenHeight_; }
    
    // Sort order (для наслоения)
    void SetSortOrder(int order) { sortOrder_ = order; }
    int GetSortOrder() const { return sortOrder_; }
    
    // Видимость всего canvas
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    std::vector<std::unique_ptr<UIBase>> roots_;
    float screenWidth_ = 1920.0f;
    float screenHeight_ = 1080.0f;
    int sortOrder_ = 0;
    bool enabled_ = true;
    
    UIBase* hoveredElement_ = nullptr;
    UIBase* pressedElement_ = nullptr;
};

// ============================================================================
// UIComponent - компонент для Actor-based UI
// ============================================================================

class UIComponent : public Component {
public:
    EOA_CLASS_DECL(UIComponent, Component)
    
    explicit UIComponent(const std::string& name = "UI");
    ~UIComponent() override;
    
    UICanvas* GetCanvas() { return &canvas_; }
    const UICanvas* GetCanvas() const { return &canvas_; }
    
    void Tick(float deltaTime) override;
    
private:
    UICanvas canvas_;
};

} // namespace eoa
