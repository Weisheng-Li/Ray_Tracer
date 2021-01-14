#include "rtObjGroup.h"
#include "boundingBox.h"

//default constructor
rtObjGroup::rtObjGroup()
{}

//copy constructor
rtObjGroup::rtObjGroup(const rtObjGroup& src_objG)
{
	myObjects = src_objG.myObjects;
	indexOfClosest = src_objG.indexOfClosest;
}

//add an object to the vector
void rtObjGroup::addObj(rtObject *o)
{
	myObjects.push_back(o);
}

//accessor for objects in the vector
rtObject* rtObjGroup::getObj(int index)
{
	return myObjects.at(index);
}

float rtObjGroup::getObjectNum()
{
	return myObjects.size();
}

float rtObjGroup::testIntersections(glm::vec3 eye, glm::vec3 dir, bvh_node* bvh_root)
{
	float closest = 9999999;
	float currentDist;

	// test if the ray hit the bvh, return if not hit
	float t_hit; // not used
	if (!bvh_root->hit(eye, dir, t_hit)) return closest;

	//test intersection distance with every object in the group
	for (int iter = 0; iter<myObjects.size(); iter++)
	{
		currentDist = myObjects.at(iter)->testIntersection(eye, dir);
		//keep track of closest distance and remember index of closest object
		if (currentDist<closest)
		{
			closest = currentDist;
			indexOfClosest = iter;
		}
	}
	return closest;
}

//returns the object that was closest in the last call to testIntersections
rtObject* rtObjGroup::getClosest()
{
	return myObjects.at(indexOfClosest);
}
