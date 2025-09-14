#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

struct Vector2D {
    double x, y;
    
    Vector2D(double x = 0, double y = 0) : x(x), y(y) {}
    
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }
    
    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }
    
    Vector2D operator*(double scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
    
    double dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }
    
    double length() const {
        return sqrt(x * x + y * y);
    }
    
    Vector2D normalize() const {
        double len = length();
        if (len == 0) return Vector2D(0, 0);
        return Vector2D(x / len, y / len);
    }
};

class Ball {
public:
    Vector2D position;
    Vector2D velocity;
    double radius;
    double mass;
    SDL_Color color;
    int id;
    
    Ball(int ballId, double x, double y, double vx, double vy, double r, double m, SDL_Color c)
        : id(ballId), position(x, y), velocity(vx, vy), radius(r), mass(m), color(c) {}
    
    void update(double dt) {
        // Pure constant velocity motion - no forces applied
        position = position + velocity * dt;
    }
    
    void bounceOffWalls(int windowWidth, int windowHeight) {
        // Perfect elastic collision with walls
        if (position.x - radius <= 0) {
            position.x = radius;
            velocity.x = -velocity.x;  // Perfect reflection
        } else if (position.x + radius >= windowWidth) {
            position.x = windowWidth - radius;
            velocity.x = -velocity.x;  // Perfect reflection
        }
        
        if (position.y - radius <= 0) {
            position.y = radius;
            velocity.y = -velocity.y;  // Perfect reflection
        } else if (position.y + radius >= windowHeight) {
            position.y = windowHeight - radius;
            velocity.y = -velocity.y;  // Perfect reflection
        }
    }
    
    bool isCollidingWith(const Ball& other) const {
        Vector2D distance = position - other.position;
        return distance.length() <= (radius + other.radius);
    }
    
    void resolveCollision(Ball& other) {
        Vector2D distance = position - other.position;
        double d = distance.length();
        
        // Avoid division by zero
        if (d == 0) {
            distance = Vector2D(1, 0);
            d = 1;
        }
        
        // Normalize collision vector
        Vector2D normal = distance * (1.0 / d);
        
        // Separate overlapping balls
        double overlap = (radius + other.radius) - d;
        double totalMass = mass + other.mass;
        
        position = position + normal * (overlap * other.mass / totalMass);
        other.position = other.position - normal * (overlap * mass / totalMass);
        
        // Calculate relative velocity
        Vector2D relativeVelocity = velocity - other.velocity;
        
        // Calculate collision impulse using conservation of momentum
        double velocityAlongNormal = relativeVelocity.dot(normal);
        
        // Do not resolve if velocities are separating
        if (velocityAlongNormal > 0) return;
        
        // Perfect elastic collision - no energy loss
        double impulse = 2 * velocityAlongNormal / totalMass;
        
        // Apply impulse to conserve momentum perfectly
        velocity = velocity - normal * (impulse * other.mass);
        other.velocity = other.velocity + normal * (impulse * mass);
    }
    
    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        // Optimized circle drawing
        int r = (int)radius;
        int x = (int)position.x;
        int y = (int)position.y;
        
        // Use a more efficient circle drawing algorithm
        for (int dy = -r; dy <= r; dy++) {
            int width = (int)sqrt(r*r - dy*dy);
            SDL_RenderDrawLine(renderer, x - width, y + dy, x + width, y + dy);
        }
    }
};

class PhysicsSimulation {
private:
    std::vector<Ball> balls;
    int windowWidth;
    int windowHeight;
    int collisionCount;
    int numBalls;
    double minRadius;
    double maxRadius;
    
