#include "triangle.h"

//constructor given  center, radius, and material
triangle::triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, float tx0, float tx1, float tx2, float ty0, float ty1, float ty2, int m, bool bmp, scene* s) : rtObject(s)
{
	point0 = p0;
	point1 = p1;
	point2 = p2;

	texX0 = tx0;
	texX1 = tx1;
	texX2 = tx2;
	texY0 = ty0;
	texY1 = ty1;
	texY2 = ty2;
	matIndex = m;
	bump = bmp;
	myScene = s;

	float x_array[3] = {p0.x, p1.x, p2.x};
	float y_array[3] = {p0.y, p1.y, p2.y};
	float z_array[3] = {p0.z, p1.z, p2.z};

	bbox.set_boundingBox(*std::min_element(x_array, x_array + 3), *std::max_element(x_array, x_array + 3),
						*std::min_element(y_array, y_array + 3), *std::max_element(y_array, y_array + 3),
						*std::min_element(z_array, z_array + 3), *std::max_element(z_array, z_array + 3));
}

float triangle::testIntersection(glm::vec3 eye, glm::vec3 dir)
{
	//see the book/slides for a description of how to use Cramer's rule to solve
	//for the intersection(s) of a line and a plane, implement it here and
	//return the minimum distance (if barycentric coordinates indicate it hit
	//the triangle) otherwise 9999999

	dir = glm::normalize(dir);
	
	// Based on textbook P78 equation 4.2, point0:a, point1:b, point2:c, eye:e, dir:d
	float a = point0.x - point1.x;
	float b = point0.y - point1.y;
	float c = point0.z - point1.z;

	float d = point0.x - point2.x;
	float e = point0.y - point2.y;
	float f = point0.z - point2.z;

	float g = dir.x;
	float h = dir.y;
	float i = dir.z;

	float j = point0.x - eye.x;
	float k = point0.y - eye.y;
	float l = point0.z - eye.z;

	float ei_hf = e * i - h * f;
	float gf_di = g * f - d * i;
	float dh_eg = d * h - e * g;

	float ak_jb = a * k - j * b;
	float jc_al = j * c - a * l;
	float bl_kc = b * l - k * c;

	float M = a * ei_hf + b * gf_di + c * dh_eg;

	// result
	float t = - (f * ak_jb + e * jc_al + d * bl_kc) / M;
	if (t < 0.001f || t > 9999999.0f)
		return 9999999;

	float beta = (j * ei_hf + k * gf_di + l * dh_eg) / M;
	if (beta <= 0.0f || beta >= 1.0f)
		return 9999999;

	float gamma = (i * ak_jb + h * jc_al + g * bl_kc) / M;
	if (gamma <= 0.0f || gamma >= (1.0f - beta))
		return 9999999;

	return t;
}

glm::vec3 triangle::getNormal(glm::vec3 eye, glm::vec3 dir)
{
	//construct the barycentric coordinates for the plane
	glm::vec3 bary1 = point1 - point0;
	glm::vec3 bary2 = point2 - point0;

	//cross them to get the normal to the plane
	//note that the normal points in the direction given by right-hand rule
	//(this can be important for refraction to know whether you are entering or leaving a material)
	glm::vec3 normal = glm::normalize(glm::cross(bary1, bary2));

	// add vector turbulance to the normal
	if (bump)
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<int> dist(-10, 10); // distribution in range [-10, 10]
		glm::vec3 turbulance = glm::vec3((float)dist(rng), (float)dist(rng), (float)dist(rng));

		normal += 0.1f * glm::normalize(turbulance);
		normal = glm::normalize(normal);
	}

	return normal;
}

glm::vec2 triangle::getTextureCoords(glm::vec3 eye, glm::vec3 dir)
{
	//find alpha and beta (parametric distance along barycentric coordinates)
	//use these in combination with the known texture surface location of the vertices
	//to find the texture surface location of the point you are seeing

	// first, find gamma and beta, similar to what testIntersection do
	// Based on textbook P78 equation 4.2, point0:a, point1:b, point2:c, eye:e, dir:d
	// any point can be expressed by a + beta(b - a) + gamma(c - a)
	float a = point0.x - point1.x;
	float b = point0.y - point1.y;
	float c = point0.z - point1.z;

	float d = point0.x - point2.x;
	float e = point0.y - point2.y;
	float f = point0.z - point2.z;

	float g = dir.x;
	float h = dir.y;
	float i = dir.z;

	float j = point0.x - eye.x;
	float k = point0.y - eye.y;
	float l = point0.z - eye.z;

	float ei_hf = e * i - h * f;
	float gf_di = g * f - d * i;
	float dh_eg = d * h - e * g;

	float ak_jb = a * k - j * b;
	float jc_al = j * c - a * l;
	float bl_kc = b * l - k * c;

	float M = a * ei_hf + b * gf_di + c * dh_eg;

	float beta = (j * ei_hf + k * gf_di + l * dh_eg) / M;
	if (beta <= 0.0f || beta >= 1.0f) {
		//assert(false);
		//printf("beta: %.4f\n", beta);
		return glm::vec2(0.0f);
	}

	float gamma = (i * ak_jb + h * jc_al + g * bl_kc) / M;
	if (gamma <= 0.0f || gamma >= (1.0f - beta)) {
		//assert(false);
		//printf("gamma: %.4f\n", gamma);
		return glm::vec2(0.0f);
	}

	float alpha = 1.0f - beta - gamma;

	glm::vec2 coords;
	coords.x = alpha * texX0 + beta * texX1 + gamma * texX2;
	coords.y = alpha * texY0 + beta * texY1 + gamma * texY2;

	return coords;
}

glm::vec3 triangle::getCenter()
{
	return (point0 + point1 + point2) / 3.0f;
}