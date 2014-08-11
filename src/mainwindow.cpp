#include "mainwindow.h"
#include <iostream>
#include <sstream>

static float pixelsPerMeter = 10.0f;
static int windowWidth = 1920;
static int windowHeight = 1080;

static float deadZoneThreshold = 0.20f;
static float maxPlayerSpeed = 20.0f;
static float maxJumpSpeed = 40.0f;
static int maxJumpFrames = 14;


static float PI = 3.14159265358979f;

static float toDegrees(float radians) {
    return radians * 180 / PI;
}

static float joyAxis(int index, sf::Joystick::Axis axis) {
    float val = sf::Joystick::getAxisPosition(index, axis) / 100.0f;
    return (fabsf(val) < deadZoneThreshold) ? 0.0f : val;
}

static float signe(float x) {
    if (x < 0) {
        return -1.0f;
    } else {
        return 1.0f;
    }
}

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
    delete world;
    delete map;
}

MainWindow::Player::Player(int index)
{
    this->index = index;
    footContacts = 0;
    jumpFrameCount = 0;
    sprite.setFrameTime(sf::seconds(0.1f));
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

    arenaWidth = fromPixels(windowWidth);
    arenaHeight = fromPixels(windowHeight);


    physDebugText.setFont(font);
    physDebugText.setString("physics debug");
    physDebugText.setCharacterSize(20);
    physDebugText.setColor(sf::Color(0, 0, 0, 255));
    physDebugText.setScale(fromPixels(1), fromPixels(1));
    physDebugText.setPosition(0, arenaHeight - fromPixels(20));

    world = new b2World(b2Vec2(0, 100));
    GlobalContactListener *listener = new GlobalContactListener(this);
    world->SetContactListener(listener);
    players.push_back(new Player(0));
    players.push_back(new Player(1));
    players.push_back(new Player(2));
    players.push_back(new Player(3));

    loadMap();


    sf::View view(sf::FloatRect(0, 0, arenaWidth, arenaHeight));
    window.setView(view);

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
                case sf::Keyboard::S:
                    view.zoom(0.9);
                    window.setView(view);
                    break;
                case sf::Keyboard::T:
                    view.zoom(1.1);
                    window.setView(view);
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

        world->Step(timeStep, 8, 3);
        world->ClearForces();

        window.clear(sf::Color(158, 204, 233, 255));

        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            b2Vec2 pos = player->body->GetPosition();
            player->sprite.setPosition(pos.x, pos.y);
            player->sprite.setRotation(toDegrees(player->body->GetAngle()));
            player->resetButtons();
            if (sf::Joystick::isConnected(i)) {
                player->xAxis = joyAxis(i, sf::Joystick::X);
                player->yAxis = joyAxis(i, sf::Joystick::Y);
                player->btnPrimary = sf::Joystick::isButtonPressed(i, 0);
                player->btnAlt = sf::Joystick::isButtonPressed(i, 1);
            }
            b2Vec2 curVel = player->body->GetLinearVelocity();

            curVel.x = player->xAxis * maxPlayerSpeed;
            if (player->btnPrimary && player->footContacts > 0) {
                player->jumpFrameCount = 1;
            } else if (!player->btnPrimary && player->jumpFrameCount > 0) {
                player->jumpFrameCount = 0;
            }
            if (player->jumpFrameCount > maxJumpFrames) {
                player->jumpFrameCount = 0;
            } else if (player->jumpFrameCount > 0) {
                player->jumpFrameCount += 1;
                curVel.y = -maxJumpSpeed;
            }
            player->body->SetLinearVelocity(curVel);

            sf::Vector2f scale = player->sprite.getScale();
            scale.x = fabsf(scale.x) * signe(player->xAxis);
            player->sprite.setScale(scale);

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
            if (i == 0) {
                std::stringstream ss;
                ss << "xAxis: " << player->xAxis << " footContacts: " << player->footContacts << " loop: " << loop;
                physDebugText.setString(ss.str());
            }
        }

        for (int i = 0; i < (int)platforms.size(); i += 1) {
            Platform *platform = platforms[i];
            window.draw(platform->sprite);
        }
        for (int i = 0; i < (int)players.size(); i += 1) {
            Player *player = players[i];
            player->sprite.update(frameTime);
            window.draw(player->sprite);
        }
        window.draw(physDebugText);

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
            b2Vec2 size(fromPixels(object->GetWidth()), fromPixels(object->GetHeight()));
            b2Vec2 pos(fromPixels(object->GetX()) + size.x / 2, fromPixels(object->GetY()) + size.y / 2);
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

void MainWindow::addPlatform(b2Vec2 pos, b2Vec2 size, std::string imgName)
{
    Platform *platform = new Platform();
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(pos.x, pos.y);
    platform->body = world->CreateBody(&bodyDef);
    b2PolygonShape shape;
    shape.SetAsBox(size.x / 2, size.y / 2);
    platform->body->CreateFixture(&shape, 0);

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
    platform->sprite.setScale(fromPixels(size.x / fromPixels(imageInfo->width)), fromPixels(size.y / fromPixels(imageInfo->height)));

    platforms.push_back(platform);
}

void MainWindow::initPlayer(int index, b2Vec2 pos)
{
    Player *player = players[index];
    RuckSackImage *imageInfo = imageMap.at("img/man.png");
    player->sprite.setOrigin(imageInfo->anchor_x, imageInfo->anchor_y);
    player->sprite.setPosition(pos.x, pos.y);
    player->sprite.setScale(fromPixels(1), fromPixels(1));

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(pos.x, pos.y);
    player->body = world->CreateBody(&bodyDef);
    player->body->SetFixedRotation(true);
    b2PolygonShape shape;
    shape.SetAsBox(fromPixels(imageInfo->width) / 2.0f, fromPixels(imageInfo->height) / 2.0f);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.0f;
    player->body->CreateFixture(&fixtureDef);

    shape.SetAsBox(fromPixels(imageInfo->width) / 2.0f - 0.01f, 0.3f, b2Vec2(0, fromPixels(imageInfo->height) / 2.0f), 0.0f);
    fixtureDef.isSensor = true;
    b2Fixture *sensor = player->body->CreateFixture(&fixtureDef);
    sensor->SetUserData(player);
}


void MainWindow::GlobalContactListener::BeginContact(b2Contact *contact)
{
    Player *player;

    player = (Player*) contact->GetFixtureA()->GetUserData();
    if (player)
        player->footContacts += 1;


    player = (Player*) contact->GetFixtureB()->GetUserData();
    if (player)
        player->footContacts += 1;
}

void MainWindow::GlobalContactListener::EndContact(b2Contact *contact)
{
    Player *player;

    player = (Player*) contact->GetFixtureA()->GetUserData();
    if (player)
        player->footContacts -= 1;


    player = (Player*) contact->GetFixtureB()->GetUserData();
    if (player)
        player->footContacts -= 1;
}
