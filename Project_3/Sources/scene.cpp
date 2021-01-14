#include "scene.h"

scene::scene(const char* filename)
{
	parse(filename);
	std::cout << std::endl << "background: " << glm::to_string(bgColor);
	std::cout << std::endl << "ambient: " << glm::to_string(ambLight);
	std::cout << std::endl << "eye: " << glm::to_string(eye);
	std::cout << std::endl << "lookAt: " << glm::to_string(lookAt);
	std::cout << std::endl << "up: " << glm::to_string(up);
	std::cout << std::endl << "fovy: " << fovy;
	std::cout << std::endl;

	// construct bounding volume hierachy
	bvh_root = new bvh_node(myObjGroup->myObjects, 2);
}

scene::~scene() 
{
	delete myObjGroup;
}

// copy constructor
scene::scene(const scene &src_scene)
{
	file = src_scene.file;
	parse_char = src_scene.parse_char; //current character read from file
	curline = src_scene.curline;       //current line in the file

	//the group of objects that are part of the scene
	//you may want to utilize multiple groups for view volume segmentation
	myObjGroup = new rtObjGroup(*(src_scene.myObjGroup));
	myMaterials = src_scene.myMaterials;
	myLights = src_scene.myLights;

	bgColor = src_scene.bgColor;   //background color
	ambLight = src_scene.ambLight; //ambient light
	eye = src_scene.eye;		   //camera location
	lookAt = src_scene.lookAt;	   //camera looking at (x, y, z)
	up = src_scene.up;			   //camera up vector
	fovy = src_scene.fovy;		   //camera vertical angle of view

	bvh_root = src_scene.bvh_root;
}

