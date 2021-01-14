#include "bvh_node.h"

// AXIS: 0 means x-axis, 1 means y-axis, 2 means z-axis
bvh_node::bvh_node(std::vector<rtObject*> myObjects, int AXIS)
{
	int N = myObjects.size();

	if (N == 1)
	{
		left_node = new bvh_node(&(myObjects[0]->bbox));
		right_node = NULL;
		bbox = &(myObjects[0]->bbox);
	}
	else if (N == 2)
	{
		left_node = new bvh_node(&(myObjects[0]->bbox));
		right_node = new bvh_node(&(myObjects[1]->bbox));
		bbox = boundingBox::combine(&(myObjects[0]->bbox), &(myObjects[1]->bbox));
	}
	else
	{
		std::map<float, rtObject*> dist_ptr_map;

		// sort myObjGroup by the object center along AXIS

		// 1. pair object pointer with the distance between object center and AXIS
		for (int i = 0; i < N; i++)
		{
			glm::vec3 projection = myObjects[i]->getCenter();
			projection[AXIS] = 0.0f;
			assert(projection[1] == projection.y);

			float dist = glm::length(projection);

			// if insert failed, slightly change the key value
			int size_before = dist_ptr_map.size();
			int size_after = size_before;
			for (float delta = 0.0f; size_before == size_after; delta += 0.0001f) 
			{
				dist += delta;

				dist_ptr_map.insert({ dist, myObjects[i] });
				size_after = dist_ptr_map.size();
			}
		}

		// 2. construct a dist list, and sort all the dist
		std::vector<float> dist_list;
		for (std::map<float, rtObject*>::iterator itr = dist_ptr_map.begin(); itr != dist_ptr_map.end(); itr++)
		{
			dist_list.push_back(itr->first);
		}
		
		std::sort(dist_list.begin(), dist_list.end());

		// 3. order the object pointers based on dist
		std::vector<rtObject*> obj_list;
		for (auto dist : dist_list)
		{
			obj_list.push_back(dist_ptr_map[dist]);
		}

		// 4. partition the sorted object list and recursively
		// constructed a tree with left and right sub-list
		std::vector<rtObject*> left_obj(obj_list.begin(), obj_list.begin() + N/2);
		std::vector<rtObject*> right_obj(obj_list.begin() + N/2, obj_list.end());
		// make sure no overlaping between left and right list
		assert(left_obj[left_obj.size() - 1] != right_obj[0]);

		left_node = new bvh_node(left_obj, (AXIS + 1) % 3);
		right_node = new bvh_node(right_obj, (AXIS + 1) % 3);
		bbox = boundingBox::combine(left_node->bbox, right_node->bbox);
	}
}

bvh_node::bvh_node(boundingBox* bounding_box)
{
	bbox = bounding_box;
	left_node = NULL;
	right_node = NULL;
}

bool bvh_node::hit(glm::vec3 eye, glm::vec3 dir, float& rec)
{
	if (bbox->testIntersection(eye, dir, rec))
	{
		if (left_node == NULL && right_node == NULL)
			return true;

		float lrec, rrec; // left & right hit record

		bool left_hit = (left_node != NULL) && (left_node->hit(eye, dir, lrec));
		bool right_hit = (right_node != NULL) && (right_node->hit(eye, dir, rrec));

		if (left_hit && right_hit)
		{
			if (lrec < rrec)
				rec = lrec;
			else
				rec = rrec;
			return true;
		}
		else if (left_hit)
		{
			rec = lrec;
			return true;
		}
		else if (right_hit)
		{
			rec = rrec;
			return true;
		}
		else
			return false;
	}
	else
		return false;
}