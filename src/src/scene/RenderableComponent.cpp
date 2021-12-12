#include <scene/RenderableComponent.h>
#include <game/GameScene.h>
#include <UI/UICanvasField.h>
#include <UI/UICanvasActor.h>
namespace GEE
{
    void RenderableComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
    {
        Component::GetEditorDescription(descBuilder);

        descBuilder.AddField("Hide").GetTemplates().TickBox([this](bool flag) { SetHide(flag); }, [this]() {return GetHide(); });
        descBuilder.AddField("Casts shadow").GetTemplates().TickBox(bCastsShadow);
    }
    Renderable::Renderable(GameScene& scene) : SceneRenderData(*scene.GetRenderData()), Hide(false), bCastsShadow(true) { SceneRenderData.AddRenderable(*this); }
    Renderable::Renderable(Renderable&& renderable) : SceneRenderData(renderable.SceneRenderData), Hide(renderable.Hide), bCastsShadow(renderable.bCastsShadow) { SceneRenderData.AddRenderable(*this); }
    bool Renderable::GetHide() const { return Hide; }
    void Renderable::SetHide(bool hide) { Hide = hide; }
    bool Renderable::CastsShadow() const { return bCastsShadow; }
    void Renderable::SetCastsShadow(bool castsShadow) { bCastsShadow = castsShadow; }
    std::vector<const Material*> Renderable::GetMaterials() const { return { nullptr }; }
    Renderable::~Renderable() { SceneRenderData.EraseRenderable(*this); }
}