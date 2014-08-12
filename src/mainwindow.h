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
        bool btnJump;
        bool btnFireGrapple;
        bool btnUnhookGrapple;

        AnimatedSprite sprite;
        sf::Sprite armSprite;
        sf::Sprite clawSprite;
        ClawState clawState = ClawStateRetracted;
        cpBody *clawBody = NULL;
        cpShape *clawShape = NULL;
        FixtureIdent clawFixtureUserData;
        cpVect aimStartPos; // where the claw will be created
        cpVect aimUnit; // unit vector pointing where aiming
        float pointAngle = 0.0f;
        cpVect localAnchorPos;
        cpVect clawLocalAnchorPos;
        cpSlideJoint *slideJoint = NULL;
        cpPivotJoint *pivotJoint = NULL;
        bool queuePivotJoint = false;

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

    void onPostSolveCollision(cpArbiter *arb);
    void handleClawHit(Player *player, cpArbiter *arb, cpShape *otherShape);
    void playerRetractClaw(Player *player);
    void playerUnhookClaw(Player *player);
    void playerReelClawOneFrame(Player *player, bool retract);
    float getPlayerReelInSpeed(Player *player);

    static void groundQueryCallback(cpShape *shape, void *data);
    static void postSolveCollisionCallback(cpArbiter *arb, cpSpace *space, void *data);
};

#endif // MAINWINDOW_H