glm::vec3 scene::rayTrace(glm::vec3 eye, glm::vec3 dir, int recurseDepth)
{
	glm::vec3 original_dir = dir;
	dir = glm::normalize(dir);

	//start with black, add color as we go
	glm::vec3 answer(0.0f);

	//test for intersection against all our objects
	float dist;
	if (recurseDepth == 0)
	{
		// when the light is shoot out from eye, the intersection is only
		// valid when it happens in front of view retangle.
		dist = myObjGroup->testIntersections(eye + original_dir, dir, bvh_root);
		//if we saw nothing, return the background color of our scene
		if (dist == 9999999)
			return bgColor;
		else
			dist += glm::length(original_dir);
	}
	else
	{
		dist = myObjGroup->testIntersections(eye, dir, bvh_root);
		//if we saw nothing, return the background color of our scene
		if (dist == 9999999)
			return bgColor;
	}

	glm::vec3 intersec_p = eye + dist * glm::normalize(dir);

	//get the material index and normal vector(at the point we saw) of the object we saw
	int matIndex = myObjGroup->getClosest()->getMatIndex();
	material * texture = &myMaterials.at(matIndex);
	glm::vec3 normal = myObjGroup->getClosest()->getNormal(eye, dir);

	//determine texture color
	glm::vec3 textureColor;

	if (!texture->image)
		//this is multiplicative, rather than additive
		//so if there is no texture, just use ones
		textureColor = glm::vec3(1.0f);
	else
	{
		//if there is a texture image, ask the object for the image coordinates (between 0 and 1)
		glm::vec2 coords = myObjGroup->getClosest()->getTextureCoords(eye, dir);

		//get the color from that image location
		
		int x = (int)(texture->width*coords.x);
		int y = (int)(texture->height*coords.y);
		textureColor = texture->data[x + y*texture->width];
	}

	//add diffuse color times ambient light to our answer
	answer += ambLight * texture->diffuseCol;

	//iterate through all lights

	//if the light can see the surface point,
	//add its diffuse color to a total diffuse for the point (using our illumination model)
	//use testIntersection to help decide this

	for (auto& light : myLights) 
	{
		float prev_closest = myObjGroup->indexOfClosest;

		std::vector<glm::vec3> shadow_rays;
		std::vector<float> intersec_dists;

		// random light source position generated for area light
		std::vector<glm::vec3> rand_positions;

		// if the light souce is an area instead of a point, choose 
		// a few random points inside the light source area
		if (light.positions.size() >= 3)
		{
			glm::vec3 origin, basis1, basis2;
			// set up random generator
			std::random_device dev;
			std::mt19937 rng(dev());
			std::uniform_int_distribution<int> dist(0, 10); // distribution in range [0, 10]

			origin = light.positions[0];
			basis1 = 1.0f / 10.0f * (light.positions[1] - origin);
			basis2 = 1.0f / 10.0f * (light.positions[2] - origin);
			assert(abs(glm::dot(basis1, basis2) - 0.0f) < EPSILON); // 2 basis must be orthogonal

			for (int i = 0; i < light.positions.size(); i++)
				rand_positions.push_back(origin + basis1 * (float)dist(rng) + basis2 * (float)dist(rng));
		}

		// create shadow ray and test intersection for each points on the light source
		for (int i = 0; i < light.positions.size(); i++)
		{
			if (rand_positions.size() > 0)
			{
				shadow_rays.push_back(intersec_p - rand_positions[i]);
				intersec_dists.push_back(myObjGroup->testIntersections(rand_positions[i], shadow_rays[i], bvh_root));
			}
			else
			{
				shadow_rays.push_back(intersec_p - light.positions[i]);
				intersec_dists.push_back(myObjGroup->testIntersections(light.positions[i], shadow_rays[i], bvh_root));
			}
		}

		// change indexOfClosest back after being changed by shadow ray
		// intersection testing
		myObjGroup->indexOfClosest = prev_closest;

		// if there is no other object between ligth source and intersect point
		for (int i = 0; i < light.positions.size(); i++) {
			if (abs(intersec_dists[i] - glm::length(shadow_rays[i])) < 3 * EPSILON)
			{
				// difuse: light * diffuse_color * (n dot shadow_ray)
				answer += (1.0f / (float)light.positions.size()) * light.color * texture->diffuseCol
					* glm::dot(myObjGroup->getClosest()->getNormal(eye, dir), glm::normalize(-shadow_rays[i]));

				// specular: light * specular_color * (n dot h)^shininess, h calculated by normalized(e + l)
				answer += (1.0f / (float)light.positions.size()) * light.color * texture->specularCol
					* pow(glm::dot(myObjGroup->getClosest()->getNormal(eye, dir), glm::normalize(glm::normalize(-dir) +
						glm::normalize(-shadow_rays[i]))), texture->shininess);
			}
		}
	}

	//put a limit on the depth of recursion
	//if (recurseDepth<3)
	//{
	//reflect our view across the normal
	//recusively raytrace from the surface point along the reflected view
	//add the color seen times the reflective color
	if (recurseDepth < 3)
	{
		// Calculate the mirror reflection direction
		glm::vec3 R1 = glm::normalize(-dir);

		// glm::vec3 reflected_ray = glm::normalize(2.0f * glm::dot(R1, normal) * normal - R1);
		// answer += texture->reflectiveCol * rayTrace(intersec_p, reflected_ray, recurseDepth + 1);

		glm::vec3 reflection_vector = glm::normalize(dir - 2 * (glm::dot(dir, normal)) * normal);
		answer += myMaterials[matIndex].reflectiveCol * rayTrace(intersec_p + reflection_vector * EPSILON, reflection_vector, recurseDepth + 1);

		//if going into material (dot prod of dir and normal is negative), bend toward normal
		//find entry angle using inverse cos of dot product of dir and -normal
		//multiply entry angle by index of refraction to get exit angle
		//else, bend away
		//find entry angle using inverse cos of dot product of dir and normal
		//divide entry angle by index of refraction to get exit angle
		//recursively raytrace from the other side of the object along the new direction
		//add the color seen times the transparent color

		// This solution comes from textbook P305, 3rd edition
		/////////////////////////////////////////////////////////////////////////////////////////////		
		glm::vec3 d;
		float n, nt;
		// going into the material
		if (glm::dot(R1, normal) > 0.0f)
		{
			normal = normal;
			d = -R1;
			n = 1.0f;					   // air refraction index
			nt = 1.0f / texture->refractionIndex; // material refraction index
		}
		// going out of the material
		else
		{
			normal = -normal;
			d = -R1;
			n = 1.0f / texture->refractionIndex;
			nt = 1.0f;
		}

		float under_sqrt = 1.0f - (n*n * (1.0f - pow(glm::dot(d, normal), 2))) / (nt * nt);

		// refraction only exist when the part under square root is non-negative
		if (under_sqrt >= 0.0f || abs(under_sqrt - 0.0f) < EPSILON)
		{
			if (abs(under_sqrt - 0.0f) < EPSILON)
				under_sqrt = 0.0f;
			glm::vec3 refracted_ray = (n / nt) * (d - normal * glm::dot(d, normal)) - normal * sqrt(under_sqrt);

			answer += texture->transparentCol * rayTrace(intersec_p, refracted_ray, recurseDepth + 1);
		}

	// An Alternative Solution, but slower.

	//	float ior = texture->refractionIndex;
	//	glm::vec3 ref_light = glm::normalize(dir);
	//	glm::vec3 ref_norm = glm::normalize(normal);
	//	
	//	float entry_angle, exit_angle;	
	//	if (glm::dot(dir, normal) < 0.0f)
	//	{
	//		entry_angle = acos(glm::dot(ref_norm, -ref_light));
	//	}
	//	else
	//	{
	//		entry_angle = acos(glm::dot(ref_norm, ref_light));
	//		ior = 1.0f / ior;
	//		ref_norm = -ref_norm;
	//	}

	//	exit_angle = entry_angle * ior;

	//	glm::vec3 refraction_dir = glm::normalize(ior * (ref_light + ref_norm * cos(entry_angle)) - ref_norm * cos(exit_angle));

	//	answer += texture->transparentCol * rayTrace(intersec_p + refraction_dir * 0.0001f, refraction_dir, recurseDepth + 1);
	}

	//multiply whatever color we have found by the texture color
	answer *= textureColor;
	return answer;
}

