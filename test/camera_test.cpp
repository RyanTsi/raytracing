#include "camera.h"
#include <glm/glm.hpp>
#include <iostream>

int main () {
    camera c0;
    glm::mat4 view = c0.getView();
    for(int i = 0; i < 4; i ++) {
        for(int j = 0; j < 4; j ++) {
            std::cout << view[i][j] << " ";
        }
        std::cout << "\n";
    }    
}