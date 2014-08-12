#include "mainwindow.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cassert>

static int windowWidth = 1920;
static int windowHeight = 1080;

static float deadZoneThreshold = 0.15f;
static float maxPlayerSpeed = 200.0f;
static float playerMoveForceAir = 300.0f;
static float playerMoveForceGround = 600.0f;
static float jumpForce = 800.0f;
static int maxJumpFrames = 14;
static float clawShootSpeed = 2500.0f;
static float minClawDist = 50.0f;
static float retractClawDist = 150.0f;
static float clawReelInSpeedAttached = 12.0f;
static float clawReelInSpeedDetached = 20.0f;
static float jnAccMin = -3000.0f;
static float maxReelOutLength = 99999999.0f;


static float toDegrees(float radians) {
    return radians * 180.0f / M_PI;
}

static float joyAxis(int index, sf::Joystick::Axis axis) {
    float val = sf::Joystick::getAxisPosition(index, axis) / 100.0f;
    return (fabsf(val) < deadZoneThreshold) ? 0.0f : val;
}

static float sign(float x) {
    if (x < 0) {
        return -1.0f;
    } else if (x > 0) {
        return 1.0f;
    } else {
        return 0.0f;
    }
}

MainWindow::MainWindow()
{
}

MainWindow::Player::Player(int i, MainWindow *window)
{
    index = i;
    sprite.setFrameTime(sf::seconds(0.1f));

    clawFixtureUserData.window = window;
    clawFixtureUserData.type = ClawFixture;
    clawFixtureUserData.player = this;

    aimUnit = {1, 0};

    resetButtons();
}

void MainWindow::Player::resetButtons()
{
    xAxis = 0.0f;
    yAxis = 0.0f;
    btnJump = false;
    btnFireGrapple = false;
    btnUnhookGrapple = false;
    btnReelOut = false;
}

void MainWindow::groundQueryCallback(cpShape *shape, void *data) {
    Player *player = reinterpret_cast<Player *>(data);
    player->footContacts += 1;
}

void MainWindow::postSolveCollisionCallback(cpArbiter *arb, cpSpace *space, void *data)
{
    MainWindow *window = reinterpret_cast<MainWindow *>(data);
    window->onPostSolveCollision(arb);
}

