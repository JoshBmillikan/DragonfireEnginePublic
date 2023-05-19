//
// Created by josh on 5/17/23.
//

#pragma once
#include <renderer.h>

namespace dragonfire {

class App {
public:
    static App INSTANCE;
    void init();
    void run();
    void shutdown();

    void stop() { running = false; }

private:
    bool running = false;
    Renderer* renderer = nullptr;

    App() = default;
    void processEvents();
    void update(double deltaTime);
};

}   // namespace dragonfire