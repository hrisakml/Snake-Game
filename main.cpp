#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <random>
#include <string>
#include <deque>
#include <string>
const std::string ASSET_DIR = "assets/";
enum class State {Menu, Playing, GameOver};
struct Bomb{
    sf::Vector2i cell;
    float timeLeft;
};

constexpr float BOMB_LIFETIME = 6.f;
constexpr float BOMB_INTERVAL = 5.f;    
constexpr size_t MAX_BOMBS = 3;

constexpr int speed = 3;
constexpr int CELL = 30;
constexpr int COLS = 600/CELL;
constexpr int ROWS = 600/CELL;
constexpr int GAP = CELL/speed;

void centerText(sf::Text& t, float x, float y){
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    t.setPosition({x, y});
}

int main(){
    sf::RenderWindow window (sf::VideoMode({600, 600}), "Game", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile(std::string(ASSET_DIR) + "font.ttf")) return 1;

    sf::Text scoreText(font, "Score: 0", 20);
    scoreText.setPosition({10.f, 0.f});
    sf::Text title(font, "", 60);
    sf::Text info(font, "", 26);

    sf::Text bestScore(font, "Best: 0", 20);
    bestScore. setPosition({500.f, 0.f});

    sf::CircleShape bombShape(CELL / 2.f - 4.f);
    bombShape.setFillColor(sf::Color(40, 40, 40));
    bombShape.setOutlineColor(sf::Color(255, 140, 0));
    bombShape.setOutlineThickness(2.f);

    sf::RectangleShape tile({CELL, CELL});
    sf::RectangleShape shape({CELL, CELL});
    sf::CircleShape foodShape (CELL / 2.f - 3.f);
    foodShape.setFillColor(sf::Color::Red);

    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(std::string(ASSET_DIR) + "nom.wav")) return 1;

    sf::SoundBuffer expbuffer;
    if (!expbuffer.loadFromFile(std::string(ASSET_DIR) + "explode.wav")) return 1;

    sf::Sound sound(buffer);
    sound.setVolume(100.f);

    sf::Sound explosion(expbuffer);
    explosion.setVolume(100.f);

    //Game Data
    sf::Vector2i d(0, 0);
    sf::Vector2i pos(10 * CELL, 10 * CELL);
    std::deque<sf::Vector2i> turns, trail;
    int length = 3;
    int score = 0;
    int best = 0;
    State state = State::Menu;

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> distX(1, COLS-2);
    std::uniform_int_distribution<int> distY(1, ROWS-2);
    sf::Vector2i food(0, 0);

    std::vector<Bomb> bombs;
    float bombTimer = 0.f;
    sf::Clock clock;

    trail.push_front(pos);

    auto segPos = [&](int i){
        size_t idx = std::min<size_t>(i * GAP, trail.size() - 1);
        return trail[idx];
    };

    auto onSnake = [&](sf::Vector2i c){
        for (int i=0; i<length; i++){
            if (segPos(i)/ CELL == c) return true;
        }
        return false;
    };

    auto bombAt = [&](sf::Vector2i c) {
        for (const Bomb& b : bombs)
            if (b.cell == c) return true;
        return false;
    };

    auto spawnFood = [&]() {
        sf::Vector2i headCell(pos.x / CELL, pos.y / CELL);
        do {
            food = {distX(rng), distY(rng)};
        }while (onSnake(food) || bombAt(food));
    };

    auto spawnBomb = [&]() {
        sf::Vector2i headCell = pos / CELL;
        for (int tries = 0; tries < 50; tries++) {
            sf::Vector2i c(distX(rng), distY(rng));
            int dist = std::abs(c.x - headCell.x) + std::abs(c.y - headCell.y);
            if (dist < 5 || c == food || onSnake(c) || bombAt(c)) continue;
            bombs.push_back({c, BOMB_LIFETIME});
            return;
        }
    };

    auto resetGame = [&]() {
        pos = {10 * CELL, 10 * CELL};
        d = {0, 0};
        turns.clear();
        trail.clear();
        trail.push_front(pos);
        length = 3;
        score = 0;
        scoreText.setString("Score: 0");
        spawnFood();
        bombs.clear();
        bombTimer = 0.f;
    };
    resetGame();

    while (window.isOpen())
    {
        float dt = std::min(clock.restart().asSeconds(), 0.1f);
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()){
                window.close();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()){
                using K = sf::Keyboard::Key;
                if (state == State::Menu){
                    if (key->code == K::Enter || key->code == K::Space) {
                        resetGame();
                        state = State::Playing;
                    }
                }else if (state == State::GameOver){
                    if (key->code == K::Space) {
                        resetGame();
                        state = State::Playing;
                    }
                    if (key->code == K::Escape) {
                        resetGame();
                        state = State::Menu;
                    }
                }else{
                    sf::Vector2i wanted(0, 0);
                    if (key->code == K::W || key->code == K::Up) wanted = {0, -1};
                    if (key->code == K::A || key->code == K::Left) wanted = {-1, 0};
                    if (key->code == K::S || key->code == K::Down) wanted = {0, 1};
                    if (key->code == K::D || key->code == K::Right) wanted = {1, 0};

                    if (wanted != sf::Vector2i(0,0)){
                        sf::Vector2i last = turns.empty() ? d : turns.back();
                        if (wanted != last && wanted != -last && turns.size() < 2) turns.push_back(wanted);
                    }
                }
            }
        }

        //Update
        if (state == State::Playing){
            bool aligned = pos.x % CELL == 0 && pos.y % CELL == 0;

            if (aligned && !turns.empty()) {
                d = turns.front();
                turns.pop_front();
            }

            pos += d * speed;

            if (d != sf::Vector2i(0, 0)) {
                bombTimer += dt;
                if (bombTimer >= BOMB_INTERVAL) {
                    bombTimer = 0.f;
                    if (bombs.size() < MAX_BOMBS) spawnBomb();
                }

                bool expired = false;
                for (Bomb& b : bombs) {
                    b.timeLeft -=dt;
                    if (b.timeLeft <= 0.f) expired = true;
                }
                if (expired) explosion.play();

                bombs.erase(std::remove_if(bombs.begin(), bombs.end(),
                [](const Bomb& b) { return b.timeLeft <= 0.f; }),
                bombs.end());
            }
 

            for (const Bomb& b : bombs) {
                if (std::abs(pos.x - b.cell.x * CELL) < CELL - 6 &&
                    std::abs(pos.y - b.cell.y * CELL) < CELL - 6)
                    { explosion.play(); state = State::GameOver; break; }
            }

            if (d != sf::Vector2i(0, 0)) {
                trail.push_front(pos);
                size_t maxSize = (length - 1) * GAP + 1;
                while (trail.size() > maxSize) trail.pop_back();
            }

            if (pos.x < CELL || pos.x > (COLS - 2) * CELL ||
                pos.y < CELL || pos.y > (ROWS - 2) * CELL)
                state = State::GameOver;

            if (state == State::Playing && pos.x % CELL == 0 && pos.y % CELL == 0) {
                sf::Vector2i headCell = pos / CELL;

                if (headCell == food) {
                    length++;
                    score++;
                    sound.play();
                    if (score>best) best = score;
                    scoreText.setString("Score: " + std::to_string(score));
                    bestScore.setString("Best: " + std::to_string(best));
                    spawnFood();
                }

                for (int i = 1; i < length; i++) {
                    if (i * GAP < (int)trail.size() && segPos(i) / CELL == headCell) {
                        state = State::GameOver;
                        break;
                    }
                }
            }
        }

        //Draw
        window.clear();

        for (int i=0; i<ROWS; i++){
            for (int j=0; j<COLS; j++){
                tile.setFillColor((i==0 || i==ROWS-1 || j==0 || j==COLS-1) 
                                ? sf::Color(47, 150, 21) 
                                : ((i+j)%2==0 ? sf::Color(170, 215, 81) : sf::Color(255, 255, 255)));
                tile.setPosition({j*CELL*1.f, i*CELL*1.f});
                window.draw(tile);
            }
        }

        if (state != State::Menu) {
            foodShape.setPosition({food.x * CELL + 3.f, food.y * CELL + 3.f});
            window.draw(foodShape);

            for (const Bomb& b : bombs) {
                if (b.timeLeft < 1.5f && (int)(b.timeLeft * 6) % 2 == 0) continue;
                bombShape.setPosition({b.cell.x * CELL + 4.f, b.cell.y * CELL + 4.f});
                window.draw(bombShape);
            }

            for (int i = length - 1; i >= 0; i--) {
                shape.setFillColor(i == 0 ? sf::Color(30, 60, 200) : sf::Color(60, 110, 255));
                // blink during the last 1.5 seconds as a warning
                shape.setPosition(sf::Vector2f(segPos(i)));
                window.draw(shape);
            }
            window.draw(scoreText);
            window.draw(bestScore);
        }
        if (state != State::Playing) {
            sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            if (state == State::Menu) {
                title.setString("SNAKE");
                info.setString("Press Enter to start");
            } else {
                title.setString("GAME OVER");
                info.setString("Score: " + std::to_string(score) +
                               "\nSpace: restart   Esc: menu");
            }
            float cx = window.getSize().x / 2.f;
            float cy = window.getSize().y / 2.f;
            centerText(title, cx, cy - 60.f);
            centerText(info, cx, cy + 30.f);
            window.draw(title);
            window.draw(info);
        }

        window.display();
    }
}