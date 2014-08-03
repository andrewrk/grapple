#include "mainwindow.h"
#include <iostream>
#include <SFML/Graphics.hpp>


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
    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}
