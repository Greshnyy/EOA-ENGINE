#pragma once

#include "Core.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Resources/Resource.h"
#include <vector>
#include <string>
#include <memory>

namespace EOA {

// Описание вершины (PBR стандарт)
struct Vertex {
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoord;
    Vector3 Tangent;
    Vector3 Bitangent;
};

// Описание буфера
struct BufferDesc {
    size_t Size = 0;
    bool IsDynamic = false;
    bool IsIndexBuffer = false;
};

// Описание текстуры
struct TextureDesc {
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t MipLevels = 1;
    bool IsCubeMap = false;
    bool IsRenderTarget = false;
};

// Базовый класс Ресурса GPU
class EOA_API GPUResource : public Resource {
public:
    virtual ~GPUResource() = default;
    virtual void* GetNativeHandle() const = 0;
    virtual size_t GetMemoryUsage() const = 0;
};

// Буфер (Vertex/Index/Uniform)
class EOA_API Buffer : public GPUResource {
public:
    virtual void SetData(const void* data, size_t size, size_t offset = 0) = 0;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
    
    const BufferDesc& GetDesc() const { return Desc; }
    
protected:
    BufferDesc Desc;
};

// Текстура
class EOA_API Texture : public GPUResource {
public:
    virtual void UpdateRegion(const void* data, uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
    virtual void GenerateMips() = 0;
    
    const TextureDesc& GetDesc() const { return Desc; }
    uint32_t GetWidth() const { return Desc.Width; }
    uint32_t GetHeight() const { return Desc.Height; }
    
protected:
    TextureDesc Desc;
};

// Шейдер
class EOA_API Shader : public GPUResource {
public:
    enum class Type { Vertex, Fragment, Compute, Geometry, TessellationControl, TessellationEvaluation };
    
    virtual bool CompileFromFile(const std::string& path, Type type) = 0;
    virtual bool CompileFromSource(const std::string& source, Type type) = 0;
    
    // Связывание параметров (Uniforms)
    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetVec3(const std::string& name, const Vector3& value) = 0;
    virtual void SetMat4(const std::string& name, const Matrix4& value) = 0;
    virtual void SetTexture(const std::string& name, const Texture* texture, uint32_t unit) = 0;
};

// Mesh (Набор буферов)
class EOA_API Mesh : public GPUResource {
public:
    void SetVertices(const std::vector<Vertex>& vertices);
    void SetIndices(const std::vector<uint32_t>& indices);
    
    virtual void Draw() = 0;
    virtual void DrawInstanced(uint32_t count) = 0;
    
    uint32_t GetVertexCount() const { return (uint32_t)Vertices.size(); }
    uint32_t GetIndexCount() const { return (uint32_t)Indices.size(); }
    
protected:
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    std::unique_ptr<Buffer> VertexBuffer;
    std::unique_ptr<Buffer> IndexBuffer;
};

// Камера (для рендер пайплайна)
struct CameraData {
    Matrix4 ViewMatrix;
    Matrix4 ProjectionMatrix;
    Vector3 Position;
    float NearPlane;
    float FarPlane;
    float FOV;
};

// Интерфейс Рендерера
class EOA_API IRenderer {
public:
    virtual ~IRenderer() = default;
    
    virtual bool Initialize(void* windowHandle, int width, int height) = 0;
    virtual void Shutdown() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    virtual void SetClearColor(const Vector4& color) = 0;
    virtual void Clear(bool color, bool depth, bool stencil) = 0;
    
    // Создание ресурсов
    virtual std::unique_ptr<Buffer> CreateBuffer(const BufferDesc& desc) = 0;
    virtual std::unique_ptr<Texture> CreateTexture(const TextureDesc& desc) = 0;
    virtual std::unique_ptr<Shader> CreateShader() = 0;
    virtual std::unique_ptr<Mesh> CreateMesh() = 0;
    
    // Рендеринг
    virtual void DrawMesh(const Mesh* mesh, const Shader* shader, const CameraData& camera, const Matrix4& model) = 0;
    
    // Статистика
    struct Stats {
        uint32_t DrawCalls;
        uint32_t TriangleCount;
        uint32_t VertexCount;
        float GPUMemoryMB;
    };
    virtual Stats GetStats() const = 0;
};

} // namespace EOA
