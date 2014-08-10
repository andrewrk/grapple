#include "mainwindow.h"
#include <iostream>

static float pixelsPerMeter = 10.0f;
static int windowWidth = 1920;
static int windowHeight = 1080;

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

    long imageCount = rucksack_texture_image_count(rsTexture);
    RuckSackImage **images = (RuckSackImage **)malloc(sizeof(RuckSackImage *) * imageCount);
    rucksack_texture_get_images(rsTexture, images);
    for (int i = 0; i < imageCount; i += 1) {
        struct RuckSackImage *image = images[i];
        imageMap[std::string(image->key, image->key_size)] = image;
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

        world->Step(timeStep, 8, 3);
        world->ClearForces();

        window.clear();

        for (int i = 0; i < (int)platforms.size(); i += 1) {
            Platform *platform = platforms[i];
            window.draw(platform->sprite);
        }

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
    platform->sprite.setTextureRect(sf::IntRect(imageInfo->x, spritesheet.getSize().y - imageInfo->y - imageInfo->height, imageInfo->width, imageInfo->height));
    platform->sprite.setOrigin(imageInfo->anchor_x, imageInfo->anchor_y);
    platform->sprite.setPosition(pos.x, pos.y);
    platform->sprite.setScale(fromPixels(size.x / fromPixels(imageInfo->width)), fromPixels(size.y / fromPixels(imageInfo->height)));

    platforms.push_back(platform);
}

void MainWindow::initPlayer(int index, b2Vec2 pos)
{

}
