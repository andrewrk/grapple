#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL.h>


class MainWindow
{
public:
    MainWindow();

    int start();

private:
    int screenWidth = 1920;
    int screenHeight = 1080;
    SDL_Window *sdlWindow = NULL;
    bool running = true;
    SDL_Renderer *renderer = NULL;
    float mMaxSpf;
    float mFps;



    void flushEvents();
    void draw();
    void update(float dt, float dx);


};

#endif // MAINWINDOW_H
