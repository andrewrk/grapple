#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <rucksack.h>
#include <map>
#include "tmxparser/Tmx.h"


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
        sf::Sprite sprite;
        b2Body *body;

        Player(int index);
        void resetButtons();
    };

    class Platform {
    public:
        sf::Sprite sprite;
        b2Body *body;
    };

    // in meters
    float arenaWidth;
    float arenaHeight;

    b2World *world;
    std::vector<Player*> players;
    std::vector<Platform *> platforms;

    RuckSackBundle *bundle;
    std::map<std::string, RuckSackImage *> imageMap;

    Tmx::Map *map = NULL;
    sf::Font font;
    sf::Text physDebugText;


    sf::Texture spritesheet;

    static float fromPixels(float pixels);
    static float toPixels(float meters);

    void loadMap();

    std::string getResourceString(const std::string &key);

    void addPlatform(b2Vec2 pos, b2Vec2 size, std::string img);
    void initPlayer(int index, b2Vec2 pos);

    sf::IntRect imageInfoToTextureRect(RuckSackImage *imageInfo);

};

#endif // MAINWINDOW_H
