#include "ConsoleRunner.hpp"

#ifdef _WIN32
#include "Win32Gui.hpp"
#endif

#include <string>

int main(int argc, char** argv) {
#ifdef _WIN32
    if (argc > 1 && (std::string(argv[1]) == "--console" || std::string(argv[1]) == "--verbose")) {
        return rvc_simul::runConsoleSimulator(argc, argv);
    }
    return rvc_simul::runWindowsGuiSimulator();
#else
    return rvc_simul::runConsoleSimulator(argc, argv);
#endif
}
