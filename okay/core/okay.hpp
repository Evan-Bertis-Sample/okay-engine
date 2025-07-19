#ifndef __OKAY_H__
#define __OKAY_H__

#include <iostream>
#include <string>

class OkayEngine {
   public:
    static void initialize() {
        std::cout << "Okay Engine initialized." << std::endl;
    }
    static void run() {
        std::cout << "Okay Engine is running." << std::endl;
    }
    static void shutdown() {
        std::cout << "Okay Engine shutdown." << std::endl;
    }
};

#endif  // __OKAY_H__