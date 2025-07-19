#include "okay.hpp"

#include <iostream>

void OkayEngine::initialize() {
    std::cout << "Okay Engine initialized." << std::endl;
}

void OkayEngine::run() {
    std::cout << "Okay Engine is running." << std::endl;
}

void OkayEngine::shutdown() {
    std::cout << "Okay Engine shutdown." << std::endl;
}