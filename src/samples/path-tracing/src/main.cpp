
#include "pch.h"
#include "utility.h"
#include "shader.h"
#include "transform.h"
#include "camera.h"
#include "object_loader.h"
#include "triangle.h"
#include "model.h"
#include "primitives.h"

int main() {
    // Initialize GLFW.
    int initializationCode = glfwInit();
    if (!initializationCode) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    // Setting up OpenGL properties.
    glfwWindowHint(GLFW_SAMPLES, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1280;
    int height = 720;

    GLFWwindow* window = glfwCreateWindow(width, height, "GLSL Path Tracing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        return 1;
    }

    // Initialize OpenGL.
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize Glad (OpenGL)." << std::endl;
        return 1;
    }

    std::cout << "Sample: GLSL Path Tracing" << std::endl;
    std::cout << "Vendor: " << (const char*)(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "Renderer: " << (const char*)(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "OpenGL Version: " << (const char*)(glGetString(GL_VERSION)) << std::endl;

    // Initialize ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    // Initialize ImGui Flags.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Attempt to load ImGui .ini configuration.
    // Find imgui_layout.ini file inside 'data' subdirectory for this sample.
    bool foundIni = false;

    for (const std::string& file : Utilities::GetFiles("src/samples/path-tracing/data")) {
        if (Utilities::GetAssetName(file) == "imgui_layout" && Utilities::GetAssetExtension(file) == "ini") {
            foundIni = true;
            break;
        }
    }

    std::string imGuiIni = "src/samples/path-tracing/data/imgui_layout.ini";

    if (foundIni) {
        ImGui::LoadIniSettingsFromDisk(imGuiIni.c_str());
    }

    io.IniFilename = nullptr; // Enable manual saving.

    // Setup Platform/Renderer backend.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize camera.
    OpenGL::Camera camera { width, height };
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    float exposure = 1.0f;
    float apertureRadius = 2.0f;
    float focusDistance = glm::max(glm::distance(camera.GetPosition(), glm::vec3(0.0f)), 10.0f);

    glViewport(0, 0, width, height);

    // Initialize skybox.
    // https://learnopengl.com/Advanced-OpenGL/Cubemaps
    std::vector<std::string> textureFaces = {
        "src/samples/path-tracing/assets/textures/skybox/pos_x.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_x.png",
        "src/samples/path-tracing/assets/textures/skybox/pos_y.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_y.png",
        "src/samples/path-tracing/assets/textures/skybox/pos_z.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_z.png"
    };

    GLuint skybox;
    glGenTextures(1, &skybox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);

    // Load skybox faces into texture.
    unsigned char* textureData;
    int textureWidth;
    int textureHeight;
    int textureChannels;

    for (unsigned i = 0; i < textureFaces.size(); ++i) {
        textureData = stbi_load(textureFaces[i].c_str(), &textureWidth, &textureHeight, &textureChannels, 0);

        if (textureData) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
            stbi_image_free(textureData);
        }
        else {
            std::cerr << "Failed to load skybox texture: " << textureFaces[i] << std::endl;
            stbi_image_free(textureData);
            return 1;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // RGBA.
    std::vector<float> blankTexture;
    blankTexture.resize(width * height * 4, 0.0f);

    GLuint frame1;
    glGenTextures(1, &frame1);
    glBindTexture(GL_TEXTURE_2D, frame1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint frame2;
    glGenTextures(1, &frame2);
    glBindTexture(GL_TEXTURE_2D, frame2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth buffer.
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Initialize custom framebuffer.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame1, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, frame2, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to initialize custom framebuffer on startup." << std::endl;
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<GLenum> drawBuffers(1);

    // Initialize global data UBO.
    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo); // Binding 0.

    // Inverse camera transforms (2x mat4), camera position (vec3, vec4 with padding).
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2 + sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Initialize scene objects.
    const int numSpheres = 256;
    std::vector<OpenGL::Sphere> spheres(numSpheres);
    int numActiveSpheres = 0;

    const int numAABBs = 256;
    std::vector<OpenGL::AABB> aabbs(numAABBs);
    int numActiveAABBs = 0;

    // 6x6 grid of spheres to showcase varying levels of both reflective materials and reflection roughness properties.
    {
        int side = 6;
        float radius = 2.0f;
        float gap = 1.0f;

        float length = (radius * 2.0f) * static_cast<float>(side) + gap * static_cast<float>(side);
        float offset = length / 2.0f;
        float delta = length / static_cast<float>(side - 1);

        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                OpenGL::Sphere& sphere = spheres[x + y * side];
                sphere.radius = radius;
                sphere.position = glm::vec3(15.0f, static_cast<float>(y) * delta - offset, static_cast<float>(x) * delta - offset);

                // Configure material properties.
                OpenGL::Material& material = sphere.material;

                material.albedo = glm::vec3(1.0f);
                material.ior = 1.0f;

                material.emissive = glm::vec3(0.0f);

                material.refractionProbability = 0.0f;
                material.absorbance = glm::vec3(0.0f);
                material.refractionRoughness = 0.1f;

                float prob = static_cast<float>(side - 1 - x) / (static_cast<float>(side - 1));
                float roug = static_cast<float>(y) / (static_cast<float>(side - 1));

                material.reflectionProbability = static_cast<float>(side - 1 - x) / (static_cast<float>(side - 1));
                material.reflectionRoughness = static_cast<float>(y) / (static_cast<float>(side - 1));

                ++numActiveSpheres;
            }

            std::cout << std::endl;
        }
    }

    // 1x6 grid of spheres to showcase refractive materials with varying levels of absorbance (Beer's Law).
    {

    }

    // 1x6 grid of spheres to showcase refractive materials with varying levels of refraction roughness.
    {

    }

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo); // Binding 1.

    // Number of active spheres (int, vec4 with padding), array of 256 spheres.
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) + numSpheres * sizeof(OpenGL::Sphere) + sizeof(glm::vec4) + numAABBs * sizeof(OpenGL::AABB), nullptr, GL_STATIC_DRAW);

    std::size_t offset = 0;

    // Set sphere object data.
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(int), &numActiveSpheres);
    offset += sizeof(glm::vec4);

    for (int i = 0; i < numActiveSpheres; ++i) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(OpenGL::Sphere), &spheres[i]);
        offset += sizeof(OpenGL::Sphere);
    }
    offset += (numSpheres - numActiveSpheres) * sizeof(OpenGL::Sphere);

    // Set AABB object data.
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(int), &numActiveAABBs);
    offset += sizeof(glm::vec4);

    for (int i = 0; i < numActiveAABBs; ++i) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(OpenGL::AABB), &aabbs[i]);
        offset += sizeof(OpenGL::AABB);
    }
    offset += (numAABBs - numActiveAABBs) * sizeof(OpenGL::AABB);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Initialize necessary buffers for a full-screen quad.
    std::vector<glm::vec3> vertices = {
        { -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }
    };
    std::vector<glm::vec2> uv = {
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
    };
    std::vector<unsigned> indices = { 0, 1, 2, 0, 2, 3 };

    GLuint verticesVBO;
    glGenBuffers(1, &verticesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint uvVBO;
    glGenBuffers(1, &uvVBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(uv[0]), uv.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Attach VBOs.
    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

    // Attach EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindVertexArray(0);

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

    // Compile shaders.
    OpenGL::Shader pathTracingShader { "Path Tracing", { "src/samples/path-tracing/assets/shaders/fsq.vert",
                                                         "src/samples/path-tracing/assets/shaders/path_tracing.frag" } };
    OpenGL::Shader postProcessingShader { "Post Processing", { "src/samples/path-tracing/assets/shaders/fsq.vert",
                                                               "src/samples/path-tracing/assets/shaders/post_processing.frag" } };

    int frameCounter = 0;
    int samplesPerPixel = 4;
    int numRayBounces = 16;

    glBindVertexArray(vao);

    // For the best visual clarity, denoising textures need to be reset when anything in the scene configuration changes.
    // This includes, but is not limited to, properties of the camera and properties of path tracing.
    bool resetRenderTargets = false;

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Start the Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handle resizing the window.
        int tempWidth;
        int tempHeight;
        glfwGetFramebufferSize(window, &tempWidth, &tempHeight);

        if (tempWidth != width || tempHeight != height) {
            // Window dimensions changed, resize content.
            width = tempWidth;
            height = tempHeight;

            blankTexture.resize(width * height * 4, 0.0f); // Inserts or deletes elements appropriately.

            // Reallocate FBO attachments with updated data storage size.
            glBindTexture(GL_TEXTURE_2D, frame1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindTexture(GL_TEXTURE_2D, frame2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Reconstruct custom frame buffer.
            // Note: not sure if this fully necessary, resizing doesn't happen every frame so the performance overhead is negligible.
            glDeleteFramebuffers(1, &fbo);

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame1, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, frame2, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Failed to reinitialize custom framebuffer on window resize." << std::endl;
                break;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Update viewport.
            glViewport(0, 0, width, height);

            // Update camera.
            float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
            camera.SetAspectRatio(aspectRatio);
        }

        // Moving camera.
        const float cameraSpeed = 10.0f;
        const glm::vec3& cameraPosition = camera.GetPosition();
        const glm::vec3& cameraForwardVector = camera.GetForwardVector();
        const glm::vec3& cameraUpVector = camera.GetUpVector();

        bool cameraPositionChanged = false;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraForwardVector * dt);
            cameraPositionChanged = true;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraForwardVector * dt);
            cameraPositionChanged = true;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
            cameraPositionChanged = true;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
            cameraPositionChanged = true;
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraUpVector * dt);
            cameraPositionChanged = true;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraUpVector * dt);
            cameraPositionChanged = true;
        }

        // Mouse input.
        static glm::vec2 previousCursorPosition;
        static bool initialInput = true;

        bool cameraOrientationChanged = false;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            glm::dvec2 cursorPosition;
            glfwGetCursorPos(window, &cursorPosition.x, &cursorPosition.y);

            // FPS camera.
            if (initialInput) {
                previousCursorPosition = cursorPosition;
                initialInput = false;
            }

            float mouseSensitivity = 0.1f;

            float dx = static_cast<float>(cursorPosition.x - previousCursorPosition.x) * mouseSensitivity;
            float dy = static_cast<float>(previousCursorPosition.y - cursorPosition.y) * mouseSensitivity; // Flipped.

            previousCursorPosition = cursorPosition;

            float pitch = glm::degrees(camera.GetPitch());
            float yaw = glm::degrees(camera.GetYaw());
            float roll = glm::degrees(camera.GetRoll());

            float limit = 89.0f;

            yaw += dx;
            pitch += dy;

            if (glm::abs(dx) > std::numeric_limits<float>::epsilon() || glm::abs(dy) > std::numeric_limits<float>::epsilon()) {
                cameraOrientationChanged = true;
            }

            // Prevent camera forward vector to be parallel to camera up vector (0, 1, 0).
            if (pitch > limit) {
                pitch = limit;
            }
            if (pitch < -limit) {
                pitch = -limit;
            }

            camera.SetEulerAngles(pitch, yaw, roll);
        }
        else {
            initialInput = true;
        }

        // Update camera transformation matrices.
        bool isCameraDirty = camera.IsDirty();
        if (isCameraDirty) {
            int offset = 0;

            glBindBuffer(GL_UNIFORM_BUFFER, ubo);

            // Calling any Get...() function recalculates all matrices.
            // Inverse projection matrix.
            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4), glm::value_ptr(glm::inverse(camera.GetPerspectiveTransform())));
            offset += sizeof(glm::mat4);

            // Inverse view matrix.
            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4), glm::value_ptr(glm::inverse(camera.GetViewTransform())));
            offset += sizeof(glm::mat4);

            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::vec3), glm::value_ptr(camera.GetPosition()));

            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        bool receivedImGuiInput = false;

        // Sample overview and statistics.
        if (ImGui::Begin("Sample Overview")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Render time:");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", dt * 1000.0f, 1.0f / dt);

            ImGui::PopStyleColor();
        }
        ImGui::End();

        // Configuration of path tracing options.
        if (ImGui::Begin("Path Tracing Options")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Samples per pixel:");
            int tempSamplesPerPixel = samplesPerPixel;
            if (ImGui::SliderInt("##spp", &tempSamplesPerPixel, 1, 64)) {
                if (tempSamplesPerPixel != samplesPerPixel) {
                    samplesPerPixel = tempSamplesPerPixel;
                    receivedImGuiInput = true;
                }
            }

            ImGui::Text("Number of ray bounces:");
            int tempNumRayBounces = numRayBounces;
            if (ImGui::SliderInt("##numRayBounces", &tempNumRayBounces, 2, 64)) {
                // Manual input can go outside the valid range.
                tempNumRayBounces = glm::clamp(tempNumRayBounces, 2, 64);

                if (tempNumRayBounces != numRayBounces) {
                    numRayBounces = tempNumRayBounces;
                    receivedImGuiInput = true;
                }
            }

            ImGui::PopStyleColor();
        }
        ImGui::End();

        // Configuration of scene camera.
        if (ImGui::Begin("Camera Options")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Exposure:");
            float tempExposure = exposure;
            if (ImGui::SliderFloat("##exposure", &tempExposure, 0.1f, 5.0f)) {
                if (glm::abs(tempExposure - exposure) > std::numeric_limits<float>::epsilon()) {
                    exposure = tempExposure;
                    receivedImGuiInput = true;
                }
            }

            ImGui::Text("Aperture Diameter:");
            float apertureDiameter = apertureRadius * 2.0f;
            if (ImGui::SliderFloat("##apertureDiameter", &apertureDiameter, 0.0f, 10.0f)) {
                if (glm::abs(apertureDiameter - (apertureRadius * 2.0f)) > std::numeric_limits<float>::epsilon()) {
                    apertureRadius = apertureDiameter / 2.0f;
                    receivedImGuiInput = true;
                }
            }

            ImGui::Text("Focus Distance:");
            float tempFocusDistance = focusDistance;
            if (ImGui::SliderFloat("##focusDistance", &tempFocusDistance, 2.0f, 100.0f)) {
                if (glm::abs(tempFocusDistance - focusDistance) > std::numeric_limits<float>::epsilon()) {
                    focusDistance = tempFocusDistance;
                    receivedImGuiInput = true;
                }
            }

            ImGui::PopStyleColor();
        }
        ImGui::End();

        pathTracingShader.Bind();

        pathTracingShader.SetUniform("frameCounter", frameCounter);
        pathTracingShader.SetUniform("samplesPerPixel", samplesPerPixel);
        pathTracingShader.SetUniform("numRayBounces", numRayBounces);
        pathTracingShader.SetUniform("focusDistance", focusDistance);
        pathTracingShader.SetUniform("apertureRadius", apertureRadius);

        int previousFrameIndex = (frameCounter + 1) % 2;
        int currentFrameIndex = (frameCounter % 2);

        GLuint previousFrameImage = previousFrameIndex == 0 ? frame1 : frame2;
        GLuint currentFrameImage = currentFrameIndex == 0 ? frame1 : frame2;

        // Reset the previous frame texture.
        if (cameraPositionChanged || cameraOrientationChanged || receivedImGuiInput) {
            glBindTexture(GL_TEXTURE_2D, previousFrameImage);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Determine which texture is the previous frame and which texture is the current frame.
        // Previous frame is readonly for de-noising, current frame will be copied into the default framebuffer for rendering.
        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, previousFrameImage, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        pathTracingShader.SetUniform("previousFrameImage", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        pathTracingShader.SetUniform("skyboxTexture", 1);

        drawBuffers[0] = GL_COLOR_ATTACHMENT0 + currentFrameIndex;
        glDrawBuffers(1, drawBuffers.data());

        // Render to FBO attachment.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pathTracingShader.Unbind();

        // Render final output to full screen quad.
        postProcessingShader.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, currentFrameImage, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        postProcessingShader.SetUniform("finalImage", 0);

        postProcessingShader.SetUniform("exposure", exposure);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        postProcessingShader.Unbind();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Save ImGui .ini settings.
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantSaveIniSettings) {
            ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());

            // Manually change flag.
            io.WantSaveIniSettings = false;
        }

        glfwSwapBuffers(window);

        ++frameCounter;
        frameCounter %= INT_MAX;

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;
    }

    // Shutdown.
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &uvVBO);
    glDeleteBuffers(1, &verticesVBO);
    glDeleteBuffers(1, &ssbo);
    glDeleteBuffers(1, &ubo);

    ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