int MainWindow::start()
{
    int err = rucksack_bundle_open("assets.bundle", &bundle);
    if (err != RuckSackErrorNone) {
        std::cerr << "Error opening rucksack bundle: " << rucksack_err_str(err) << "\n";
        std::exit(1);
    }



    RuckSackFileEntry *entry = rucksack_bundle_find_file(bundle, "spritesheet", -1);
    if (!entry) {
        std::cerr << "Could not find resource 'spritesheet' in bundle.\n";
        std::exit(1);
    }
    RuckSackTexture *rsTexture;
    err = rucksack_file_open_texture(entry, &rsTexture);
    if (err) {
        std::cerr << "Error reading 'spritesheet' as texture: " << rucksack_err_str(err) << "\n";
        std::exit(1);
    }
    std::vector<unsigned char> buffer;
    buffer.resize(rucksack_texture_size(rsTexture));
    rucksack_texture_read(rsTexture, &buffer[0]);
    spritesheet.loadFromMemory(&buffer[0], buffer.size());

    RuckSackFileEntry *fontEntry = rucksack_bundle_find_file(bundle, "font/LuckiestGuy.ttf", -1);
    if (!fontEntry) {
        std::cerr << "Could not find resource 'font/LuckiestGuy.ttf' in bundle.\n";
        std::exit(1);
    }
    buffer.resize(rucksack_file_size(fontEntry));
    rucksack_file_read(fontEntry, &buffer[0]);
    font.loadFromMemory(&buffer[0], buffer.size());

    long imageCount = rucksack_texture_image_count(rsTexture);
    RuckSackImage **images = (RuckSackImage **)malloc(sizeof(RuckSackImage *) * imageCount);
    rucksack_texture_get_images(rsTexture, images);
    for (int i = 0; i < imageCount; i += 1) {
        struct RuckSackImage *image = images[i];
        imageMap[std::string(image->key, image->key_size)] = image;
    }

    std::vector<std::string> animFrames;

    animFrames.clear();
    animFrames.push_back("img/walk-0.png");
    animFrames.push_back("img/walk-1.png");
    animFrames.push_back("img/walk-2.png");
    animFrames.push_back("img/walk-3.png");
    animFrames.push_back("img/walk-4.png");
    animFrames.push_back("img/walk-5.png");
    loadAnimation(walkingAnim, animFrames);

    animFrames.clear();
    animFrames.push_back("img/jump-0.png");
    animFrames.push_back("img/jump-1.png");
    animFrames.push_back("img/jump-2.png");
    animFrames.push_back("img/jump-3.png");
    loadAnimation(jumpingAnim, animFrames);

    animFrames.clear();
    animFrames.push_back("img/man.png");
    loadAnimation(stillAnim, animFrames);

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Grapple", sf::Style::Default);
    window.setVerticalSyncEnabled(true);
    float timeStep = 1.0f/60.0f;

    arenaWidth = windowWidth;
    arenaHeight = windowHeight;
    armLength = 50.0f;
    ropeColor = sf::Color(255, 255, 0);


    physDebugText.setFont(font);
    physDebugText.setString("physics debug");
    physDebugText.setCharacterSize(20);
    physDebugText.setColor(sf::Color(0, 0, 0, 255));
    physDebugText.setPosition(0, 0);

    space = cpSpaceNew();
    cpSpaceSetGravity(space, cpv(0, 1000));
    cpSpaceSetDamping(space, 0.95f);
    cpSpaceAddCollisionHandler(space, 0, 0, NULL, NULL, postSolveCollisionCallback, NULL, this);

    players.push_back(new Player(0, this));
    players.push_back(new Player(1, this));
    players.push_back(new Player(2, this));
    players.push_back(new Player(3, this));

    loadMap();


    sf::Clock frameClock;
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

        sf::Time frameTime = frameClock.restart();

        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            if (player->queuePivotJoint) {
                player->queuePivotJoint = false;
                cpSpaceAddConstraint(space, &player->pivotJoint->constraint);
                setPlayerClawState(player, ClawStateAttached);
            }
        }



        cpSpaceStep(space, timeStep);

        window.clear(sf::Color(158, 204, 233, 255));

        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            cpVect pos = player->body->p;
            player->sprite.setPosition(pos.x, pos.y);
            player->sprite.setRotation(toDegrees(cpBodyGetAngle(player->body)));
            player->resetButtons();
            if (sf::Joystick::isConnected(i)) {
                player->xAxis = joyAxis(i, sf::Joystick::X);
                player->yAxis = joyAxis(i, sf::Joystick::Y);
                player->btnJump = sf::Joystick::isButtonPressed(i, 0);
                player->btnFireGrapple = sf::Joystick::isButtonPressed(i, 2);
                player->btnUnhookGrapple = sf::Joystick::isButtonPressed(i, 1);
                player->btnReelOut = sf::Joystick::isButtonPressed(i, 3);
            }

            // find out if grounded
            player->footContacts = 0;
            float distFromEdge = 4.0f;
            cpBB footBB = cpBBNew(
                        pos.x - player->size.x / 2.0f + distFromEdge,
                        pos.y + player->size.y / 2.0f + 1.0f,
                        pos.x + player->size.x / 2.0f - distFromEdge,
                        pos.y + player->size.y / 2.0f + 3.0f);
            cpSpaceBBQuery(space, footBB, -1, 0, groundQueryCallback, player);


            cpVect curVel = cpBodyGetVel(player->body);

            float moveForce = (player->footContacts > 0) ? playerMoveForceGround : playerMoveForceAir;
            if (player->xAxis < 0) {
                if (curVel.x >= -maxPlayerSpeed) {
                    cpBodyApplyImpulse(player->body, cpv(-moveForce, 0), cpvzero);
                }
            } else if (player->xAxis > 0) {
                if (curVel.x <= maxPlayerSpeed) {
                    cpBodyApplyImpulse(player->body, cpv(moveForce, 0), cpvzero);
                }
            }


            if (player->btnJump && player->footContacts > 0) {
                player->jumpFrameCount = 1;
            } else if (!player->btnJump && player->jumpFrameCount > 0) {
                player->jumpFrameCount = 0;
            }
            if (player->jumpFrameCount > maxJumpFrames) {
                player->jumpFrameCount = 0;
            } else if (player->jumpFrameCount > 0) {
                player->jumpFrameCount += 1;
                cpBodyApplyImpulse(player->body, cpv(0, -jumpForce), cpvzero);
            }

            float scaleSign = sign(player->xAxis);
            if (scaleSign != 0) {
                sf::Vector2f bodyScale = player->sprite.getScale();
                bodyScale.x = fabsf(bodyScale.x) * scaleSign;
                player->sprite.setScale(bodyScale);

                sf::Vector2f armScale = player->armSprite.getScale();
                armScale.x = fabsf(armScale.x) * scaleSign;
                player->armRotateOffset = (scaleSign == -1.0f) ? M_PI : 0;
                player->armSprite.setScale(armScale);
            }
            if (player->xAxis != 0 || player->yAxis != 0) {
                player->pointAngle = atan2(player->yAxis, player->xAxis);
            }
            player->aimUnit = cpv(cos(player->pointAngle), sin(player->pointAngle));
            player->aimStartPos = cpvadd(pos, cpvmult(player->aimUnit, armLength));

            if (player->btnFireGrapple && player->clawState == ClawStateRetracted) {
                player->clawBody = cpSpaceAddBody(space, cpBodyNew(1.0f, INFINITY));
                cpBodySetPos(player->clawBody, player->aimStartPos);
                cpBodySetAngle(player->clawBody, player->pointAngle);
                cpBodySetVel(player->clawBody, cpvadd(curVel, cpvmult(player->aimUnit, clawShootSpeed)));

                player->clawShape = cpSpaceAddShape(space, cpCircleShapeNew(player->clawBody, clawRadius, cpvzero));
                cpShapeSetFriction(player->clawShape, 0.0f);
                cpShapeSetElasticity(player->clawShape, 0.0f);
                cpShapeSetUserData(player->clawShape, &player->clawFixtureUserData);

                player->slideJoint = cpSlideJointAlloc();
                cpSlideJointInit(player->slideJoint, player->body, player->clawBody,
                                 player->localAnchorPos, player->clawLocalAnchorPos, minClawDist, maxReelOutLength);
                cpSpaceAddConstraint(space, &player->slideJoint->constraint);
                setPlayerClawState(player, ClawStateAir);
            } else if (player->btnFireGrapple && player->clawState == ClawStateAttached) {
                playerReelClawOneFrame(player, false);
            } else if (player->btnFireGrapple && player->clawState == ClawStateDetached) {
                playerReelClawOneFrame(player, true);
            } else if (player->btnUnhookGrapple && player->clawState == ClawStateDetached) {
                playerReelClawOneFrame(player, true);
            } else if (player->btnUnhookGrapple && player->clawState == ClawStateAttached) {
                playerUnhookClaw(player);
            } else if (player->btnReelOut && player->clawState == ClawStateAttached) {
                playerReelOutClawOneFrame(player);
            }

            if (player->clawState != ClawStateRetracted) {
                if (player->slideJoint->jnAcc < jnAccMin) {
                    // too tense. give it some slack.
                    float currentLength = cpSlideJointGetMax(&player->slideJoint->constraint);
                    float newMax = currentLength + getPlayerReelInSpeed(player);
                    cpSlideJointSetMax(&player->slideJoint->constraint, newMax);
                    player->slideJoint->jnAcc = jnAccMin;
                }
            }


            if (i == 0 && player->slideJoint) {
                std::stringstream ss;
                ss << "xAxis: " << player->xAxis << " footContacts: " << player->footContacts <<
                      " jnAcc: " << player->slideJoint->jnAcc;
                physDebugText.setString(ss.str());
            }


            Animation *currentAnim = NULL;
            bool loop = true;
            if (player->footContacts > 0) {
                if (curVel.x != 0) {
                    currentAnim = &walkingAnim;
                } else {
                    currentAnim = &stillAnim;
                }
            } else {
                currentAnim = &jumpingAnim;
                loop = false;
            }
            player->sprite.setLooped(loop);
            if (player->sprite.getAnimation() != currentAnim) {
                player->sprite.setAnimation(*currentAnim);
                player->sprite.play();
            }
            player->armSprite.setPosition(pos.x, pos.y);
            if (player->xAxis != 0 || player->yAxis != 0)
                player->armSprite.setRotation(toDegrees(player->armRotateOffset + player->pointAngle));

        }

        for (int i = 0; i < (int)platforms.size(); i += 1) {
            Platform *platform = platforms[i];
            window.draw(platform->sprite);
        }
        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            player->sprite.update(frameTime);
            window.draw(player->sprite);
            window.draw(player->armSprite);
            if (player->clawState != ClawStateRetracted) {
                cpVect clawPos = player->clawBody->p;
                player->clawSprite.setPosition(clawPos.x, clawPos.y);
                player->clawSprite.setRotation(toDegrees(player->clawBody->a));
                window.draw(player->clawSprite);

                sf::Vertex line[] =
                {
                    sf::Vertex(sf::Vector2f(player->aimStartPos.x, player->aimStartPos.y), ropeColor),
                    sf::Vertex(sf::Vector2f(clawPos.x, clawPos.y), ropeColor)
                };

                window.draw(line, 2, sf::Lines);
            }
        }
        window.draw(physDebugText);

        window.display();
    }

    return 0;
}

