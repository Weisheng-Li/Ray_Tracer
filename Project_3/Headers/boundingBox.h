#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class boundingBox
{
public:
	boundingBox();
	boundingBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);
	void set_boundingBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

	bool testIntersection(glm::vec3 eye, glm::vec3 dir, float& rec);
	static boundingBox* combine(boundingBox* bbox1, boundingBox* bbox2);

private:
	// each bounding box is defined
	// by 6 axis aligned surface
	float x_min;
	float x_max;
	float y_min;
	float y_max;
	float z_min;
	float z_max;
};




