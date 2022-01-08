
#pragma once

#include "pch.h"
#include "material.h"

namespace OpenGL {

    // Structs padded to a size of glm::vec4 (16 bytes) for GPU buffer alignment.

    struct alignas(16) Sphere {
        glm::vec3 position;
        float radius;

        Material material;
    };

    struct alignas(16) AABB {
        glm::vec4 minimum;
        glm::vec4 maximum;

        Material material;
    };

}