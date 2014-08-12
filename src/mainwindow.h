#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SFML/Graphics.hpp>
#include <chipmunk/chipmunk.h>

#include <rucksack.h>
#include <map>
#include <tmxparser/Tmx.h>

#include "animatedsprite.h"
#include "animation.h"


class MainWindow
{
public:
    MainWindow();

    int start();

private:


    enum FixtureIdentType {
        FootSensorFixture,
        ClawFixture,
        PlayerFixture,
    };

    class Player;
    struct FixtureIdent {
        FixtureIdentType type;
        MainWindow *window;
        Player *player;
    };

    enum ClawState {
        ClawStateRetracted,
        ClawStateAir,
        ClawStateAttached,
        ClawStateDetached,
    };

    class Player {
    public:
        int index;
        float xAxis;
        float yAxis;
        bool btnJump;
        bool btnFireGrapple;
        AnimatedSprite sprite;
        sf::Sprite armSprite;
        sf::Sprite clawSprite;
        ClawState clawState = ClawStateRetracted;
        cpBody *clawBody = NULL;
        cpShape *clawShape = NULL;
        FixtureIdent clawFixtureUserData;
        FixtureIdent footSensorUserData;
        FixtureIdent playerBodyUserData;
        cpVect aimStartPos; // where the claw will be created
        cpVect aimUnit; // unit vector pointing where aiming
        cpVect localAnchorPos;
        cpVect clawLocalAnchorPos;
        cpSlideJoint *slideJoint = NULL;
        cpPivotJoint *pivotJoint = NULL;
        bool queuePivoteJoint = false;

        cpBody *body = NULL;
        cpShape *shape = NULL;
        cpVect size;
        int footContacts = 0;
        int jumpFrameCount = 0;
        float armRotateOffset = 0;

        Player(int index, MainWindow *window);
        void resetButtons();
    };

    class Platform {
    public:
        sf::Sprite sprite;
        cpShape *shape;
        cpBody *body;
    };


    float arenaWidth;
    float arenaHeight;

    cpSpace *space;
    std::vector<Player*> players;
    std::vector<Platform *> platforms;

    RuckSackBundle *bundle;
    std::map<std::string, RuckSackImage *> imageMap;

    Tmx::Map *map = NULL;
    sf::Font font;
    sf::Text physDebugText;

    sf::Texture spritesheet;
    Animation walkingAnim;
    Animation jumpingAnim;
    Animation stillAnim;


    sf::IntRect clawInAirRect;
    sf::IntRect clawDetachedRect;
    sf::IntRect clawAttachedRect;
    float clawRadius;
    float armLength;
    sf::IntRect armFlungRect;
    sf::IntRect armNormalRect;



    sf::Color ropeColor;

    void loadMap();

    std::string getResourceString(const std::string &key);

    void addPlatform(cpVect pos, cpVect size, std::string img);
    void initPlayer(int index, cpVect pos);

    sf::IntRect imageInfoToTextureRect(RuckSackImage *imageInfo);

    void loadAnimation(Animation &animation, const std::vector<std::string> &list);

    void setPlayerClawState(Player *player, ClawState state);

    static void groundQueryCallback(cpShape *shape, void *data);
};

#endif // MAINWINDOW_H