    SDL_Color generateRandomColor(std::mt19937& rng) {
        // Neon color palette - bright, vibrant, refreshing colors
        std::vector<SDL_Color> neonColors = {
            {0, 255, 255, 255},     // Electric Cyan
            {255, 0, 255, 255},     // Electric Magenta
            {255, 255, 0, 255},     // Electric Yellow
            {0, 255, 0, 255},       // Electric Green
            {255, 64, 255, 255},    // Hot Pink
            {64, 255, 64, 255},     // Lime Green
            {255, 128, 0, 255},     // Electric Orange
            {128, 255, 255, 255},   // Light Cyan
            {255, 128, 255, 255},   // Light Magenta
            {255, 255, 128, 255},   // Light Yellow
            {128, 255, 128, 255},   // Light Green
            {255, 64, 128, 255},    // Pink Neon
            {64, 255, 255, 255},    // Aqua Neon
            {255, 255, 64, 255},    // Bright Yellow
            {128, 128, 255, 255},   // Electric Blue
            {255, 128, 128, 255},   // Light Red
            {192, 255, 64, 255},    // Electric Lime
            {255, 64, 192, 255},    // Hot Pink 2
            {64, 192, 255, 255},    // Sky Blue Neon
            {255, 192, 64, 255}     // Golden Neon
        };
        
        std::uniform_int_distribution<int> colorIndex(0, neonColors.size() - 1);
        return neonColors[colorIndex(rng)];
    }
    
    void initializeBalls() {
        balls.clear();
        collisionCount = 0;
        
        // Create random number generator
        std::random_device rd;
        std::mt19937 rng(rd());
        
        // Distributions for ball properties
        std::uniform_real_distribution<double> radiusDist(minRadius, maxRadius);
        std::uniform_real_distribution<double> massDist(0.5, 1.5);
        std::uniform_real_distribution<double> velocityDist(80.0, 200.0);
        std::uniform_int_distribution<int> signDist(0, 1);
        
        // Calculate grid size based on number of balls
        int gridSize = (int)ceil(sqrt(numBalls));
        if (gridSize * gridSize < numBalls) gridSize++;
        
        const double SPACING_X = (windowWidth - 100) / (gridSize - 1);
        const double SPACING_Y = (windowHeight - 100) / (gridSize - 1);
        
        int ballCount = 0;
        
        // Create balls in a grid pattern with random properties
        for (int row = 0; row < gridSize && ballCount < numBalls; row++) {
            for (int col = 0; col < gridSize && ballCount < numBalls; col++) {
                if (ballCount >= numBalls) break;
                
                double x = 50 + col * SPACING_X;
                double y = 50 + row * SPACING_Y;
                
                // Add small random offset to avoid perfect grid
                std::uniform_real_distribution<double> offsetDist(-15.0, 15.0);
                x += offsetDist(rng);
                y += offsetDist(rng);
                
                // Ensure ball stays within bounds
                double radius = radiusDist(rng);
                x = std::max(radius + 5, std::min(x, windowWidth - radius - 5));
                y = std::max(radius + 5, std::min(y, windowHeight - radius - 5));
                
                // Random velocity
                double vx = velocityDist(rng) * (signDist(rng) ? 1 : -1);
                double vy = velocityDist(rng) * (signDist(rng) ? 1 : -1);
                
                double mass = massDist(rng);
                SDL_Color color = generateRandomColor(rng);
                
                balls.push_back(Ball(ballCount + 1, x, y, vx, vy, radius, mass, color));
                ballCount++;
            }
        }
        
        std::cout << "Created " << balls.size() << " balls!" << std::endl;
    }
    
public:
    PhysicsSimulation(int width, int height, int numberOfBalls, double minR, double maxR) 
        : windowWidth(width), windowHeight(height), collisionCount(0), 
          numBalls(numberOfBalls), minRadius(minR), maxRadius(maxR) {
        initializeBalls();
        
        std::cout << "🔥 CUSTOMIZABLE NEON BALL PHYSICS SIMULATION INITIALIZED! 🔥" << std::endl;
        std::cout << "Total balls: " << balls.size() << std::endl;
        std::cout << "Ball radius range: " << minRadius << " - " << maxRadius << std::endl;
        std::cout << "Possible collision pairs: " << (balls.size() * (balls.size() - 1)) / 2 << std::endl;
        std::cout << "LET THE NEON CHAOS BEGIN! 🌈💥" << std::endl;
    }
    
