#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL.h>
#include <Box2D/Box2D.h>

#include <rucksack.h>


class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    int start();

private:


    class Player {
    public:
        int index;
        float xAxis;
        float yAxis;
        bool btnPrimary;
        bool btnAlt;

        Player(int index);
        void resetButtons();
    };


    // in meters
    float arenaWidth;
    float arenaHeight;

    b2World *world;
    std::vector<Player*> players;

    RuckSackBundle *bundle;

    static float fromPixels(float pixels);
    static float toPixels(float meters);

    void loadMap();
};

#endif // MAINWINDOW_H
