/*
Author: Weisheng Li
Email: wjl5238@psu.edu
*/

#include <Project3.hpp>
#include <omp.h>
#define CLUSTER true


//  Modify this preamble based on your code.  It should contain all the functionality of your project.  
std::string preamble =
"Project 3 code: Raytracing \n"
"Sample Input:  SampleScenes/reflectionTest.ray \n"
"Also, asks whether or not you want to configure for use with clusters (does not display results while running) \n"
"If you want to run without being asked for an input\n"
"'./Project_3/Project_3  SampleScenes/reflectionTest.ray y'\n\n";
int main(int argc, char **argv)
{
	std::printf(preamble.c_str());
	std::string fn,ans_str,v_m;

	// This turns off the window and OpenGL functions so that you can run it on the clusters.    
		//  This will also make it run slightly faster since it doesn't need to display in every 10 rows. 
	bool cluster=false;
	bool video_mode = false;
	if (argc < 2)// if not specified, prompt for filename and if running on cluster
	{
		char input[999];
		std::printf("Input .ray file: ");
		std::cin >> input;
		fn = input;
		std::printf("Running on cluster or no display (y/n): ");
		std::cin >> input;
		ans_str = input;
		cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
		std::printf("Rendering for video or not (y/n): ");
		std::cin >> input;
		v_m = input;
		video_mode = (v_m.compare("y") == 0) || (v_m.compare("Y") == 0);
	}
	else //otherwise, use the name provided
	{
		/* Open jpeg (reads into memory) */
		char* filename = argv[1];
		fn = filename;

		if (argc < 3)
		{
			char input[999];
			std::printf("Running on cluster or no display (y/n): ");
			std::cin >> input;
			ans_str = input;
			cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
		}
		else 
		{
			char* ans = argv[2];
			ans_str = ans;
			cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
		}
	}


	if (cluster)
		std::printf("Configured for Clusters\n");
	else
		std::printf("Not configured for Clusters\n");
	fn = "../Project_3/Media/" + fn;
	std::printf("Opening File %s\n", fn.c_str());

	myScene = new scene(fn.c_str());

	GLFWwindow* window  = NULL;

	Shader screenShader("", "");
	if (!cluster)
	{
	    // glfw: initialize and configure
	    // ------------------------------

	    glfwInit();
	    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	    glfwWindowHint(GLFW_SAMPLES, 4);

	#ifdef __APPLE__
	    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif
	    // glfw window creation
	    // --------------------
	    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project 3", NULL, NULL);
	    if (window == NULL)
	    {
		    std::cout << "Failed to create GLFW window" << std::endl;
		    glfwTerminate();
		    return -1;
	    }
	    glfwMakeContextCurrent(window);
	    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	    // glad: load all OpenGL function pointers
	    // ---------------------------------------
	    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	    {
		    std::cout << "Failed to initialize GLAD" << std::endl;
		    return -1;
	    }


	    // configure global opengl state
	    // -----------------------------
	    glEnable(GL_MULTISAMPLE); // Enabled by default on some drivers, but not all so always enable to make sure

	    // build and compile shaders
	    // -------------------------

	    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
							     // positions   // texCoords
		    -1.0f,  1.0f,  0.0f, 1.0f,
		    -1.0f, -1.0f,  0.0f, 0.0f,
		    1.0f, -1.0f,  1.0f, 0.0f,

		    -1.0f,  1.0f,  0.0f, 1.0f,
		    1.0f, -1.0f,  1.0f, 0.0f,
		    1.0f,  1.0f,  1.0f, 1.0f
	    };

	    glGenVertexArrays(1, &quadVAO);
	    glGenBuffers(1, &quadVBO);
	    glBindVertexArray(quadVAO);
	    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	    glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	    glEnableVertexAttribArray(1);
	    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	    // shader configuration
	    // --------------------

	    screenShader = Shader("../Project_3/Shaders/screenShader.vert", "../Project_3/Shaders/screenShader.frag");
	    screenShader.use();
	    screenShader.setInt("screenTexture", 0);
	    glActiveTexture(GL_TEXTURE0);
	    // Create initial image texture
	    // Starts from the top left (0,0) to the bottom right
	    // X is the column number and Y is the row number
	    glGenTextures(1, &textureColorbuffer);
	}
	for (int y = 0; y < SCR_HEIGHT; y++)
		for (int x = 0; x < SCR_WIDTH; x++)
			image.push_back(glm::u8vec3(0));

	if (!cluster)
		update_image_texture();

	bool raytracing = true;

	//get camera parameters from scene
	glm::vec3 eye = myScene->getEye();
	glm::vec3 lookAt = myScene->getLookAt();
	glm::vec3 up = myScene->getUp();
	float fovy = myScene->getFovy();

	fovy = fovy * 3.1416 / 180; // so now fovy is in radians

	//get the direction we are looking
	glm::vec3 dir = glm::normalize(lookAt - eye);
	//cross up and dir to get a vector to our left
	glm::vec3 left = glm::normalize(glm::cross(up, dir)); 

	// Camera Movement
	glm::vec3 cam_movement(0, 2, 0);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	const char* glsl_version = "#version 330";
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// GUI state variables
	int item_current = 0;
	int item_prev = 0;
	const char* items[] = { "reflection&refraction.ray",
							"testEverything.ray",
							"reflectiveSpheres&Tris.ray",
							"myScene01.ray",
							"myScene02.ray" };

	// render loop
	// -----------
    bool running = true;
    if (!cluster) 
        running=!glfwWindowShouldClose(window);
	while (running)
	{
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			// Most ImGui functions would normally just crash if the context is missing.
			IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");

			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(450, 180), ImGuiCond_FirstUseEver);

			// Main body of the Demo window starts here.
			ImGui::Begin("Control Panel");

			ImGui::Text("Current Scene: ");
			ImGui::SameLine();
			ImGui::Text(fn.c_str());

			ImGui::Text("Render Time: %.4f s", Timer::duration_s);

			ImGui::NewLine();
			ImGui::Text("Scene Selection");

			// Using the _simplified_ one-liner Combo() api here
			// See "Combo" section for examples of how to use the more complete BeginCombo()/EndCombo() api.
			if (ImGui::Combo("", &item_current, items, IM_ARRAYSIZE(items)))
			{
				// Update the Scene
				delete myScene;

				std::string filename(items[item_current]);
				filename = "../Project_3/Media/SampleScenes/" + filename;
				myScene = new scene(filename.c_str());

				//update camera parameters from scene
				eye = myScene->getEye();
				lookAt = myScene->getLookAt();
				up = myScene->getUp();
				fovy = myScene->getFovy();

				fovy = fovy * 3.1416 / 180; // convert to radians

				//get the direction we are looking
				dir = glm::normalize(lookAt - eye);
				//cross up and dir to get a vector to our left
				left = glm::normalize(glm::cross(up, dir));

				raytracing = true;
			}

			ImGui::SameLine(); 

			const std::string helper_message =
				"The list contains several sample scenes. The application will render "
				"the scene you choose, you can also write your own scene and put it on Media\\Scene folder\n";

			ImGui::TextDisabled("More Info");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(helper_message.c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			ImGui::End();
		}

		// Start the ray tracing
		if ((video_mode && total_frames--) || (!video_mode && raytracing))
		{
			Timer timer;

			// Update the scene on each frame
			if (video_mode) {
				myScene->setEye(myScene->getEye() + cam_movement / (float)FPS);
				eye = myScene->getEye();
			}

			// Go through 10 rows in parallel
			#pragma omp parallel for schedule(dynamic) num_threads(12)
			for (int y = 0; y < SCR_HEIGHT; y++)
			{
				// instantiate a local copy of myScene
				scene scene_cp = *myScene;

				// This runs the following loop in parallel -- You can comment the next line out for debugging purposes 
				for (int x = 0; x < SCR_WIDTH; x++)
				{
					glm::vec3 currentColor;
					
					// Obtain two basis vectors for view rectangle
					// direction
					glm::vec3 screenRight = glm::normalize(glm::cross(dir, up));
					glm::vec3 screenDown = glm::normalize(glm::cross(dir, screenRight));

					// magnitude
					screenDown *= 2 * tan(fovy / 2); 
					screenRight *= glm::length(screenDown) * ((float)SCR_WIDTH / (float)SCR_HEIGHT);

					screenDown /= 2.0f;
					screenRight /= 2.0f;

					float right_coeff = ((float)x - 0.5f * SCR_WIDTH) / (float)(0.5f * SCR_WIDTH);
					float down_coeff = ((float)y - 0.5f * SCR_HEIGHT) / (float)(0.5f * SCR_HEIGHT);

					glm::vec3 currentDir = dir + right_coeff * screenRight + down_coeff * screenDown;

					// currentColor = myScene->rayTrace(eye,currentDir,0);
					currentColor = scene_cp.rayTrace(eye, currentDir, 0);

					//Put the color into our image buffer.  
					//  This first clamps the "currentColor" variable within a range of 0,1 which means min(max(x,0),1) 
					//	so if all white or black, your colors are outside this range.
					//  Then, this takes the float colors between 0 and 1 and makes them between [0,255], then coverts to uint8 through rounding. 

					image[x + y * SCR_WIDTH] = glm::u8vec3(glm::clamp(currentColor, 0.0f, 1.0f) * 255.0f);
				}

				if (!cluster) {
					processInput(window);
					if (glfwWindowShouldClose(window))
						break;

					//  Update texture for for drawing
					update_image_texture();

					// Draw the textrue
					glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
					glClear(GL_COLOR_BUFFER_BIT);
					// Bind our texture we are creating
					glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
					glBindVertexArray(quadVAO);
					glDrawArrays(GL_TRIANGLES, 0, 6);

					glfwSwapBuffers(window);
					glfwPollEvents();
				}
			}

			// Write the final output image
			if (!video_mode)
				stbi_write_png("../Project_3/final_out.png", SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3)*SCR_WIDTH);
			else {
				int current_frame_num = VIDEO_LEN * FPS - total_frames;
				std::string current_frame = std::to_string(current_frame_num);
				std::string filename = std::string("../Project_3/img_output/frame_") + std::string(3 - current_frame.length(), '0')
					+ current_frame + std::string(".png");

				std::cout << filename << std::endl;
				stbi_write_png(&filename[0], SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3) * SCR_WIDTH);
			}

			timer.Stop();
		}

		// Done rendering, just draw the image now
		raytracing = false;


        if (!cluster) 
        {
		    // input
		    // -----
		    processInput(window);

		    // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
		    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
		    glClear(GL_COLOR_BUFFER_BIT);
		    // Bind our texture we are creating
		    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		    glBindVertexArray(quadVAO);
		    glDrawArrays(GL_TRIANGLES, 0, 6);

			// Render GUI
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		    glfwSwapBuffers(window);
		    glfwPollEvents();

            running=!glfwWindowShouldClose(window);
        }
        else
            running=false;
	}


	// de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
    if (!cluster) 
    {
        glDeleteBuffers(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);

		// ImGui Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

        glfwTerminate();
    }
	return 0;
}

//   process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
//   The movement of the boxes is still here.  Feel free to use it or take it out
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	// Escape Key quits
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);


	// P Key saves the image
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		stbi_write_png("../Project_3/out.png", SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3)*SCR_WIDTH);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);

}

// Update the current texture in image and sent the data to the textureColorbuffer
void update_image_texture()
{
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}
