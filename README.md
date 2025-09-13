# Physics engine
to run the program you need SDL2 library in your C:drive(recommended)
https://github.com/libsdl-org/SDL/releases 
download (SDL2-devel-2.x.x-mingw.tar.gz)
extract in C:drive copy SDL2.dll in your project directory where physics_sim.cpp exists from bin folder 
(choose compiler compatible architecture SDL2.dll for 32-bit compiler from 32-bit bin folder or 64-bit from bin folder for 64-bit compiler)
open terminal in the directory of physics_sim.cpp and compile the code by running 
g++ -o physics_sim.exe physics_sim.cpp -I"C:/SDL2/SDL2-2.x.x/i686-w64-mingw32/include" -L"C:/SDL2/SDL2-2.x.x/i686-w64-mingw32/lib" -lmingw32 -lSDL2main -lSDL2'  (for 32-bit compiler/SDL2 library)
'g++ -o physics_sim.exe physics_sim.cpp -I"C:/SDL2/SDL2-2.x.x/x86_64-w64-mingw32/include" -L"C:/SDL2/SDL2-2.x.x/x86_64-w64-mingw32/lib" -lmingw32 -lSDL2main -lSDL2'  (for 64-bit compiler/SDL2 library)
run the physics_sim.exe
