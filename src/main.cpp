#include "cli/cli.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        return envctl::run_cli(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
