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
		GraphRenderingComponent(Actor& actor, Component* parentComp, const std::string name, const Transform& transform = Transform());
		
		void PushRawMarker(const Vec2f&);
		void PopFrontMarker();

		Mat4f GetGraphViewMatrix() const;
		std::vector<Vec2f>& GetMarkersUnitInterval();
		const std::vector<Vec2f>& GetMarkersUnitInterval() const;
		virtual std::vector<const Material*> GetMaterials() const;

		void SetGraphView(const Transform& graphView);


		virtual void Render(const RenderInfo&, Shader*) override;
	protected:
		virtual unsigned int GetUIDepth() const override;
	private:
		void UpdateMarkerUniformData(Shader&);

		/**
		 * @brief Contains graph markers with appropiate positions in the unit interval [0, 1]
		 * @note This should be a vector, since we need a contiguous memory container (to send the markers to a shader).
		*/
		std::vector<Vec2f> GraphMarkersUnitInterval;

		Transform GraphView;

		SharedPtr<MaterialInstance> GraphMaterialInst;

		float LASTUPDATE;
		unsigned int FRAMESELAPSED;
	};
}