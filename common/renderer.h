//--------------------------------------------
//
// レンダラーインターフェース [renderer.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "graphics_types.h" // Vertex3D
#include <span>
#include <functional>

#undef near

class Renderer;
class Window;

namespace gui
{
    void init(const Window& window, const Renderer& renderer);
}

class RendererImpl;
class TextureManager;
class Scene;
struct TextureData;

struct ID3D11Device;
struct ID3D11DeviceContext;

//----------------------------
// レンダラー (外部インターフェース)
//----------------------------
class Renderer
{
public:
    Renderer();
    ~Renderer();

    void init(HWND handle, long width, long height);
    void uninit();
    bool render(const Scene& scene, std::function<void()> guiRender = {});

    MeshHandle createMesh(VertexShaderType type, const void* vertices, size_t verticesCount, const void* indices, size_t indicesCount);
    bool uploadTextures(const TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback = {});

    void setPostProcessShaderMask(PostProcessShaderMask mask);
    void setToneMappingType(ToneMappingType type);
    void setShadowMapResolution(int resolution);
    void setShadowMapArea(float width, float height, float near, float fur);
    void setAmbient(const Color& ambient);
    bool setFog(const FogData& fog);

    bool setMesh(const MeshHandle& handle);
    bool setTexture(const TextureHandle& handle);
    bool setTransformWorld(const Matrix& matrix);
    bool setMaterial(const Material& material);
    bool setBoneTransforms(std::span<const Matrix> boneTransforms);
    void setOutlineData(OutlineData data);

    bool drawMesh(const MeshHandle& handle);
    bool drawIndexedPrimitive(VertexShaderType vertexShaderType, int indexCount, unsigned int startIndexLocation, unsigned int baseVertexLocation);
    void drawDecal(Matrix transform, const MeshHandle& handle, Color color);
    void drawString(std::string_view string, Vector2 pos = { 0,0 }, Color color = Color::White(), float angle = 0.0f, Vector2 scale = { 1,1 });

    void onResize(int width, int height);

    HWND getRegisteredHWND() const;
    void getScreenSizeMagnification(Vector2& magnification) const;
    void getViewportSize(Vector2& size) const;

private:
    // ↓ friend Gui
    friend void gui::init(const Window& window, const Renderer& renderer);
    ID3D11Device* getDevice() const;
    ID3D11DeviceContext* getContext() const;
    // ↑
    
    std::unique_ptr<RendererImpl> m_pImpl;
};