void MainWindow::loadMap()
{
    map = new Tmx::Map();
    map->ParseText(getResourceString("text/basic.tmx"));
    if (map->HasError()) {
        std::cerr << "error parsing map: " << map->GetErrorText() << "\n";
        std::exit(1);
    }
    for (int i = 0; i < map->GetNumObjectGroups(); i += 1) {
        const Tmx::ObjectGroup *objectGroup = map->GetObjectGroup(i);
        for (int j = 0; j < objectGroup->GetNumObjects(); j += 1) {
            const Tmx::Object *object = objectGroup->GetObject(j);
            cpVect size = cpv(object->GetWidth(), object->GetHeight());
            cpVect pos = cpv(object->GetX() + size.x / 2, object->GetY() + size.y / 2);
            const Tmx::PropertySet &properties = object->GetProperties();
            std::string img = properties.GetLiteralProperty("img");

            if (object->GetName().compare("Platform") == 0) {
                addPlatform(pos, size, img);
            } else if (object->GetName().compare("Start") == 0) {
                int index = properties.GetNumericProperty("player");
                initPlayer(index, pos);
            } else {
                std::cerr << "unrecognized object name: " << object->GetName() << "\n";
                std::exit(1);
            }
        }
    }
}

std::string MainWindow::getResourceString(const std::string &key)
{
    RuckSackFileEntry *entry = rucksack_bundle_find_file(bundle, key.c_str(), key.size());

    if (!entry) {
        std::cerr << "Could not find resource '" << key << "' in bundle.\n";
        std::exit(1);
    }

    long size = rucksack_file_size(entry);
    std::string contents;
    contents.resize(size);
    int err = rucksack_file_read(entry, reinterpret_cast<unsigned char *>(&contents[0]));

    if (err) {
        std::cerr << "Error reading '" << key << "' resource: " << rucksack_err_str(err) << "\n";
        std::exit(1);
    }

    return contents;
}

