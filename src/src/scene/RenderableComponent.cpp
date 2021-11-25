#include <scene/RenderableComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UICanvasActor.h>
namespace GEE
{
    void RenderableComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
    {
        Component::GetEditorDescription(descBuilder);

        descBuilder.AddField("Hide").GetTemplates().TickBox([this](bool flag) { SetHide(flag); }, [this]() {return GetHide(); });
        descBuilder.AddField("Casts shadow").GetTemplates().TickBox(bCastsShadow);
    }
}