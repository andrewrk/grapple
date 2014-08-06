#include "mainwindow.h"
#include <iostream>
#include <SFML/Graphics.hpp>
#include "tmxparser/Tmx.h"

static float pixelsPerMeter = 10.0f;
static int windowWidth = 1920;
static int windowHeight = 1080;

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
    delete world;
}

MainWindow::Player::Player(int index)
{
    this->index = index;
    resetButtons();
}

void MainWindow::Player::resetButtons()
{
    xAxis = 0.0f;
    yAxis = 0.0f;
    btnPrimary = false;
    btnAlt = false;
}

int MainWindow::start()
{
    int err = rucksack_bundle_open("assets.bundle", &bundle);
    if (err != RuckSackErrorNone) {
        std::cerr << "Error opening rucksack bundle: " << rucksack_err_str(err) << "\n";
        return -1;
    }

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Grapple", sf::Style::Fullscreen);
    window.setVerticalSyncEnabled(true);
    float timeStep = 1.0f/60.0f;

    arenaWidth = fromPixels(windowWidth);
    arenaHeight = fromPixels(windowHeight);

    world = new b2World(b2Vec2(0, 10));
    players.push_back(new Player(0));
    players.push_back(new Player(1));
    players.push_back(new Player(2));
    players.push_back(new Player(3));

    loadMap();

    sf::View view(sf::FloatRect(0, 0, arenaWidth, arenaHeight));
    window.setView(view);

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

        world->Step(timeStep, 8, 3);
        world->ClearForces();

        window.clear();


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

void MainWindow::loadMap()
{
//    Tmx::Map *map = new Tmx::Map();
//    map->ParseText();
//    Map *map = loadTmx("basic.tmx");

}
