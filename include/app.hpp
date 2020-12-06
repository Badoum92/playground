#pragma once
#include "base/types.hpp"
#include "camera.hpp"
#include "ecs.hpp"
#include "file_watcher.hpp"
#include "platform/window.hpp"
#include "render/renderer.hpp"
#include "timer.hpp"
#include "ui.hpp"

namespace my_app
{

struct CameraComponent
{
    u8 nice = 38;
    static const char *type_name() { return "CameraComponent"; }
};

class App
{
  public:
    App();
    ~App();

    void run();

  private:
    void camera_update();
    void update();
    void display_ui();

    UI::Context ui;
    window::Window window;
    InputCamera camera;
    Renderer renderer;
    TimerData timer;
    ECS::World ecs;

    FileWatcher watcher;
    Watch shaders_watch;

    bool is_minimized;
};

} // namespace my_app
