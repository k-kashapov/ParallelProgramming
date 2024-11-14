#include <iostream>
#include <unordered_map>
#include <vector>
#include <list>
#include <SFML/Graphics.hpp>
#include <omp.h>

const int   WIN_WIDTH  = 1280;
const int   WIN_HEIGHT = 720;

const int   MAP_WIDTH  = 1000;
const int   MAP_HEIGHT = 1000;
const int   PADDING    = 250;
const float FILL       = 0.3;

const float CAMERA_ZOOM_SPD = 1.01;
const float CAMERA_SPEED = 270;
const float CAMERA_SPD_MUL = 3;

const sf::Int32 UPDATE_TIME_MS = 0;

template <>
struct std::hash<sf::Vector2i> {
    std::size_t operator()(const sf::Vector2i& v) const {
        using std::hash;
        return hash<int>()(v.x) ^ (hash<int>()(v.y) << 1);
    }
};

class WorldState {
private:
    std::vector<std::vector<bool>> field;
    std::list<sf::Vector2i> cells;
    std::unordered_map<sf::Vector2i, std::list<sf::Vector2i>::iterator> map;

    size_t countNeighbours(size_t x, size_t y) {
        return field[x+1][y] + field[x-1][y] + field[x][y-1] + field[x][y+1]
                + field[x+1][y+1] + field[x+1][y-1] + field[x-1][y+1] + field[x-1][y-1];
    }

    void removeCell(size_t x, size_t y) {
        field[x][y] = false;
        auto found = map.find(sf::Vector2i(x, y));
        if (found != map.end()) {
            cells.erase(found->second);
        }
    }

    void flipCell(sf::Vector2i cell) {
        if (field[cell.x][cell.y]) {
            removeCell(cell.x, cell.y);
        } else {
            AddCell(cell.x, cell.y);
        }
    }

    void updateCells() {
        // std::cout << "update cells\n";
        std::vector<sf::Vector2i> flips{};
        #pragma omp parallel firstprivate(flips)
        {
            #pragma omp for schedule(static)
            for (size_t x = 1; x < (field.size() - 1); x++) {
                for (size_t y = 1; y < field[0].size() - 1; y++) {
                    size_t neigh = countNeighbours(x, y);

                    if (field[x][y]) {
                        if (neigh > 3 || neigh < 2) {
                            // std::cout << "remove cell at " << x << ", " << y << " as it has " << neigh << " neigh\n";
                            flips.push_back(sf::Vector2i(x, y));
                            continue;
                        }
                    } else if (neigh == 3) {
                        // std::cout << "add cell at " << x << ", " << y << " as it has " << neigh << " neigh\n";
                        flips.push_back(sf::Vector2i(x, y));
                        continue;
                    }
                }
            }

            #pragma omp critical
            for (auto flip : flips) {
                // std::cout << flip.x << ", " << flip.y << "\n";
                flipCell(flip);
            }
        }
    }

public:
    WorldState(size_t x, size_t y)
        : field(std::vector<std::vector<bool>>(x, std::vector<bool>(y))), cells()
    {}

    void AddCell(size_t x, size_t y) {
        // std::cout << "create cell at " << x << ", " << y << std::endl;
        if (field.at(x).at(y)) return;

        field.at(x).at(y) = true;
        cells.push_back(sf::Vector2i(x, y));
        map[sf::Vector2i(x, y)] = std::prev(cells.end());
    }

    bool GetFieldAt(size_t x, size_t y) {
        return field.at(x).at(y);
    }

    void UpdateGame(sf::Time elapsed) {
        static sf::Int32 time_passed = 0;

        time_passed += elapsed.asMilliseconds();
        
        if (time_passed > UPDATE_TIME_MS) {
            time_passed = 0;
            updateCells();
        }
    }

    const std::list<sf::Vector2i>& GetCells() {
        return cells;
    }
};

int main(int argc, const char **argv)
{
    WorldState ws(MAP_WIDTH, MAP_HEIGHT);

    float CELL_SIZE = 3.0;

    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT * FILL; i++) {
        size_t x = rand() % (MAP_WIDTH - PADDING * 2) + PADDING;
        size_t y = rand() % (MAP_HEIGHT - PADDING * 2) + PADDING;
        ws.AddCell(x, y);
    }

    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT, 1), "Game of Life");
    sf::CircleShape shape(CELL_SIZE / 2);

    sf::Vector2f center = sf::Vector2f(float(MAP_WIDTH) / 2, float(MAP_HEIGHT) / 2);
    sf::Vector2f win_center = sf::Vector2f(float(WIN_WIDTH) / 2, float(WIN_HEIGHT) / 2);

    // sf::Text fps;
    // fps.setFillColor(sf::Color::Red);
    // fps.setCharacterSize(24);

    // window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(false);

    sf::Clock clock;

    int i = 0;
    sf::Int64 fpsTime = 0;

    while (window.isOpen()) {
        sf::Time elapsed = clock.restart();
        fpsTime += elapsed.asMicroseconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear();

        float spd = CAMERA_SPEED / CELL_SIZE;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            spd *= CAMERA_SPD_MUL;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            center.y -= spd * elapsed.asSeconds();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            center.y += spd * elapsed.asSeconds();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            center.x -= spd * elapsed.asSeconds();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            center.x += spd * elapsed.asSeconds();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
            CELL_SIZE *= CAMERA_ZOOM_SPD;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
            CELL_SIZE /= CAMERA_ZOOM_SPD;
        }

        // std::cout << "update start\n";
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        ws.UpdateGame(elapsed);
        // }
        // std::cout << "update end\n";

        if (ws.GetCells().size() == 0) {
            std::cout << "All dead\n";
            window.close();
        }

        for (auto cell : ws.GetCells()) {
            shape.setPosition(CELL_SIZE * (sf::Vector2f(cell.x, cell.y) - center) + win_center);
            window.draw(shape);
        }

        i++;

        if (i > 20) {
            std::cout << '\r' << (1000000.0 * 20 / fpsTime) << "            ";
            std::flush(std::cout);
            fpsTime = 0;
            i = 0;
        }
        // window.draw(fps);

        window.display();
    }

    return 0;
}
