#pragma once

#include "boundingBox.h"
#include "rtObjGroup.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <algorithm>
#include <vector>

class bvh_node
{
public:
	bvh_node(std::vector<rtObject*> myObjects, int AXIS);
	bvh_node(boundingBox* bounding_box);
	bool hit(glm::vec3 eye, glm::vec3 dir, float& record);

private:
	boundingBox* bbox;
	bvh_node* left_node;
	bvh_node* right_node;
};