glm::vec3 scene::getEye()
{
	return eye;
}

glm::vec3 scene::getLookAt()
{
	return lookAt;
}

glm::vec3 scene::getUp()
{
	return up;
}

float scene::getFovy()
{
	return fovy;
}

void scene::parse(const char *filename)
{
	//some default values in case parsing fails
	myObjGroup = NULL;
	bgColor = glm::vec3(0.5, 0.5, 0.5);
	ambLight = glm::vec3(0.5, 0.5, 0.5);
	eye = glm::vec3(0, 0, 0);
	lookAt = glm::vec3(0, 0, -1);
	up = glm::vec3(0, 1, 0);
	fovy = 45;
	file = NULL;
	curline = 1;

	//the file-extension needs to be "ray"
	assert(filename != NULL);
	const char *ext = &filename[strlen(filename) - 4];
	// if no ray extention
	if (strcmp(ext, ".ray"))
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		assert(!strcmp(ext, ".ray"));
	}
	file = fopen(filename, "r");
	// Fails if no file exists
	if (file == NULL)
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		//assert(file != NULL);
	}
	char token[MAX_PARSER_TOKEN_LENGTH];

	//prime the parser pipe
	parse_char = fgetc(file);

	while (getToken(token))
	{
		if (!strcmp(token, "Background"))
			parseBackground();
		else if (!strcmp(token, "Camera"))
			parseCamera();
		else if (!strcmp(token, "Materials"))
			parseMaterials();
		else if (!strcmp(token, "Group"))
			myObjGroup = parseGroup();
		else if (!strcmp(token, "Lights"))
			parseLights();
		else
		{
			std::cout << "Unknown token in parseFile: '" << token <<
				"' at input line " << curline << "\n";
			exit(-1);
		}
	}
}

/* Parse the "Camera" token */
void scene::parseCamera()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	//get the eye, center, and up vectors (similar to gluLookAt)
	getToken(token); assert(!strcmp(token, "eye"));
	eye = readVec3f();

	getToken(token); assert(!strcmp(token, "lookAt"));
	lookAt = readVec3f();

	getToken(token); assert(!strcmp(token, "up"));
	up = readVec3f();


	getToken(token); assert(!strcmp(token, "fovy"));
	fovy = readFloat();

	getToken(token); assert(!strcmp(token, "}"));
}

/* Parses the "Background" token */
void scene::parseBackground()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	while (1)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			break;
		}
		else if (!strcmp(token, "color"))
		{
			bgColor = readVec3f();
		}
		else if (!strcmp(token, "ambientLight"))
		{
			ambLight = readVec3f();
		}
		else
		{
			std::cout << "Unknown token in parseBackground: " << token << "\n";
			assert(0);
		}
	}
}

