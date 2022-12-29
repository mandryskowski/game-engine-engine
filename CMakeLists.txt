cmake_minimum_required (VERSION 3.2)

project (game-engine-engine C CXX)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "LibraryDebug"
    "LibraryRelease"
    "Release"
    CACHE STRING "" FORCE
)

if (MSVC)
  add_compile_options(/bigobj /MP )
else ()
  add_compile_options(-Wa,-mbig-obj)
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_subdirectory(../rehab-game ${CMAKE_BINARY_DIR}/rehab-game)
add_subdirectory(Projects/example_8_ball_pool/example_8_ball_pool)

# GEE
set(PROJECT_NAME game-engine-engine)

set(Header_Files
    "src/src/animation/Animation.h"
    "src/src/animation/AnimationManagerComponent.h"
    "src/src/animation/SkeletonInfo.h"
    "src/src/assetload/FileLoader.h"
    "src/src/audio/AudioEngine.h"
    "src/src/audio/AudioFile.h"
    "src/src/editor/DefaultEditorController.h"
    "src/src/editor/EditorActions.h"
    "src/src/editor/EditorManager.h"
    "src/src/editor/EditorMessageLogger.h"
    "src/src/editor/EditorRenderer.h"
    "src/src/editor/FPSCounter.h"
    "src/src/editor/GameEngineEngineEditor.h"
    "src/src/editor/GEditorSettings.h"
    "src/src/editor/GEditorToolbox.h"
    "src/src/editor/GraphRenderingComponent.h"
    "src/src/editor/MousePicking.h"
    "src/src/editor/UIViewportWindow.h"
    "src/src/game/Game.h"
    "src/src/game/GameManager.h"
    "src/src/game/GameScene.h"
    "src/src/game/GameSettings.h"
    "src/src/game/IDSystem.h"
    "src/src/input/Event.h"
    "src/src/input/InputDevicesStateRetriever.h"
    "src/src/math/Box.h"
    "src/src/math/Transform.h"
    "src/src/math/Vec.h"
    "src/src/physics/CollisionObject.h"
    "src/src/physics/DynamicPhysicsObjects.h"
    "src/src/physics/PhysicsEngine.h"
    "src/src/physics/PhysicsObjects.h"
    "src/src/rendering/Framebuffer.h"
    "src/src/rendering/LightProbe.h"
    "src/src/rendering/Material.h"
    "src/src/rendering/Mesh.h"
    "src/src/rendering/OutlineRenderer.h"
    "src/src/rendering/Postprocess.h"
    "src/src/rendering/RenderableVolume.h"
    "src/src/rendering/RenderEngine.h"
    "src/src/rendering/Renderer.h"
    "src/src/rendering/RenderInfo.h"
    "src/src/rendering/RenderToolbox.h"
    "src/src/rendering/Shader.h"
    "src/src/rendering/Texture.h"
    "src/src/rendering/Viewport.h"
    "src/src/scene/Actor.h"
    "src/src/scene/BoneComponent.h"
    "src/src/scene/CameraComponent.h"
    "src/src/scene/Component.h"
    "src/src/scene/Controller.h"
    "src/src/scene/GunActor.h"
    "src/src/scene/hierarchy/HierarchyNode.h"
    "src/src/scene/hierarchy/HierarchyNodeBase.h"
    "src/src/scene/hierarchy/HierarchyNodeInstantiation.h"
    "src/src/scene/hierarchy/HierarchyTree.h"
    "src/src/scene/LightComponent.h"
    "src/src/scene/LightProbeComponent.h"
    "src/src/scene/ModelComponent.h"
    "src/src/scene/PawnActor.h"
    "src/src/scene/RenderableComponent.h"
    "src/src/scene/SoundSourceComponent.h"
    "src/src/scene/TextComponent.h"
    "src/src/scene/UIButtonActor.h"
    "src/src/scene/UIInputBoxActor.h"
    "src/src/scene/UIListElement.h"
    "src/src/scene/UIWindowActor.h"
    "src/src/UI/Font.h"
    "src/src/UI/UIActor.h"
    "src/src/UI/UICanvas.h"
    "src/src/UI/UICanvasActor.h"
    "src/src/UI/UICanvasField.h"
    "src/src/UI/UIComponent.h"
    "src/src/UI/UIElement.h"
    "src/src/UI/UIListActor.h"
    "src/src/utility/Alignment.h"
    "src/src/utility/Asserts.h"
    "src/src/utility/Log.h"
    "src/src/utility/OperatingSystem.h"
    "src/src/utility/Profiling.h"
    "src/src/utility/Utility.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "src/src/animation/Animation.cpp"
    "src/src/animation/AnimationManagerComponent.cpp"
    "src/src/animation/SkeletonInfo.cpp"
    "src/src/assetload/FileLoader.cpp"
    "src/src/audio/AudioEngine.cpp"
    "src/src/editor/DefaultEditorController.cpp"
    "src/src/editor/EditorActions.cpp"
    "src/src/editor/EditorManager.cpp"
    "src/src/editor/EditorMessageLogger.cpp"
    "src/src/editor/EditorRenderer.cpp"
    "src/src/editor/GameEngineEngineEditor.cpp"
    "src/src/editor/GEditorSettings.cpp"
    "src/src/editor/GEditorToolbox.cpp"
    "src/src/editor/GraphRenderingComponent.cpp"
    "src/src/editor/MousePicking.cpp"
    "src/src/game/Game.cpp"
    "src/src/game/GameManager.cpp"
    "src/src/game/GameScene.cpp"
    "src/src/game/GameSettings.cpp"
    "src/src/input/Event.cpp"
    "src/src/input/InputDevicesStateRetriever.cpp"
    "src/src/main.cpp"
    "src/src/math/Box.cpp"
    "src/src/math/Transform.cpp"
    "src/src/math/Vec.cpp"
    "src/src/physics/CollisionObject.cpp"
    "src/src/physics/DynamicPhysicsObjects.cpp"
    "src/src/physics/PhysicsEngine.cpp"
    "src/src/physics/PhysicsObjects.cpp"
    "src/src/rendering/Framebuffer.cpp"
    "src/src/rendering/LightProbe.cpp"
    "src/src/rendering/Material.cpp"
    "src/src/rendering/Mesh.cpp"
    "src/src/rendering/Postprocess.cpp"
    "src/src/rendering/RenderableVolume.cpp"
    "src/src/rendering/RenderEngine.cpp"
    "src/src/rendering/Renderer.cpp"
    "src/src/rendering/RenderInfo.cpp"
    "src/src/rendering/RenderToolbox.cpp"
    "src/src/rendering/Shader.cpp"
    "src/src/rendering/Texture.cpp"
    "src/src/rendering/Viewport.cpp"
    "src/src/scene/Actor.cpp"
    "src/src/scene/BoneComponent.cpp"
    "src/src/scene/CameraComponent.cpp"
    "src/src/scene/Component.cpp"
    "src/src/scene/Controller.cpp"
    "src/src/scene/GunActor.cpp"
    "src/src/scene/hierarchy/HierarchyNode.cpp"
    "src/src/scene/hierarchy/HierarchyNodeInstantiation.cpp"
    "src/src/scene/hierarchy/HierarchyTree.cpp"
    "src/src/scene/LightComponent.cpp"
    "src/src/scene/LightProbeComponent.cpp"
    "src/src/scene/ModelComponent.cpp"
    "src/src/scene/PawnActor.cpp"
    "src/src/scene/RenderableComponent.cpp"
    "src/src/scene/SoundSourceComponent.cpp"
    "src/src/scene/TextComponent.cpp"
    "src/src/scene/UIButtonActor.cpp"
    "src/src/scene/UIInputBoxActor.cpp"
    "src/src/scene/UIListElement.cpp"
    "src/src/scene/UIWindowActor.cpp"
    "src/src/UI/Font.cpp"
    "src/src/UI/UIActor.cpp"
    "src/src/UI/UICanvas.cpp"
    "src/src/UI/UICanvasActor.cpp"
    "src/src/UI/UICanvasField.cpp"
    "src/src/UI/UIComponent.cpp"
    "src/src/UI/UIElement.cpp"
    "src/src/UI/UIListActor.cpp"
    "src/src/utility/Alignment.cpp"
    "src/src/utility/Profiling.cpp"
    "src/src/utility/Utility.cpp"
    "src/vendor/source/glad/glad.c"
    "src/vendor/source/tinyfiledialogs/tinyfiledialogs.c"
    "src/vendor/source/whereami/whereami.c"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

set(LibrariesDebug
    "x64/glfw/debug/glfw3;"
    "x64/OpenAL/debug/OpenAL64;"
    "x64/PhysX/debug/PhysX_64;"
    "x64/PhysX/debug/PhysXCommon_64;"
    "x64/PhysX/debug/PhysXFoundation_64;"
    "x64/PhysX/debug/PhysXPvdSDK_static_64;"
    "x64/PhysX/debug/PhysXExtensions_static_64;"
    "x64/PhysX/debug/PhysXCharacterKinematic_static_64;"
    "x64/PhysX/debug/PhysXCooking_64;"
    "x64/assimp/debug/assimp-vc143-mtd;"
    "x64/freetype/debug/freetyped"
    "opengl32")
source_group("LibrariesDebug" FILES ${LibrariesDebug})

add_executable(${PROJECT_NAME} ${ALL_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC src/src)
target_include_directories(${PROJECT_NAME} PUBLIC src/vendor/include)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LibrariesDebug})
target_link_directories(${PROJECT_NAME} PUBLIC src/vendor/lib)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/Settings.ini
)