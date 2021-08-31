#include <editor/GraphRenderingComponent.h>
#include <UI/UICanvas.h>

namespace GEE
{
	GraphRenderingComponent::GraphRenderingComponent(Actor& actor, Component* parentComp, const std::string name, const Transform& transform):
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		LASTUPDATE(0.0f),
		FRAMESELAPSED(0)
	{
		if (SharedPtr<Material> foundMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Default_Graph_Material"))
			GraphMaterialInst = MakeShared<MaterialInstance>(*foundMaterial);
		else
		{
			Material* material = GameHandle->GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_Graph_Material", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("GraphShader")));
			GraphMaterialInst = MakeShared<MaterialInstance>(*material);
		}
	}
	void GraphRenderingComponent::PushRawMarker(const Vec2f& rawMarker)
	{
		GraphMarkersUnitInterval.push_back(static_cast<Vec2f>(GetGraphViewMatrix() * Vec4f(rawMarker, 0.0f, 1.0f)));
	}
	void GraphRenderingComponent::PopFrontMarker()
	{
		if (!GraphMarkersUnitInterval.empty())
			GraphMarkersUnitInterval.erase(GraphMarkersUnitInterval.begin());
	}
	Mat4f GraphRenderingComponent::GetGraphViewMatrix() const
	{
		return glm::inverse(GraphView.GetMatrix());
	}
	std::vector<Vec2f>& GraphRenderingComponent::GetMarkersUnitInterval()
	{
		return GraphMarkersUnitInterval;
	}
	const std::vector<Vec2f>& GraphRenderingComponent::GetMarkersUnitInterval() const
	{
		return GraphMarkersUnitInterval;
	}
	std::vector<const Material*> GraphRenderingComponent::GetMaterials() const
	{
		if (GraphMaterialInst)
			return { &GraphMaterialInst->GetMaterialRef() };
		return { nullptr };
	}
	void GraphRenderingComponent::SetGraphView(const Transform& graphView)
	{
		// Transform all markers from previous graph view space to current graph view space.
		for (auto& it : GraphMarkersUnitInterval)
			it = glm::inverse(graphView.GetMatrix())* GraphView.GetMatrix()* static_cast<Vec4f>(it, 0.0f, 1.0f);

		GraphView = graphView;
	}
	void GraphRenderingComponent::Render(const RenderInfo& info, Shader* shader)
	{
		if (GetHide())
			return;

		if (glfwGetTime() > LASTUPDATE + 1.0f)
		{
			for (auto& it : GetMarkersUnitInterval())	// move each marker by -1 second on the X axis
				it.x -= 1.0f / 30.0f;
			PushRawMarker(Vec2f(30.0f, (float)FRAMESELAPSED));
			if (GetMarkersUnitInterval().size() > 31)
				PopFrontMarker();
			LASTUPDATE = glfwGetTime();
			FRAMESELAPSED = 0;
		}
		FRAMESELAPSED++;

		UpdateMarkerUniformData(*shader);
		GameHandle->GetRenderEngineHandle()->RenderStaticMesh((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), GraphMaterialInst), GetTransform().GetWorldTransform(), shader);

		if (CanvasPtr)
			CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
	}
	unsigned int GraphRenderingComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}
	void GraphRenderingComponent::UpdateMarkerUniformData(Shader& shader)
	{
		shader.Uniform("dataPointsCount", static_cast<int>(GraphMarkersUnitInterval.size()));
		shader.UniformArray("dataPoints", &GraphMarkersUnitInterval[0], GraphMarkersUnitInterval.size());
	}
}