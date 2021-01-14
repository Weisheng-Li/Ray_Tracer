#include "boundingBox.h"

// Constructor
boundingBox::boundingBox(float x0, float x1, float y0, float y1, float z0, float z1)
{
	x_min = x0;
	x_max = x1;
	y_min = y0;
	y_max = y1;
	z_min = z0;
	z_max = z1;

	printf("bounding box created: %.3f %.3f %.3f %.3f %.3f %.3f\n", x0, x1, y0, y1, z0, z1);
}

// default constructor
boundingBox::boundingBox()
{
	x_min = 0;
	x_max = 0;
	y_min = 0;
	y_max = 0;
	z_min = 0;
	z_max = 0;
}

void boundingBox::set_boundingBox(float x0, float x1, float y0, float y1, float z0, float z1)
{
	x_min = x0;
	x_max = x1;
	y_min = y0;
	y_max = y1;
	z_min = z0;
	z_max = z1;

	printf("bounding box created: %.3f %.3f %.3f %.3f %.3f %.3f\n", x0, x1, y0, y1, z0, z1);
}

// detect if the ray pass through the bounding box
bool boundingBox::testIntersection(glm::vec3 eye, glm::vec3 dir, float& rec)
{
	float t_xmin, t_xmax, t_ymin, t_ymax, t_zmin, t_zmax;

	// First, Calculate the range of t
	if (dir.x >= 0)
	{
		t_xmin = (x_min - eye.x) / dir.x;
		t_xmax = (x_max - eye.x) / dir.x;
	}
	else
	{
		t_xmin = (x_max - eye.x) / dir.x;
		t_xmax = (x_min - eye.x) / dir.x;
	}

	if (dir.y >= 0)
	{
		t_ymin = (y_min - eye.y) / dir.y;
		t_ymax = (y_max - eye.y) / dir.y;
	}
	else 
	{
		t_ymin = (y_max - eye.y) / dir.y;
		t_ymax = (y_min - eye.y) / dir.y;
	}
	
	if (dir.z >= 0) 
	{
		t_zmin = (z_min - eye.z) / dir.z;
		t_zmax = (z_max - eye.z) / dir.z;
	}
	else
	{
		t_zmin = (z_max - eye.z) / dir.z;
		t_zmax = (z_min - eye.z) / dir.z;
	}

	// return false if there is no overlap between any
	// pair of t ranges
	if (t_xmin > t_ymax || t_xmax < t_ymin ||
		t_xmin > t_zmax || t_xmax < t_zmin ||
		t_ymin > t_zmax || t_ymax < t_zmin)
		return false;
	else
	{
		// closest and farthest t value
		float min_rec, max_rec;

		// find the maximum t_min, which would be the
		// closest intersection point
		if (t_xmin > t_ymin && t_xmin > t_zmin)
			min_rec = t_xmin;
		else if (t_ymin > t_xmin && t_ymin > t_zmin)
			min_rec = t_ymin;
		else
			min_rec = t_zmin;

		if (min_rec > 0.001f) {
			rec = min_rec;
			return true;
		}

		// find the minimum t_max, which would be the
		// farthest intersection point
		if (t_xmax < t_ymax && t_xmax < t_zmax)
			max_rec = t_xmax;
		else if (t_ymax < t_xmax && t_ymax < t_zmax)
			max_rec = t_ymax;
		else
			max_rec = t_zmax;

		if (max_rec > 0.001f) {
			rec = max_rec;
			return true;
		}
		else
			return false;
	}
}

// return a larger bounding box that contains 2 input bounding box
boundingBox* boundingBox::combine(boundingBox* bbox1, boundingBox* bbox2)
{
	boundingBox* new_bbox = new boundingBox();
	new_bbox->x_min = fmin(bbox1->x_min, bbox2->x_min);
	new_bbox->x_max = fmax(bbox1->x_max, bbox2->x_max);
	new_bbox->y_min = fmin(bbox1->y_min, bbox2->y_min);
	new_bbox->y_max = fmax(bbox1->y_max, bbox2->y_max);
	new_bbox->z_min = fmin(bbox1->z_min, bbox2->z_min);
	new_bbox->z_max = fmax(bbox1->z_max, bbox2->z_max);

	return new_bbox;
}