sf::IntRect MainWindow::imageInfoToTextureRect(RuckSackImage *imageInfo)
{
    return sf::IntRect(imageInfo->x, spritesheet.getSize().y - imageInfo->y - imageInfo->height,
                       imageInfo->width, imageInfo->height);
}

void MainWindow::loadAnimation(Animation &animation, const std::vector<std::string> &list)
{
    animation.setSpriteSheet(spritesheet);
    for (int i = 0; i < (int)list.size(); i += 1) {
        std::string frame = list[i];
        std::map<std::string, RuckSackImage *>::iterator it = imageMap.find(frame);
        if (it == imageMap.end()) {
            std::cerr << "Missing image: " << frame << "\n";
            std::exit(1);
        }
        RuckSackImage *imageInfo = it->second;
        animation.addFrame(imageInfoToTextureRect(imageInfo));
    }
}

void MainWindow::setPlayerClawState(MainWindow::Player *player, MainWindow::ClawState state)
{
    player->clawState = state;
    switch (state) {
    case ClawStateRetracted:
        player->clawSprite.setTextureRect(clawInAirRect);
        player->armSprite.setTextureRect(armNormalRect);
        break;
    case ClawStateAir:
        player->clawSprite.setTextureRect(clawInAirRect);
        player->armSprite.setTextureRect(armFlungRect);
        break;
    case ClawStateAttached:
        player->clawSprite.setTextureRect(clawAttachedRect);
        player->armSprite.setTextureRect(armFlungRect);
        break;
    case ClawStateDetached:
        player->clawSprite.setTextureRect(clawDetachedRect);
        player->armSprite.setTextureRect(armFlungRect);
        break;
    }

}

