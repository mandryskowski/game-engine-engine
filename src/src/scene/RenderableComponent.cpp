#include <scene/RenderableComponent.h>
#include <game/GameScene.h>
#include <UI/UICanvasField.h>
#include <UI/UICanvasActor.h>
#include <utility/Serialisation.h>
namespace GEE
{
    void RenderableComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
    {
        Component::GetEditorDescription(descBuilder);

        descBuilder.AddField("Hide").GetTemplates().TickBox([this](bool flag) { SetHide(flag); }, [this]() {return GetHide(); });
        descBuilder.AddField("Casts shadow").GetTemplates().TickBox(bCastsShadow);
    }

    Renderable::Renderable(GameScene& scene) : Hide(false), bCastsShadow(true), SceneRenderData(*scene.GetRenderData())
    {
	    SceneRenderData.AddRenderable(*this);
    }

    Renderable::Renderable(Renderable&& renderable) : Hide(renderable.Hide), bCastsShadow(renderable.bCastsShadow),
                                                      SceneRenderData(renderable.SceneRenderData)
    {
	    SceneRenderData.AddRenderable(*this);
    }
    bool Renderable::GetHide() const { return Hide; }
    void Renderable::SetHide(bool hide) { Hide = hide; }
    bool Renderable::CastsShadow() const { return bCastsShadow; }
    void Renderable::SetCastsShadow(bool castsShadow) { bCastsShadow = castsShadow; }
    std::vector<const Material*> Renderable::GetMaterials() const { return { nullptr }; }
    template <typename Archive>
    void RenderableComponent::Save(Archive& archive) const
    {
        archive(cereal::make_nvp("Hide", GetHide()), cereal::make_nvp("CastsShadow", CastsShadow()), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
    }
    template <typename Archive>
    void RenderableComponent::Load(Archive& archive)
    {
        bool hide = false, castsShadow = true;
        archive(cereal::make_nvp("Hide", hide), cereal::make_nvp("CastsShadow", castsShadow), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
        SetHide(hide);
        SetCastsShadow(castsShadow);
    }
    Renderable::~Renderable() { SceneRenderData.EraseRenderable(*this); }
}
GEE_SERIALIZATION_INST_DEFAULT(GEE::RenderableComponent);