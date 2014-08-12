#include "mainwindow.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>

static int windowWidth = 1920;
static int windowHeight = 1080;

static float deadZoneThreshold = 0.20f;
static float maxPlayerSpeed = 20.0;
static float maxJumpSpeed = 40.0f;
static int maxJumpFrames = 14;
static float clawShootSpeed = 2000.0f;
static float minClawDist = 50.0f;
static float clawReelInSpeed = 5.0f;


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

    footSensorUserData.window = window;
    footSensorUserData.type = FootSensorFixture;
    footSensorUserData.player = this;

    playerBodyUserData.window = window;
    playerBodyUserData.type = PlayerFixture;
    playerBodyUserData.player = this;

    aimUnit = {1, 0};

    resetButtons();
}

void MainWindow::Player::resetButtons()
{
    xAxis = 0.0f;
    yAxis = 0.0f;
    btnJump = false;
    btnFireGrapple = false;
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

    int textHeight = 20;
    physDebugText.setFont(font);
    physDebugText.setString("physics debug");
    physDebugText.setCharacterSize(textHeight);
    physDebugText.setColor(sf::Color(0, 0, 0, 255));
    physDebugText.setPosition(0, arenaHeight - textHeight);

    space = cpSpaceNew();
    cpSpaceSetGravity(space, cpv(0, 1000));
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
            if (player->queuePivoteJoint) {
                player->queuePivoteJoint = false;
                // TODO player->pivotJoint = (b2RevoluteJoint *) space->CreateJoint(&player->revoluteJointDef);
            }
        }

        cpSpaceStep(space, timeStep);

        window.clear(sf::Color(158, 204, 233, 255));

        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            cpVect pos = player->body->p;
            player->sprite.setPosition(pos.x, pos.y);
            player->sprite.setRotation(toDegrees(player->body->a));
            player->resetButtons();
            if (sf::Joystick::isConnected(i)) {
                player->xAxis = joyAxis(i, sf::Joystick::X);
                player->yAxis = joyAxis(i, sf::Joystick::Y);
                player->btnJump = sf::Joystick::isButtonPressed(i, 0);
                player->btnFireGrapple = sf::Joystick::isButtonPressed(i, 2);
            }
            cpVect curVel = player->body->v;

            curVel.x = player->xAxis * maxPlayerSpeed;
            if (player->btnJump && player->footContacts > 0) {
                player->jumpFrameCount = 1;
            } else if (!player->btnJump && player->jumpFrameCount > 0) {
                player->jumpFrameCount = 0;
            }
            if (player->jumpFrameCount > maxJumpFrames) {
                player->jumpFrameCount = 0;
            } else if (player->jumpFrameCount > 0) {
                player->jumpFrameCount += 1;
                curVel.y = -maxJumpSpeed;
            }
            cpBodySetVel(player->body, curVel);

            float scaleSign = sign(player->xAxis);
            if (scaleSign != 0) {
                sf::Vector2f bodyScale = player->sprite.getScale();
                bodyScale.x = fabsf(bodyScale.x) * scaleSign;
                player->sprite.setScale(bodyScale);

                sf::Vector2f armScale = player->armSprite.getScale();
                armScale.x = fabsf(armScale.x) * scaleSign;
                player->armRotateOffset = (scaleSign == -1.0f) ? M_PI : 0;
                player->armSprite.setScale(armScale);

                player->aimUnit = cpvnormalize(cpv(player->xAxis, player->yAxis));
            }
            player->aimStartPos = cpvadd(pos, cpvmult(player->aimUnit, armLength));

            if (player->btnFireGrapple && player->clawState == ClawStateRetracted) {
                player->clawBody = cpSpaceAddBody(space, cpBodyNew(5.0f, INFINITY));
                cpBodySetPos(player->clawBody, player->aimStartPos);
                cpBodySetAngle(player->clawBody, atan2(player->yAxis, player->xAxis));
                cpBodySetVel(player->clawBody, cpvadd(curVel, cpvmult(player->aimUnit, clawShootSpeed)));

                player->clawShape = cpSpaceAddShape(space, cpCircleShapeNew(player->clawBody, clawRadius, cpvzero));
                cpShapeSetFriction(player->clawShape, 0.0f);
                cpShapeSetElasticity(player->clawShape, 0.0f);

                player->slideJoint = cpSlideJointAlloc();
                cpSlideJointInit(player->slideJoint, player->body, player->clawBody,
                                 player->localAnchorPos, player->clawLocalAnchorPos, minClawDist, INFINITY);
                cpSpaceAddConstraint(space, &player->slideJoint->constraint);
                setPlayerClawState(player, ClawStateAir);
            } else if (player->btnFireGrapple && player->clawState == ClawStateAttached) {
                cpVect clawPos = player->clawBody->p;
                float clawDist = cpvlength(cpvsub(clawPos, pos));
                if (clawDist <= minClawDist) {
                    // TODO retract claw
                } else {
                    // prevent the claw from going back out once it goes in
                    float currentLength = player->slideJoint->max;
                    float autoDelta = currentLength - clawDist;
                    float delta = std::max(autoDelta, clawReelInSpeed);
                    float newMax = std::max(currentLength - delta, minClawDist);
                    cpSlideJointSetMax(&player->slideJoint->constraint, newMax);

                    if (i == 0) {
                        std::stringstream ss;
                        ss << "xAxis: " << player->xAxis << " footContacts: " << player->footContacts << " newMax: " << newMax;
                        physDebugText.setString(ss.str());
                    }
                }
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
                player->armSprite.setRotation(toDegrees(player->armRotateOffset + atan2(player->yAxis, player->xAxis)));

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

void MainWindow::addPlatform(cpVect pos, cpVect size, std::string imgName)
{
    Platform *platform = new Platform();

    cpVect topLeft = cpvsub(pos, cpvmult(size, 0.5f));
    platform->shape = cpSegmentShapeNew(space->staticBody, topLeft, cpvadd(topLeft, size), 0.0f);
    cpShapeSetFriction(platform->shape, 1.0f);
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

    player->shape = cpSpaceAddShape(space, cpBoxShapeNew(player->body, imageInfo->width, imageInfo->height));
    cpShapeSetFriction(player->shape, 0.0f);
}
