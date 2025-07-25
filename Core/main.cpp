#include "Main/Application.hpp"
#include <iostream>
#include <stdexcept>

int main(int, char**) {
    try {
        Application app;

        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
