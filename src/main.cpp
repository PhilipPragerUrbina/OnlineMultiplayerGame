
#include "Renderer/SDL/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Loaders/OBJLoader.hpp"
#include "Loaders/TextureLoader.hpp"
#include "Loaders/FBXLoader.hpp"
#include "GameState/GameObject.hpp"
#include "GameState/Shark.hpp"
#include "GameState/Player.hpp"
#include "Networking/UDPSocket.hpp"
#include <chrono>
#include <memory>
#include <thread>
#include <typeinfo>
#include "server_main.hpp"
#include "Physics/SDFCollision.hpp"
#include "GameState/SDFDemo.hpp"

bool window_close = false; //does not need atomic as is only written by one thread

//todo add external licenses

/**
 * The rendering thread
 * @param window Window to render to
 * @param gameobjects Gameobjects to render
 * @param renderer Renderer to use
 * @param buffer Framebuffer to render to
 */
void runRender(Window* window, const std::vector<std::unique_ptr<GameObject>>* gameobjects, Renderer* renderer, FrameBuffer* buffer){
    while (!window_close){
        renderer->clearFrame(*buffer, {0,0,0});
        for (const auto& gameobject : *gameobjects) {
            gameobject->render(*renderer,*buffer);
        }
        window->drawFrameBuffer(*buffer);
    }
}

//for now any additional command line args create server
int main(int argc, char* argv[]) {

    bool is_server = false;
    if(argc > 1){
        is_server = true;
    }

    if(is_server){
        return serverMain();
    }

    std::cout << "Initializing client \n";
    UDPSocket socket;
    UDPSocket::Address server = UDPSocket::getAddress(8080, "127.0.0.1");

    std::string message = "whoo hoo whoooooooooooooooooooooooo hooooooooooooooo!!!!!";
    std::cout << "Sending message to server \n";
    socket.sendTo(server,message.c_str(), message.size());

    std::cout << "Closing Client \n";
    socket.close();

    const int WIDTH = 800;
    const int HEIGHT = 800;
    Window window(WIDTH,HEIGHT,"Shork");
    FrameBuffer buffer(WIDTH,HEIGHT,{0,0,0,0});

    std::vector<std::unique_ptr<GameObject>> gameobjects;

    Camera camera(90,{0,0,1}, (float)WIDTH / HEIGHT);
    camera.setPosition({0,2,5});

    gameobjects.emplace_back(new Shark{});
    gameobjects.emplace_back(new Shark{glm::translate(glm::identity<glm::mat4>(), {3,0,0})});
    gameobjects.emplace_back(new Shark{glm::translate(glm::scale(glm::identity<glm::mat4>(), {0.2,0.2,0.2}),{0,15,0})});
    gameobjects.emplace_back(new SDFDemo());

    //Player sets camera so should be drawn last or first
    gameobjects.emplace_back(new Player{{0,2,5},camera});


    std::vector<Texture> textures;
    for (const auto& gameobject : gameobjects) {
        gameobject->setUp(textures);
    }




    Renderer renderer(textures,camera);

    std::thread render_thread(runRender, &window, &gameobjects,&renderer,&buffer);

    window.setMouseRelative();


    EventList event;
    auto last_update = std::chrono::steady_clock::now();
    while (window.isOpen(event)) {

        auto now = std::chrono::steady_clock::now();
        double delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(now - last_update).count() / 1000000.0f;
        last_update = now;
        auto start = std::chrono::steady_clock::now();
        for (auto& gameobject : gameobjects) {
            gameobject->update(delta_time,event);
        }
        event.events.clear();

    }

    window_close = true;
    render_thread.join();

    return 0;
}

