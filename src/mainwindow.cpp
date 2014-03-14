#include "mainwindow.h"
#include <iostream>
#include <chrono>


static const float EPSILON = 0.0000000001;
static const float MAX_DISPLAY_FPS = 90000;
static const float TARGET_FPS = 60.0;
static const float TARGET_SPF = 1.0 / TARGET_FPS;
static const float FPS_SMOOTHNESS = 0.9;
static const float FPS_ONE_FRAME_WEIGHT = 1.0 - FPS_SMOOTHNESS;


MainWindow::MainWindow()
{
}

int MainWindow::start()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Unable to initialize SDL\n";
        exit(1);
    }
    atexit(SDL_Quit);

    sdlWindow = SDL_CreateWindow("Grapple",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screenWidth, screenHeight, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!sdlWindow) {
        std::cerr << "Unable to create window\n";
        exit(1);
    }
    renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    mFps = 60.0f;
    mMaxSpf = 1 / mFps;
    std::chrono::high_resolution_clock::time_point previousTime = std::chrono::high_resolution_clock::now();
    while(running) {
        flushEvents();
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - previousTime);
        previousTime = now;
        float dt = delta.count();
        if (dt < EPSILON) dt = EPSILON;
        if (dt > mMaxSpf) dt = mMaxSpf;
        float dx = dt / TARGET_SPF;
        update(dt, dx);
        draw();
        float fps = 1 / delta.count();
        fps = fps < MAX_DISPLAY_FPS ? fps : MAX_DISPLAY_FPS;
        mFps = mFps * FPS_SMOOTHNESS + fps * FPS_ONE_FRAME_WEIGHT;
    }

    return 0;
}

void MainWindow::flushEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
            running = false;
            break;
        }
    }
}

void MainWindow::draw()
{
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void MainWindow::update(float dt, float dx)
{

}
