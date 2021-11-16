#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>

namespace GEE
{
	class UIAutomaticListActor;

	enum class Alignment
	{
		Left,
		Center,
		Right,
		Bottom = Left,
		Top = Right
	};

	class IconData
	{
	public:
		/**
		 * @brief Construct an invalid IconData object.
		*/
		IconData() : IconMaterial(nullptr), IconAtlasID(-1.0f) {}

		IconData(RenderEngineManager& renderHandle, const std::string& texFilepath, Vec2i iconAtlasSize = Vec2i(0), float iconAtlasID = -1.0f);
		bool IsValid() const;
		SharedPtr<MaterialInstance> GetMatInstance() const;
	private:
		SharedPtr<Material> IconMaterial;
		float IconAtlasID;
	};

	class Alignment2D
	{
	public:
		Alignment2D(Alignment horizontal, Alignment vertical = Alignment::Center);

		Alignment GetHorizontal() const;
		Alignment GetVertical() const;
	private:
		Alignment Horizontal, Vertical;
	};

	class PopupDescription
	{
	public:
		PopupDescription(SystemWindow& window, UIAutomaticListActor*, UICanvasActor* canvas);
		void AddOption(const std::string&, std::function<void()>, const IconData& = IconData());
		PopupDescription AddSubmenu(const std::string&, std::function<void(PopupDescription)> prepareSubmenu, const IconData& = IconData());
		Transform GetView() { if (!bViewComputed) ComputeView(); return LastComputedView; }
		/**
		 * @brief Should be called after adding all the options and submenus.
		*/
		void RefreshPopup();
	private:
		void ComputeView();

		SystemWindow& Window;
		UIAutomaticListActor* DescriptionRoot;
		UICanvasActor* PopupCanvas;
		bool bViewComputed;
		Transform LastComputedView;
		Vec2i PopupOptionSizePx;
	};

	namespace Editor
	{
		class EditorActions
		{
		public:
			void SelectActor(Actor* actor, GameScene& editorScene);
			void SelectScene(GameScene* selectedScene, GameScene& editorScene);

			void CreateComponentWindow(Actor&);
		};
	}
}