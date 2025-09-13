#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>

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
    
    Ball(double x, double y, double vx, double vy, double r, double m, SDL_Color c)
        : position(x, y), velocity(vx, vy), radius(r), mass(m), color(c) {}
    
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
        Vector2D relativeVelocity = velocity -other.velocity;
        
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
        
        // Draw filled circle using efficient algorithm
        int r = (int)radius;
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r) {
                    SDL_RenderDrawPoint(renderer, 
                                      (int)position.x + dx, 
                                      (int)position.y + dy);
                }
            }
        }
    }
};

class PhysicsSimulation {
private:
    Ball ball1;
    Ball ball2;
    int windowWidth;
    int windowHeight;
    
public:
    PhysicsSimulation(int width, int height) 
        : windowWidth(width), windowHeight(height),
          ball1(150, 150, 200, 150, 30, 1.0, {255, 0, 0, 255}),    // Red ball
          ball2(600, 400, -180, -120, 25, 0.8, {0, 100, 255, 255}) // Blue ball
    {
        std::cout << "Physics Simulation Initialized!" << std::endl;
        std::cout << "Ball 1 - Initial velocity: (" << ball1.velocity.x << ", " << ball1.velocity.y << ")" << std::endl;
        std::cout << "Ball 2 - Initial velocity: (" << ball2.velocity.x << ", " << ball2.velocity.y << ")" << std::endl;
    }
    
    void update(double deltaTime) {
        // Update ball positions with constant velocity
        ball1.update(deltaTime);
        ball2.update(deltaTime);
        
        // Handle wall collisions with perfect bouncing
        ball1.bounceOffWalls(windowWidth, windowHeight);
        ball2.bounceOffWalls(windowWidth, windowHeight);
        
        // Handle ball-to-ball collision with perfect momentum conservation
        if (ball1.isCollidingWith(ball2)) {
            ball1.resolveCollision(ball2);
            std::cout << "Collision! Ball speeds: " 
                      << ball1.velocity.length() << ", " 
                      << ball2.velocity.length() << std::endl;
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
        
        // Render balls
        ball1.render(renderer);
        ball2.render(renderer);
        
        SDL_RenderPresent(renderer);
    }
    
    void reset() {
        // Reset to initial state with fresh velocities
        ball1 = Ball(150, 150, 200, 150, 30, 1.0, {255, 0, 0, 255});
        ball2 = Ball(600, 400, -180, -120, 25, 0.8, {0, 100, 255, 255});
        std::cout << "Simulation reset!" << std::endl;
    }
    
    double getTotalEnergy() const {
        // Calculate total kinetic energy for debugging
        double ke1 = 0.5 * ball1.mass * (ball1.velocity.x * ball1.velocity.x + ball1.velocity.y * ball1.velocity.y);
        double ke2 = 0.5 * ball2.mass * (ball2.velocity.x * ball2.velocity.x + ball2.velocity.y * ball2.velocity.y);
        return ke1 + ke2;
    }
};

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Everlasting Ball Physics - Perfect Conservation",
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
    PhysicsSimulation simulation(WINDOW_WIDTH, WINDOW_HEIGHT);
    
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
    
    std::cout << "\nControls:" << std::endl;
    std::cout << "SPACE - Reset simulation" << std::endl;
    std::cout << "ESC   - Exit" << std::endl;
    std::cout << "\nInitial total energy: " << simulation.getTotalEnergy() << std::endl;
    
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
                    running = false;
                } else if (event.key.keysym.sym == SDLK_SPACE) {
                    simulation.reset();
                }
            }
        }
        
        // Update physics with precise timing
        simulation.update(deltaTime);
        
        // Render
        simulation.render(renderer);
        
        // Optional: Print energy conservation check every few seconds
        static int frameCount = 0;
        if (++frameCount % (int)(TARGET_FPS * 3) == 0) {
            std::cout << "Energy check: " << simulation.getTotalEnergy() << std::endl;
        }
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Simulation ended!" << std::endl;
    return 0;
}