void MainWindow::onPostSolveCollision(cpArbiter *arb)
{
    cpShape *a;
    cpShape *b;
    cpArbiterGetShapes(arb, &a, &b);

    FixtureIdent *identA = reinterpret_cast<FixtureIdent*>(cpShapeGetUserData(a));
    FixtureIdent *identB = reinterpret_cast<FixtureIdent*>(cpShapeGetUserData(b));

    if (identA) {
        switch (identA->type) {
        case ClawFixture:
            handleClawHit(identA->player, arb, b);
            return;
        }
        std::cerr << "Unrecognized fixture identification type: " << identA->type << "\n";
        assert(0);
    }
    if (identB) {
        switch (identB->type) {
        case ClawFixture:
            handleClawHit(identA->player, arb, a);
            return;
        }
        std::cerr << "Unrecognized fixture identification type: " << identB->type << "\n";
        assert(0);
    }
}

void MainWindow::handleClawHit(MainWindow::Player *player, cpArbiter *arb, cpShape *otherShape)
{
    if (player->clawState != ClawStateAir || player->queuePivotJoint)
        return;

    cpContactPointSet pointSet = cpArbiterGetContactPointSet(arb);

    // average the contact points to get a single value
    float scaleVal = 1 / (float) pointSet.count;
    cpVect pt = cpvzero;
    for (int i = 0; i < pointSet.count; i += 1) {
        pt = cpvadd(pt, cpvmult(pointSet.points[i].point, scaleVal));
    }

    cpVect shapeAnchor = cpvsub(pt, otherShape->body->p);
    cpVect clawAnchor = cpvsub(pt, player->clawBody->p);

    player->pivotJoint = cpPivotJointAlloc();
    cpPivotJointInit(player->pivotJoint, player->clawBody, otherShape->body, clawAnchor, shapeAnchor);
    player->queuePivotJoint = true;
}

void MainWindow::playerRetractClaw(MainWindow::Player *player)
{
    assert(player->clawState != ClawStateRetracted);

    if (player->clawState == ClawStateAttached)
        playerUnhookClaw(player);

    setPlayerClawState(player, ClawStateRetracted);

    cpSpaceRemoveConstraint(space, &player->slideJoint->constraint);
    cpConstraintDestroy(&player->slideJoint->constraint);
    player->slideJoint = NULL;

    cpSpaceRemoveShape(space, player->clawShape);
    cpShapeDestroy(player->clawShape);
    player->clawShape = NULL;

    cpSpaceRemoveBody(space, player->clawBody);
    cpBodyDestroy(player->clawBody);
    player->clawBody = NULL;
}

void MainWindow::playerUnhookClaw(MainWindow::Player *player)
{
    assert(player->clawState == ClawStateAttached);

    setPlayerClawState(player, ClawStateDetached);

    cpSpaceRemoveConstraint(space, &player->pivotJoint->constraint);
    cpConstraintDestroy(&player->pivotJoint->constraint);
    player->pivotJoint = NULL;
}

void MainWindow::playerReelClawOneFrame(MainWindow::Player *player, bool retract)
{
    cpVect clawPos = player->clawBody->p;
    float clawDist = cpvlength(cpvsub(clawPos, cpBodyGetPos(player->body)));
    if (clawDist <= retractClawDist) {
        if (retract)
            playerRetractClaw(player);
    } else {
        // prevent the claw from going back out once it goes in
        float currentLength = cpSlideJointGetMax(&player->slideJoint->constraint);
        float autoDelta = currentLength - std::max(clawDist, retractClawDist);
        float delta = std::max(autoDelta, getPlayerReelInSpeed(player));
        float newMax = std::max(currentLength - delta, minClawDist);
        cpSlideJointSetMax(&player->slideJoint->constraint, newMax);

    }
}

