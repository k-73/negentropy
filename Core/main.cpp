#include "Main/Application.hpp"
#include <iostream>
#include <stdexcept>

int main() noexcept {
    try {
        Application app;
        app.Run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return -1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return -2;
    }
}
