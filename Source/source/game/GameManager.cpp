#include <game/GameManager.h>

GameScene* GameManager::DefaultScene = nullptr;

template <> void EditorManager::Select<Component>(Component* obj, GameScene& editorScene) { SelectComponent(obj, editorScene); }
template <> void EditorManager::Select<Actor>(Actor* obj, GameScene& editorScene) { SelectActor(obj, editorScene); }
template <> void EditorManager::Select<GameScene>(GameScene* obj, GameScene& editorScene) { SelectScene(obj, editorScene); }
