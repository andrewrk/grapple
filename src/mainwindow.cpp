#include "mainwindow.h"
#include <iostream>
#include <SFML/Graphics.hpp>

static float pixelsPerMeter = 10.0f;
static int windowWidth = 1920;
static int windowHeight = 1080;

MainWindow::MainWindow()
{
}

int MainWindow::start()
{
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Grapple", sf::Style::Fullscreen);
    window.setVerticalSyncEnabled(true);

    arenaWidth = fromPixels(windowWidth);
    arenaHeight = fromPixels(windowHeight);

    // let's define a view
    sf::View view(sf::FloatRect(0, 0, arenaWidth, arenaHeight));
    window.setView(view);

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}

float MainWindow::fromPixels(float pixels)
{
    return pixels / pixelsPerMeter;
}

float MainWindow::toPixels(float meters)
{
    return meters * pixelsPerMeter;
}
