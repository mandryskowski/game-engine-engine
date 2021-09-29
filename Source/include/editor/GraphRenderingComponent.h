#pragma once
#include <scene/RenderableComponent.h>
#include <UI/UIComponent.h>

namespace GEE
{
	/**
	 * @brief Used to store graph data and render it on a screen.
	 * @note Unit interval value - A value between 0 and 1.
	 * @note Raw graph marker - A 2D vector which can have any value (since it is not yet converted to unit interval).
	*/
	class GraphRenderingComponent : public RenderableComponent, public UIComponent
	{
	public:
		struct GraphRange;

		GraphRenderingComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform = Transform());
		
		void PushRawMarker(const Vec2f&);
		void PopFrontMarker();

		/**
		 * @brief Get a statistic based on values in the given range (or selected range by default). The markers must be sorted when this funciton is called.
		 * @tparam BinaryFunction: A function accepting the begin and end iterator of values which match the range. Operate on unit interval values!
		 * @param optionalRange: Optionally, pass your own range. Leave it as it is to use the range which is currently selected by the user.
		 * @return: The statistic (a float value) as a raw value (not unit interval). Returns max float if the array isn't sorted.
		*/
		template <typename BinaryFunction>
		float GetRangeStatistic(BinaryFunction&& statistic, GraphRange optionalRange = Vec2f(-1.0f));
		float GetMinValueInRange(const GraphRange&);
		Mat4f GetGraphViewMatrix() const;
		std::vector<Vec2f>& GetMarkersUnitInterval();
		const std::vector<Vec2f>& GetMarkersUnitInterval() const;
		virtual std::vector<const Material*> GetMaterials() const;
		GraphRange& GetSelectedRange();
		const GraphRange& GetSelectedRange() const;

		Vec2f MarkerToRaw(const Vec2f& unitIntervalMarker) const;
		Vec2f MarkerToUnitInterval(const Vec2f& rawMarker) const;

		void SetGraphView(const Transform& graphView);


		virtual void Render(const RenderInfo&, Shader*) override;


		struct GraphRange
		{
			GraphRange(Vec2f endpoints = Vec2f(0.0f)) : EndpointsX(endpoints) {}

			/**
			 * @return A 2D vector where the X component is the smaller x coordinate of the endpoints (range begin) and the Y component is the bigger x coordinate of the endpoints (range end).
			*/
			Vec2f GetRangeBeginEnd() const;

			void SetEndpoint1(float unitIntervalEndpoint);
			void SetEndpoint2(float unitIntervalEndpoint);

			void SetEndpoints(const Vec2f& unitIntervalEndpoints);


		private:
			Vec2f EndpointsX;
		};
	protected:
		virtual unsigned int GetUIDepth() const override;
	private:
		void UpdateMarkerUniformData(Shader&);

		/**
		 * @brief Contains graph markers with appropiate positions in the unit interval [0, 1]
		 * @note This should be a vector, since we need a contiguous memory container (to send the markers to a shader).
		*/
		std::vector<Vec2f> GraphMarkersUnitInterval;

		GraphRange SelectedRange;

		Transform GraphView;

		SharedPtr<MaterialInstance> GraphMaterialInst;
	};

	class FPSGraphRenderingComponent : public GraphRenderingComponent
	{
	public:
		FPSGraphRenderingComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform = Transform());
		
		virtual void Update(float deltaTime) override;
		void UpdateFPSTextInfo();

	private:
		float PrevFPSUpdateTime;
		unsigned long long PrevFPSUpdateFrame;

		TextComponent *MinFPSText, *MaxFPSText, *AvgFPSText;
	};

	template<typename BinaryFunction>
	inline float GraphRenderingComponent::GetRangeStatistic(BinaryFunction&& getStatistic, GraphRange range)
	{
		if (range.GetRangeBeginEnd() == Vec2f(-1.0f))
			range = GetSelectedRange();

		// Find the first graph marker that is in range
		auto it = std::lower_bound(GraphMarkersUnitInterval.begin(), GraphMarkersUnitInterval.end(), range.GetRangeBeginEnd().x, [](const Vec2f& marker, float beginX) { return marker.x <= beginX; });
		// Find the last graph marker that is in range
		auto rangeEnd = std::lower_bound(GraphMarkersUnitInterval.begin(), GraphMarkersUnitInterval.end(), range.GetRangeBeginEnd().y, [](const Vec2f& marker, float endX) { return marker.x <= endX; });

		if (it > rangeEnd)	// If array is unsorted
			return std::numeric_limits<float>().max();
		
		float statistic = getStatistic(it, rangeEnd);
		if (statistic == std::numeric_limits<float>().max())	// Do not convert max float (just return it instead)
			return statistic;
		return MarkerToRaw(Vec2f(0.0f, statistic)).y;
	}
}