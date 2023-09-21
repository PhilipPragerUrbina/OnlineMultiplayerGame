# PointClickAdventure
Jokingly named point&click, this is an online multiplayer shooter game inspired by Valve's half life multiplayer mode. 

It runs on a custom engine built from the ground up,even featuring a software renderer.

Uses SDL2 and GLM. As well as TinyOBJ, UFBX, and STB_image for IO.
![gamescreenshot](https://github.com/PhilipPragerUrbina/OnlineMultiplayerGame/assets/72355251/03135ee7-aaa9-48f4-9e67-baf36e1dcad0)


Currently features:
* Software renderer
  * Transparency
  * Clipping
  * Perspective correct textures
  * Fast performance
  * Skinned meshes
* Physics engine
  * Raycasting
  * Map player collisions
  * Acceleration structure
  * Player controller
* Game engine
  * Event system
  * Concurrent render and game thread
* Networking
  * TCP + UDP combined synchronous connection.
  * Server authoratative protocol
  * Gameobject behavior representation model(state vs. prediction)

Future features:
* Software renderer
  * Lighting
  * Gourad Shading
  * GPU backend
  * Baked light maps(seperate light baking project known as OVEN)
  * Ray marching integration
  * Performance metrics
* Physics engine
  * Mesh/polygon collisions
  * Dynamic BVH
  * Particle system
* Game engine
  * Procedural animations
  * Scene graph on server
  * Vehicles
  * Terrain system
  * Frustum culling
* Networking
  * Multiplayer
  * Lag compensation
* Audio ray tracing
* And more!