/* Parses the "Group" token */
rtObjGroup* scene::parseGroup()
{
	char token[MAX_PARSER_TOKEN_LENGTH];


	getToken(token);
	assert(!strcmp(token, "{"));

	/**********************************************/
	/* Instantiate the group object               */
	/**********************************************/
	rtObjGroup *answer = new rtObjGroup();

	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else
		{
			if (!strcmp(token, "Sphere"))
			{
				sphere *sceneElem = parseSphere();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else if (!strcmp(token, "Triangle"))
			{
				triangle *sceneElem = parseTriangle();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else
			{
				std::cout << "Unknown token in Group: '" << token << "' at line "
					<< curline << "\n";
				exit(0);
			}
		}
	}

	/* Return the group */
	return answer;
}

/* Parse the "Sphere" token */
sphere* scene::parseSphere()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int sphere_material_index = readInt();

	getToken(token); assert(!strcmp(token, "center"));
	glm::vec3 center = readVec3f();
	getToken(token); assert(!strcmp(token, "radius"));
	float radius = readFloat();
	getToken(token); assert(!strcmp(token, "}"));

	std::printf("Sphere:\n\tCenter: %s\n", glm::to_string(center).c_str());
	std::printf("\tRadius: %f\n", radius);
	std::printf("\tSphere Material Index: %d\n\n", sphere_material_index);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new sphere(center, radius, sphere_material_index, this);
}

/* Parse out the "Triangle" token */
triangle* scene::parseTriangle()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Parse out vertex information */
	getToken(token); assert(!strcmp(token, "vertex0"));
	glm::vec3 v0 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex1"));
	glm::vec3 v1 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex2"));
	glm::vec3 v2 = readVec3f();
	getToken(token); assert(!strcmp(token, "tex_xy_0"));
	float x0 = 0;
	float y0 = 0;
	x0 = readFloat();
	y0 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_1"));
	float x1 = 0;
	float y1 = 0;
	x1 = readFloat();
	y1 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_2"));
	float x2 = 0;
	float y2 = 0;
	x2 = readFloat();
	y2 = readFloat();
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int mat = readInt();

	// whether bump map is enabled, false if not specified
	getToken(token); 
	bool bump = false;
	if (!strcmp(token, "bump")) {
		bump = readFloat();

		getToken(token);
		assert(!strcmp(token, "}"));
	}
	else 
		assert(!strcmp(token, "}"));




	std::printf("Triangle:\n");
	std::printf("\tVertex0: %s tex xy 0 %s\n", glm::to_string(v0).c_str(), glm::to_string(glm::vec2(x0, y0)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v1).c_str(), glm::to_string(glm::vec2(x1, y1)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v2).c_str(), glm::to_string(glm::vec2(x2, y2)).c_str());
	std::printf("\tTriangle Material Index: %d\n\n", mat);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new triangle(v0, v1, v2, x0, x1, x2, y0, y1, y2, mat, bump, this);
}

/* Parse the "Materials" token */
void scene::parseMaterials()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Material */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Material"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 diffuseColor(1, 1, 1);
			glm::vec3 specularColor(0, 0, 0);
			float shininess = 1;
			glm::vec3 transparentColor(0, 0, 0);
			glm::vec3 reflectiveColor(0, 0, 0);
			float indexOfRefraction = 1;

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "textureFilename"))
				{
					getToken(token);
					strcpy(texname, token);
				}
				else if (!strcmp(token, "diffuseColor"))
					diffuseColor = readVec3f();
				else if (!strcmp(token, "specularColor"))
					specularColor = readVec3f();
				else if (!strcmp(token, "shininess"))
					shininess = readFloat();
				else if (!strcmp(token, "transparentColor"))
					transparentColor = readVec3f();
				else if (!strcmp(token, "reflectiveColor"))
					reflectiveColor = readVec3f();
				else if (!strcmp(token, "indexOfRefraction"))
					indexOfRefraction = readFloat();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			material temp;

			std::printf("Material:\n", texname);
			temp.diffuseCol = diffuseColor;
			std::printf("\tDiffuse Color: %s\n", glm::to_string(diffuseColor).c_str());
			temp.specularCol = specularColor;
			std::printf("\tSpecular Color: %s\n", glm::to_string(specularColor).c_str());
			temp.shininess = shininess;
			std::printf("\tShininess: %f\n", shininess);
			temp.transparentCol = transparentColor;
			std::printf("\tTransparent Color: %s\n", glm::to_string(transparentColor).c_str());
			temp.reflectiveCol = reflectiveColor;
			std::printf("\tReflective Color: %s\n", glm::to_string(reflectiveColor).c_str());
			temp.refractionIndex = indexOfRefraction;
			std::printf("\tIndex Of Refraction: %f\n", indexOfRefraction);
			std::printf("\tFileName: %s\n\n", texname);
			if (strcmp(texname, "NULL"))
			{
				temp.image = true;
				unsigned char *raw_data = stbi_load(texname, &temp.width, &temp.height, &temp.nrComponents, 0);
				if (raw_data)
				{
					// Make vector for each image
					for (int y = 0; y < temp.height; y++)
						for (int x = 0; x < temp.width; x++)
							if (temp.nrComponents == 1)
								temp.data.push_back(glm::vec3((float)raw_data[x + y*temp.height]));
							else
								temp.data.push_back(glm::vec3((float)raw_data[temp.nrComponents * (x + y*temp.width)],
									(float)raw_data[temp.nrComponents * (x + y*temp.width) + 1],
									(float)raw_data[temp.nrComponents * (x + y*temp.width) + 2]) / 255.0f);
					stbi_image_free(raw_data);
				}
				else
				{
					std::cout << "Texture failed to load at path: " << texname << std::endl;
					stbi_image_free(raw_data);
				}
			}
			else
				temp.image = false;

			myMaterials.push_back(temp);

		}
	}
}

