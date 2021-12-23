#include <editor/GraphRenderingComponent.h>
#include <UI/UICanvas.h>
#include <scene/TextComponent.h>
#include <numeric>

#include <rendering/Renderer.h>

namespace GEE
{
	GraphRenderingComponent::GraphRenderingComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform):
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp)
	{
		if (SharedPtr<Material> foundMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Default_Graph_Material"))
			GraphMaterialInst = MakeShared<MaterialInstance>(foundMaterial);
		else
		{
			auto material = GameHandle->GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_Graph_Material", *GameHandle->GetRenderEngineHandle()->FindShader("GraphShader")));
			GraphMaterialInst = MakeShared<MaterialInstance>(material);
		}
	}
	void GraphRenderingComponent::PushRawMarker(const Vec2f& rawMarker)
	{
		GraphMarkersUnitInterval.push_back(MarkerToUnitInterval(rawMarker));
	}
	void GraphRenderingComponent::PopFrontMarker()
	{
		if (!GraphMarkersUnitInterval.empty())
			GraphMarkersUnitInterval.erase(GraphMarkersUnitInterval.begin());
	}
	float GraphRenderingComponent::GetMinValueInRange(const GraphRange& range)
	{
		// Find the first graph marker that is in range
		auto it = std::lower_bound(GraphMarkersUnitInterval.begin(), GraphMarkersUnitInterval.end(), range.GetRangeBeginEnd().x, [](const Vec2f& marker, float beginX) { return marker.x <= beginX; });
		// Find the last graph marker that is in range
		auto rangeEnd = std::lower_bound(GraphMarkersUnitInterval.begin(), GraphMarkersUnitInterval.end(), range.GetRangeBeginEnd().y, [](const Vec2f& marker, float endX) { return marker.x <= endX; });
		float minValue = std::numeric_limits<float>().max();

		if (it > rangeEnd)	// If array is unsorted
			return minValue;

		for (; it != rangeEnd; it++)	// Find the smallest marker between it and rangeEnd
			if (it->y < minValue)
				minValue = it->y;

		if (minValue == std::numeric_limits<float>().max())	// Do not convert max float (just return it instead)
			return minValue;
		return MarkerToRaw(Vec2f(0.0f, minValue)).y;
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
	GraphRenderingComponent::GraphRange& GraphRenderingComponent::GetSelectedRange()
	{
		return SelectedRange;
	}
	const GraphRenderingComponent::GraphRange& GraphRenderingComponent::GetSelectedRange() const
	{
		return SelectedRange;
	}
	Vec2f GraphRenderingComponent::MarkerToRaw(const Vec2f& unitIntervalMarker) const
	{
		return static_cast<Vec2f>(glm::inverse(GetGraphViewMatrix()) * Vec4f(unitIntervalMarker, 0.0f, 1.0f));
	}
	Vec2f GraphRenderingComponent::MarkerToUnitInterval(const Vec2f& rawMarker) const
	{
		return static_cast<Vec2f>(GetGraphViewMatrix() * Vec4f(rawMarker, 0.0f, 1.0f));
	}
	void GraphRenderingComponent::SetGraphView(const Transform& graphView)
	{
		// Transform all markers from previous graph view space to current graph view space.
		for (auto& it : GraphMarkersUnitInterval)
			it = glm::inverse(graphView.GetMatrix()) * GraphView.GetMatrix() * static_cast<Vec4f>(it, 0.0f, 1.0f);

		GraphView = graphView;
	}
	void GraphRenderingComponent::Render(const SceneMatrixInfo& info, Shader* shader)
	{
		if (GetHide())
			return;

		if (info.GetRequiredShaderInfo().IsValid() && shader != GameHandle->GetRenderEngineHandle()->FindShader("GraphShader"))
			return;

		UpdateMarkerUniformData(*shader);
		
		
		Renderer(*GameHandle->GetRenderEngineHandle()).StaticMeshInstances((CanvasPtr) ? (MatrixInfoExt(info.GetContextID(), CanvasPtr->BindForRender(info))) : (info), { MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), GraphMaterialInst) }, GetTransform().GetWorldTransform(), *shader);

		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
	}
	Vec2f GraphRenderingComponent::GraphRange::GetRangeBeginEnd() const
	{
		return Vec2f(glm::min(EndpointsX.x, EndpointsX.y), glm::max(EndpointsX.x, EndpointsX.y));
	}
	void GraphRenderingComponent::GraphRange::SetEndpoint1(float unitIntervalEndpoint)
	{
		EndpointsX.x = unitIntervalEndpoint;
	}
	void GraphRenderingComponent::GraphRange::SetEndpoint2(float unitIntervalEndpoint)
	{
		EndpointsX.y = unitIntervalEndpoint;
	}
	void GraphRenderingComponent::GraphRange::SetEndpoints(const Vec2f& unitIntervalEndpoints)
	{
		EndpointsX = unitIntervalEndpoints;
	}
	unsigned int GraphRenderingComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}
	void GraphRenderingComponent::UpdateMarkerUniformData(Shader& shader)
	{
		shader.Uniform("dataPointsCount", static_cast<int>(GraphMarkersUnitInterval.size()));
		if (!GraphMarkersUnitInterval.empty())
			shader.UniformArray("dataPoints", &GraphMarkersUnitInterval[0], GraphMarkersUnitInterval.size());
		shader.Uniform("selectedRangeEndpoints", GetSelectedRange().GetRangeBeginEnd());
	}

	
	FPSGraphRenderingComponent::FPSGraphRenderingComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform):
		GraphRenderingComponent(actor, parentComp, name, transform),
		PrevFPSUpdateTime(GameHandle->GetProgramRuntime()),
		PrevFPSUpdateFrame(GameHandle->GetTotalFrameCount()),
		MinFPSText(nullptr),
		MaxFPSText(nullptr),
		AvgFPSText(nullptr)
	{
		Shader* textShader = GameHandle->GetRenderEngineHandle()->FindShader("TextShader");

		MinFPSText = &CreateComponent<TextComponent>("MinFPSText", Transform(Vec2f(1.5f, -0.5f), Vec2f(0.1f)), "No data", "", Alignment2D::Center());
		auto redMaterial = MakeShared<Material>("Red", Vec3f(1.0f, 0.155f, 0.25f), *textShader);
		MinFPSText->SetMaterialInst(GameHandle->GetRenderEngineHandle()->AddMaterial(redMaterial));

		MaxFPSText = &CreateComponent<TextComponent>("MaxFPSText", Transform(Vec2f(1.5f, 0.5f), Vec2f(0.1f)), "No data", "", Alignment2D::Center());
		auto greenMaterial = MakeShared<Material>("Green", Vec3f(0.155f, 1.0f, 0.25f), *textShader);
		MaxFPSText->SetMaterialInst(GameHandle->GetRenderEngineHandle()->AddMaterial(greenMaterial));

		AvgFPSText = &CreateComponent<TextComponent>("AvgFPSText", Transform(Vec2f(1.5f, 0.0f), Vec2f(0.1f)), "No data", "", Alignment2D::Center());
		auto coralMaterial = MakeShared<Material>("Coral", Vec3f(1.0f, 0.5f, 0.31f), *textShader);
		AvgFPSText->SetMaterialInst(GameHandle->GetRenderEngineHandle()->AddMaterial(coralMaterial));
	}

	void FPSGraphRenderingComponent::Update(float deltaTime)
	{
		GraphRenderingComponent::Update(deltaTime);

		unsigned int currentFrame = GameHandle->GetTotalFrameCount();
		float time = GameHandle->GetProgramRuntime();

		// Every second update graph
		if (time >= PrevFPSUpdateTime + 1.0f)
		{
			for (auto& it : GetMarkersUnitInterval())	// move each marker by -1 second on the X axis
				it.x -= 1.0f / 30.0f;

			PushRawMarker(Vec2f(30.0f, static_cast<float>(currentFrame - PrevFPSUpdateFrame) / (time - PrevFPSUpdateTime)));
			if (GetMarkersUnitInterval().size() > 31)
				PopFrontMarker();

			PrevFPSUpdateTime = time;
			PrevFPSUpdateFrame = currentFrame;

			UpdateFPSTextInfo();
		}
	}

	void FPSGraphRenderingComponent::UpdateFPSTextInfo()
	{
		float minVal = GetRangeStatistic([](std::vector<Vec2f>::iterator& begin, std::vector<Vec2f>::iterator& end) -> float {
			auto min = std::min_element(begin, end, [](const Vec2f& lhs, const Vec2f& rhs) -> bool { return lhs.y < rhs.y; });
			return (min != end) ? (min->y) : (std::numeric_limits<float>().max());
		});
		MinFPSText->SetContent((minVal != std::numeric_limits<float>().max()) ? ("Min:" + ToStringPrecision(minVal, 2)) : ("No data"));

		float maxVal = GetRangeStatistic([](std::vector<Vec2f>::iterator& begin, std::vector<Vec2f>::iterator& end) -> float {
			auto max = std::max_element(begin, end, [](const Vec2f& lhs, const Vec2f& rhs) -> bool { return lhs.y < rhs.y; });
			return (max != end) ? (max->y) : (std::numeric_limits<float>().max());
		});
		MaxFPSText->SetContent((maxVal != std::numeric_limits<float>().max()) ? ("Max:" + ToStringPrecision(maxVal, 2)) : ("No data"));

		float avgVal = GetRangeStatistic([](std::vector<Vec2f>::iterator& begin, std::vector<Vec2f>::iterator& end) -> float {
			float avg = std::accumulate(begin, end, Vec2f(0.0f)).y;
			return (static_cast<int>(end - begin) > 0) ? (avg / static_cast<float>(end - begin)) : (std::numeric_limits<float>().max());
			});
		AvgFPSText->SetContent((avgVal != std::numeric_limits<float>().max()) ? ("Avg:" + ToStringPrecision(avgVal, 2)) : ("No data"));
	}

}