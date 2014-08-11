#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <rucksack.h>
#include <map>
#include <tmxparser/Tmx.h>

#include "animatedsprite.h"
#include "animation.h"


class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    int start();

private:


    enum FixtureIdentType {
        FootSensorFixture,
        ClawFixture,
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
        bool btnPrimary;
        bool btnAlt;
        AnimatedSprite sprite;
        sf::Sprite armSprite;
        sf::Sprite clawSprite;
        ClawState clawState;
        b2Body *clawBody;
        FixtureIdent clawFixtureUserData;
        FixtureIdent footSensorUserData;
        b2Vec2 aimStartPos; // where the claw will be created
        b2Vec2 localAnchorPos;
        b2Vec2 clawLocalAnchorPos;
        b2RopeJoint *ropeJoint;

        b2Body *body;
        int footContacts;
        int jumpFrameCount;
        float armRotateOffset;

        Player(int index, MainWindow *window);
        void resetButtons();
    };

    class Platform {
    public:
        sf::Sprite sprite;
        b2Body *body;
    };

    class GlobalContactListener : public b2ContactListener {
    public:
        virtual void BeginContact(b2Contact* contact) override;
        virtual void EndContact(b2Contact* contact) override;
        MainWindow *window;
        GlobalContactListener(MainWindow *window) : window(window){}
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

    static float fromPixels(float pixels);
    static float toPixels(float meters);

    void loadMap();

    std::string getResourceString(const std::string &key);

    void addPlatform(b2Vec2 pos, b2Vec2 size, std::string img);
    void initPlayer(int index, b2Vec2 pos);

    sf::IntRect imageInfoToTextureRect(RuckSackImage *imageInfo);

    void loadAnimation(Animation &animation, const std::vector<std::string> &list);

    void handleClawHit(Player *player, b2Contact *contact, b2Fixture *clawFixture);
    void setPlayerClawState(Player *player, ClawState state);

    friend class GlobalContactListener;
};

#endif // MAINWINDOW_H