void scene::parseLights()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Light */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Light"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 position1(0, 0, 0);
			glm::vec3 position2(0, 0, 0);
			glm::vec3 position3(0, 0, 0);
			glm::vec3 position4(0, 0, 0);
			glm::vec3 color(1, 1, 1);

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "position"))
					position1 = readVec3f();
				else if (!strcmp(token, "position2"))
					position2 = readVec3f();
				else if (!strcmp(token, "position3"))
					position3 = readVec3f();
				else if (!strcmp(token, "position4"))
					position4 = readVec3f();
				else if (!strcmp(token, "color"))
					color = readVec3f();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			/**********************************************/
			/* The call to your own constructor goes here */
			/**********************************************/
			light temp;
			if (position1 != glm::vec3(0.0f))
				temp.positions.push_back(position1);
			if (position2 != glm::vec3(0.0f))
				temp.positions.push_back(position2);
			if (position3 != glm::vec3(0.0f))
				temp.positions.push_back(position3);
			if (position4 != glm::vec3(0.0f))
				temp.positions.push_back(position4);

			std::printf("Light:\n\tPostion: %s %s %s %s\n", glm::to_string(position1).c_str(), glm::to_string(position2).c_str(),
				glm::to_string(position3).c_str(), glm::to_string(position4).c_str());
			temp.color = color;
			std::printf("\Color: %s\n\n", glm::to_string(color).c_str());
			myLights.push_back(temp);
		}
	}
}

/* consume whitespace */
void scene::eatWhitespace(void)
{
	bool working = true;

	do {
		while (isspace(parse_char))
		{
			if (parse_char == '\n')
			{
				curline++;
			}
			parse_char = fgetc(file);
		}

		if ('#' == parse_char)
		{
			/* this is a comment... eat until end of line */
			while (parse_char != '\n')
			{
				parse_char = fgetc(file);
			}

			curline++;
		}
		else
		{
			working = false;
		}

	} while (working);
}

/* Parse out a single token */
int scene::getToken(char token[MAX_PARSER_TOKEN_LENGTH])
{
	int idx = 0;

	assert(file != NULL);
	eatWhitespace();

	if (parse_char == EOF)
	{
		token[0] = '\0';
		return 0;
	}
	while ((!(isspace(parse_char))) && (parse_char != EOF))
	{
		token[idx] = parse_char;
		idx++;
		parse_char = fgetc(file);
	}

	token[idx] = '\0';
	return 1;
}

/* Reads in a 3-vector */
glm::vec3 scene::readVec3f()
{	
	float a = readFloat();
	float b = readFloat();
	float c = readFloat();
	return glm::vec3(a, b, c);
}

/* Reads in a single float */
float scene::readFloat()
{
	float answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 float (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%f", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 float\n";
		assert(0);

	}
	return answer;
}

/* Reads in a single int */
int scene::readInt()
{
	int answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 int (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%d", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 int\n";
		assert(0);

	}
	return answer;
}

void scene::setEye(glm::vec3 new_eye)
{
	eye = new_eye;
}

void scene::setLookAt(glm::vec3 new_LookAt)
{
	lookAt = new_LookAt;
}

void scene::setUp(glm::vec3 new_Up)
{
	up = new_Up;
}