void MainWindow::playerReelOutClawOneFrame(MainWindow::Player *player)
{
    float currentLength = cpSlideJointGetMax(&player->slideJoint->constraint);
    float newMax = std::min(currentLength + getPlayerReelInSpeed(player), maxReelOutLength);
    cpSlideJointSetMax(&player->slideJoint->constraint, newMax);
}

float MainWindow::getPlayerReelInSpeed(MainWindow::Player *player)
{
    return (player->clawState == ClawStateAttached) ? clawReelInSpeedAttached : clawReelInSpeedDetached;
}

void MainWindow::addPlatform(cpVect pos, cpVect size, std::string imgName)
{
    Platform *platform = new Platform();

    platform->body = cpBodyNewStatic();
    cpBodySetPos(platform->body, pos);
    platform->shape = cpBoxShapeNew(platform->body, size.x, size.y);
    cpShapeSetFriction(platform->shape, 0.8f);
    cpSpaceAddShape(space, platform->shape);

    platform->sprite.setTexture(spritesheet);

    std::map<std::string, RuckSackImage *>::iterator it = imageMap.find(imgName);
    if (it == imageMap.end()) {
        std::cerr << "Missing image: " << imgName << "\n";
        std::exit(1);
    }
    RuckSackImage *imageInfo = it->second;
    platform->sprite.setTextureRect(imageInfoToTextureRect(imageInfo));
    platform->sprite.setOrigin(imageInfo->anchor_x, imageInfo->anchor_y);
    platform->sprite.setPosition(pos.x, pos.y);
    platform->sprite.setScale(size.x / (float)imageInfo->width, size.y / (float)imageInfo->height);

    platforms.push_back(platform);
}

void MainWindow::initPlayer(int index, cpVect pos)
{
    Player *player = players[index];
    RuckSackImage *imageInfo = imageMap.at("img/man.png");
    player->sprite.setOrigin(imageInfo->anchor_x, imageInfo->anchor_y);
    player->localAnchorPos = cpv(imageInfo->anchor_x, imageInfo->anchor_y);
    player->size = cpv(imageInfo->width, imageInfo->height);

    RuckSackImage *armImageInfo = imageMap.at("img/arm.png");
    player->armSprite.setTexture(spritesheet);
    player->armSprite.setOrigin(armImageInfo->anchor_x, armImageInfo->anchor_y);
    armNormalRect = imageInfoToTextureRect(armImageInfo);
    player->armSprite.setTextureRect(armNormalRect);
    RuckSackImage *armFlungImageInfo = imageMap.at("img/arm-flung.png");
    armFlungRect = imageInfoToTextureRect(armFlungImageInfo);

    RuckSackImage *clawOpenImageInfo = imageMap.at("img/claw.png");
    player->clawSprite.setTexture(spritesheet);
    player->clawSprite.setOrigin(clawOpenImageInfo->anchor_x, clawOpenImageInfo->anchor_y);
    clawInAirRect = imageInfoToTextureRect(clawOpenImageInfo);
    clawRadius = clawOpenImageInfo->width / 2.0f;
    RuckSackImage *clawClosedImageInfo = imageMap.at("img/claw-retracted.png");
    clawDetachedRect = imageInfoToTextureRect(clawClosedImageInfo);
    RuckSackImage *clawAttachedImageInfo = imageMap.at("img/claw-attached.png");
    clawAttachedRect = imageInfoToTextureRect(clawAttachedImageInfo);
    player->clawLocalAnchorPos = cpv(clawOpenImageInfo->anchor_x, clawOpenImageInfo->anchor_y);

    player->body = cpSpaceAddBody(space, cpBodyNew(20.0f, INFINITY));
    cpBodySetPos(player->body, cpv(pos.x, pos.y));

    player->shape = cpSpaceAddShape(space, cpBoxShapeNew(player->body, player->size.x, player->size.y));
    cpShapeSetFriction(player->shape, 0.8f);
}