    void update(double deltaTime) {
        // Update all ball positions with constant velocity
        for (Ball& ball : balls) {
            ball.update(deltaTime);
            ball.bounceOffWalls(windowWidth, windowHeight);
        }
        
        // Handle ball-to-ball collisions (check every pair)
        int frameCollisions = 0;
        for (size_t i = 0; i < balls.size(); i++) {
            for (size_t j = i + 1; j < balls.size(); j++) {
                if (balls[i].isCollidingWith(balls[j])) {
                    balls[i].resolveCollision(balls[j]);
                    frameCollisions++;
                    collisionCount++;
                }
            }
        }
        
        // Only print if there are collisions to avoid spam
        if (frameCollisions > 0) {
            std::cout << "💥 " << frameCollisions << " collisions this frame! Total: " 
                      << collisionCount << std::endl;
        }
    }
    
    void render(SDL_Renderer* renderer) {
        // Clear screen with black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw blue box outline (closed container)
        SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
        for (int i = 0; i < 4; i++) {
            SDL_Rect border = {i, i, windowWidth - 2*i, windowHeight - 2*i};
            SDL_RenderDrawRect(renderer, &border);
        }
        
        // Render all balls
        for (Ball& ball : balls) {
            ball.render(renderer);
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void reset() {
        std::cout << "🔄 RESETTING NEON BALL CHAOS!" << std::endl;
        initializeBalls();
        std::cout << "Fresh neon chaos initiated! 🎯✨" << std::endl;
    }
    
    double getTotalEnergy() const {
        double totalEnergy = 0;
        for (const Ball& ball : balls) {
            double ke = 0.5 * ball.mass * (ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
            totalEnergy += ke;
        }
        return totalEnergy;
    }
    
    void printStats() const {
        std::cout << "\n📊 NEON BALL SIMULATION STATS:" << std::endl;
        std::cout << "Number of balls: " << balls.size() << std::endl;
        std::cout << "Ball size range: " << minRadius << " - " << maxRadius << std::endl;
        std::cout << "Total collisions: " << collisionCount << std::endl;
        std::cout << "Total energy: " << getTotalEnergy() << std::endl;
        
        // Count balls in different speed ranges
        int slow = 0, medium = 0, fast = 0;
        for (const Ball& ball : balls) {
            double speed = ball.velocity.length();
            if (speed < 100) slow++;
            else if (speed < 150) medium++;
            else fast++;
        }
        std::cout << "Speed distribution - Slow(<100): " << slow 
                  << ", Medium(100-150): " << medium 
                  << ", Fast(>150): " << fast << std::endl;
    }
    
    int getCollisionCount() const {
        return collisionCount;
    }
};

int main(int argc, char* argv[]) {
    // Get user input for simulation parameters
    int numberOfBalls;
    double minRadius, maxRadius;
    
    std::cout << "🌈 WELCOME TO THE NEON BALL PHYSICS SIMULATOR! 🌈" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    // Get number of balls
    std::cout << "\n🎱 Enter the number of balls (1-1000): ";
    std::cin >> numberOfBalls;
    
    // Validate number of balls
    if (numberOfBalls < 1) {
        numberOfBalls = 1;
        std::cout << "⚠️  Minimum 1 ball set!" << std::endl;
    } else if (numberOfBalls > 1000) {
        numberOfBalls = 1000;
        std::cout << "⚠️  Maximum 1000 balls set (for performance)!" << std::endl;
    }
    
    // Get radius range
    std::cout << "\n🔵 Enter minimum ball radius (5-50): ";
    std::cin >> minRadius;
    std::cout << "🔴 Enter maximum ball radius (" << minRadius << "-100): ";
    std::cin >> maxRadius;
    
    // Validate radius range
    if (minRadius < 5) minRadius = 5;
    if (minRadius > 50) minRadius = 50;
    if (maxRadius < minRadius) maxRadius = minRadius + 5;
    if (maxRadius > 100) maxRadius = 100;
    
    std::cout << "\n✅ SIMULATION CONFIGURED:" << std::endl;
    std::cout << "   Balls: " << numberOfBalls << std::endl;
    std::cout << "   Radius Range: " << minRadius << " - " << maxRadius << std::endl;
    std::cout << "   Possible Collisions: " << (numberOfBalls * (numberOfBalls - 1)) / 2 << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    // Calculate window size based on number of balls
    int WINDOW_WIDTH = std::max(800, (int)(400 + sqrt(numberOfBalls) * 60));
    int WINDOW_HEIGHT = std::max(600, (int)(300 + sqrt(numberOfBalls) * 45));
    
    // Cap window size for very large numbers
    WINDOW_WIDTH = std::min(WINDOW_WIDTH, 1920);
    WINDOW_HEIGHT = std::min(WINDOW_HEIGHT, 1080);
    
    std::string windowTitle = "🔥 NEON PHYSICS: " + std::to_string(numberOfBalls) + 
                             " BALLS (Size: " + std::to_string((int)minRadius) + 
                             "-" + std::to_string((int)maxRadius) + ") 🔥";
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cout << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Create physics simulation
    PhysicsSimulation simulation(WINDOW_WIDTH, WINDOW_HEIGHT, numberOfBalls, minRadius, maxRadius);
    
    // Game loop variables
    bool running = true;
    SDL_Event event;
    
    // High precision timing
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;
    double deltaTime;
    
    // FPS tracking
    const double TARGET_FPS = 60.0;
    const double FRAME_TIME = 1.0 / TARGET_FPS;
    
    std::cout << "\n🎮 CONTROLS:" << std::endl;
    std::cout << "SPACE - Reset simulation (new random chaos!)" << std::endl;
    std::cout << "S     - Show detailed statistics" << std::endl;
    std::cout << "ESC   - Exit simulation" << std::endl;
    std::cout << "\n🚀 NEON CHAOS ACTIVATED!" << std::endl;
    std::cout << "Initial total energy: " << simulation.getTotalEnergy() << std::endl;
    
    // Main game loop
    while (running) {
        // Calculate high-precision delta time
        lastTime = currentTime;
        currentTime = SDL_GetPerformanceCounter();
        deltaTime = (double)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        
        // Cap delta time to prevent large jumps
        if (deltaTime > FRAME_TIME * 2) {
            deltaTime = FRAME_TIME;
        }
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    std::cout << "\n🚪 ESCAPE pressed - Exiting simulation..." << std::endl;
                    std::cout << "Thanks for the neon chaos experience! 🌈✨" << std::endl;
                    running = false;
                } else if (event.key.keysym.sym == SDLK_SPACE) {
                    simulation.reset();
                } else if (event.key.keysym.sym == SDLK_s) {
                    simulation.printStats();
                }
            }
        }
        
        // Update physics with precise timing
        simulation.update(deltaTime);
        
        // Render
        simulation.render(renderer);
        
        // Print energy conservation check every 10 seconds
        static int frameCount = 0;
        if (++frameCount % (int)(TARGET_FPS * 10) == 0) {
            std::cout << "🔋 Energy conservation check: " << simulation.getTotalEnergy() 
                      << " | Total collisions: " << simulation.getCollisionCount() << std::endl;
        }
    }
    
    // Final stats
    simulation.printStats();
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "\n🎯 NEON BALL SIMULATION ENDED!" << std::endl;
    std::cout << "Thanks for experiencing the chaos! ✨💫🔥" << std::endl;
    return 0;
}