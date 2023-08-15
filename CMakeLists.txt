cmake_minimum_required(VERSION 3.25)
project(PointClick)

set(CMAKE_CXX_STANDARD 17)

include_directories(external/SDL2/include external)
link_directories(${CMAKE_SOURCE_DIR}/external/SDL2/bin)

add_executable(PointClick src/main.cpp src/Renderer/Camera.hpp src/Renderer/Mesh.hpp src/Renderer/Texture.hpp src/Renderer/FrameBuffer.hpp src/Renderer/Shaders/FragmentShader.hpp src/Renderer/Shaders/VertexShader.hpp src/Renderer/Renderer.hpp src/Renderer/SDL/Window.hpp src/Renderer/Triangle.hpp src/Loaders/TextureLoader.hpp src/Loaders/OBJLoader.hpp src/Loaders/OBJLoader.hpp src/GameState/GameObject.hpp src/Renderer/SkinnedMesh.hpp src/GameState/Pose.hpp src/Loaders/FBXLoader.hpp external/ufbx/ufbx.c src/Events/EventList.hpp src/GameState/Shark.hpp src/Physics/PhysicsMesh.hpp src/Physics/SphereBV.hpp src/GameState/Player.hpp  src/Networking/ConnectionManager.hpp src/Physics/SDFCollision.hpp src/Physics/CollisionInfo.hpp src/GameState/SDFDemo.hpp src/Networking/PacketStructures.hpp src/Server.hpp src/Client.hpp src/Services/Services.hpp src/Loaders/ResourceManager.hpp src/GameState/GameMap.hpp src/Services/MapService.hpp src/GameState/Car.hpp)

target_link_libraries(PointClick SDL2)
if(WIN32)
    target_link_libraries(PointClick ws2_32)
endif(WIN32)