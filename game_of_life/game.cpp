#include <iostream>
#include <vector>
#include <list>
#include <SFML/Graphics.hpp>

const int   WIN_WIDTH  = 800;
const int   WIN_HEIGHT = 600;

const int   MAP_WIDTH  = 800;
const int   MAP_HEIGHT = 600;
const float CELL_SIZE  = 10;

const float CAMERA_SPEED = 10;
const float CAMERA_SPD_MUL = 3;

const sf::Int32 UPDATE_TIME_MS = 500;

class WorldState {
private:
    std::vector<std::vector<bool>> field;
    std::list<sf::Vector2i> cells;
    std::vector<sf::Vector2i> flips;

    size_t countNeighbours(size_t x, size_t y) {
        return field[x+1][y] + field[x-1][y] + field[x][y-1] + field[x][y+1]
                + field[x+1][y+1] + field[x+1][y-1] + field[x-1][y+1] + field[x-1][y-1];
    }

    void removeCell(size_t x, size_t y) {
        field[x][y] = false;
        auto found = std::find(cells.begin(), cells.end(), sf::Vector2i(x, y));
        if (found != cells.end()) {
            cells.erase(found);
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
        #pragma omp for
        for (size_t x = 1; x < field.size() - 1; x++) {
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

                // if (neigh > 0) {
                //     if (field[x][y]) {
                //         std::cout << "cell at " << x << ", " << y << " keeps living\n";
                //     } else {
                //         std::cout << "cell at " << x << ", " << y << " is stil dead\n";
                //     }
                // }
            }
        }

        for (auto flip : flips) {
            flipCell(flip);
        }

        flips.clear();
    }

public:
    WorldState(size_t x, size_t y)
        : field(std::vector<std::vector<bool>>(x, std::vector<bool>(y))), cells()
    {}

    void AddCell(size_t x, size_t y) {
        field.at(x).at(y) = true;
        cells.push_back(sf::Vector2i(x, y));
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

    ws.AddCell(MAP_WIDTH / 2, MAP_HEIGHT / 2);
    ws.AddCell(MAP_WIDTH / 2, MAP_HEIGHT / 2 + 1);
    ws.AddCell(MAP_WIDTH / 2, MAP_HEIGHT / 2 + 2);
    ws.AddCell(MAP_WIDTH / 2, MAP_HEIGHT / 2 + 3);
    ws.AddCell(MAP_WIDTH / 2, MAP_HEIGHT / 2 + 4);

    for (int i = 0; i < 3000; i++) {
        ws.AddCell(rand() % 200 + MAP_WIDTH / 2 - 100, rand() % 100 + MAP_HEIGHT / 2 - 50);
    }

    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT, 1), "Game of Life");
    sf::CircleShape shape(CELL_SIZE / 2);

    sf::Vector2f center = sf::Vector2f(float(MAP_WIDTH) / 2, float(MAP_HEIGHT) / 2);
    sf::Vector2f win_center = sf::Vector2f(float(WIN_WIDTH) / 2, float(WIN_HEIGHT) / 2);

    window.setFramerateLimit(50);

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time elapsed = clock.restart();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear();

        float spd = CAMERA_SPEED;

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

        // std::cout << "update start\n";
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        ws.UpdateGame(elapsed);
        // }
        // std::cout << "update end\n";

        for (auto cell : ws.GetCells()) {
            shape.setPosition(CELL_SIZE * (sf::Vector2f(cell.x, cell.y) - center) + win_center);
            window.draw(shape);
        }

        window.display();
    }

    return 0;
}
