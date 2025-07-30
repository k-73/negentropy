#include "Main/Application.hpp"
#include <iostream>
#include <stdexcept>

int main() noexcept {
    enum class ExitCode : int {
        Success = 0,
        Error = -1,
        UnknownError = -2
    };

    try {
        Application app;
        app.Run();
        return static_cast<int>(ExitCode::Success);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return static_cast<int>(ExitCode::Error);
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return static_cast<int>(ExitCode::UnknownError);
    }
}
