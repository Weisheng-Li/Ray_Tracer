#include "sphere.h"

//constructor given  center, radius, and material
sphere::sphere(glm::vec3 c, float r, int m, scene* s) : rtObject(s)
{
	center = c;
	radius = r;
	matIndex = m;
	myScene = s;

	bbox.set_boundingBox(center.x - radius, center.x + radius, center.y - radius, center.y + radius,
		center.z - radius, center.z + radius);
}

float sphere::testIntersection(glm::vec3 eye, glm::vec3 dir)
{
	//see the book for a description of how to use the quadratic rule to solve
	//for the intersection(s) of a line and a sphere, implement it here and
	//return the minimum positive distance or 9999999 if none

	// based on textbook P77 equations, eye:e, dir:d, center:c, radius:r
	float epsilon = 0.0001f;
	dir = glm::normalize(dir);

	float discriminant = pow(glm::dot(dir, (eye - center)), 2.0f) 
		 - glm::dot(dir, dir) * (glm::dot(eye - center, eye - center) - radius*radius);

	// if discriminant less than or equal to 0, no intersection
	if (discriminant < 0 || abs(discriminant - 0.0f) < epsilon) return 9999999.0f; 

	float t_plus = (glm::dot(-dir, eye - center) + glm::sqrt(discriminant)) / dot(dir, dir);
	float t_minus = (glm::dot(-dir, eye - center) - glm::sqrt(discriminant)) / dot(dir, dir);

	if (t_minus > epsilon && t_minus < 9999999.0f)
		return t_minus;
	else if (t_plus > epsilon && t_plus < 9999999.0f)
		return t_plus;
	else
		return 9999999.0f;
}

glm::vec3 sphere::getNormal(glm::vec3 eye, glm::vec3 dir)
{
	//once you can test for intersection,
	//simply add (distance * view direction) to the eye location to get surface location,
	//then subtract the center location of the sphere to get the normal out from the sphere
	glm::vec3 normal;

	dir = glm::normalize(dir);
	float dist = testIntersection(eye, dir);
	if (dist == 9999999)
		return glm::vec3(0, 0, 0);

	glm::vec3 intersec_p = eye + dist * dir;

	// the normal is always point ourside of the box
	normal = glm::normalize(intersec_p - center);
	
	return normal;
}

glm::vec2 sphere::getTextureCoords(glm::vec3 eye, glm::vec3 dir)
{
	float PI = 3.14159265f;

	//find the normal as in getNormal
	glm::vec3 normal = getNormal(eye, dir);
	if (normal == glm::vec3(0.0f, 0.0f, 0.0f))
		return glm::vec3(1.0f);

	//use these to find spherical coordinates
	glm::vec3 x(1, 0, 0);
	glm::vec3 y(0, 1, 0);
	glm::vec3 z(0, 0, 1);

	//phi is the angle down from z
	//theta is the angle from x curving toward y
	//hint: angle between two vectors is the acos() of the dot product

	//find phi using normal and z
	float phi = acos(glm::dot(normal, z));

	//find the x-y projection of the normal
	// formula: P_u(v) = <v, basis1>basis1 + <v, basis2>basis2
	// assuming that we have a orthonormal basis of U
	glm::vec3 P_xy = glm::dot(normal, x) * x + glm::dot(normal, y) * y;
	assert(abs(P_xy.z - 0.0f) < 0.0001f);

	//find theta using the x-y projection and x
	float theta = acos(glm::dot(P_xy, x));

	//if x-y projection is in quadrant 3 or 4, then theta=2*PI-theta
	if (P_xy.y <= 0.0f)
		theta = 2 * PI - theta;

	//return coordinates scaled to be between 0 and 1
	glm::vec2 coords;
	coords.x = theta / (2 * PI);
	coords.y = phi / PI;
	assert(0.0f <= coords.x && coords.x <= 1.0f);
	assert(0.0f <= coords.y && coords.y <= 1.0f);

	return coords;
}

glm::vec3 sphere::getCenter()
{
	return center;
}