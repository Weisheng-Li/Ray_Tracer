#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//struct that holds a 3D location and an RGB color
struct light
{
	glm::vec3 color;
	std::vector<glm::vec3> positions;
};