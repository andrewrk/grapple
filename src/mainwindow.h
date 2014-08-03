#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL.h>
#include <Box2D/Box2D.h>


class MainWindow
{
public:
    MainWindow();

    int start();

private:

    // in meters
    float arenaWidth;
    float arenaHeight;

    static float fromPixels(float pixels);
    static float toPixels(float meters);
};

#endif // MAINWINDOW_H
