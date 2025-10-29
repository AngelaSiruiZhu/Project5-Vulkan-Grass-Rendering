#include <vulkan/vulkan.h>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"

#define ADD_INTERACTIVE_SPHERE 1

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

namespace {
    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(device->GetVkDevice());
        swapChain->Recreate();
        renderer->RecreateFrameResources();
    }

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    double previousX = 0.0;
    double previousY = 0.0;

    void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                leftMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                leftMouseDown = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                rightMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                rightMouseDown = false;
            }
        }
    }

    void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
        if (leftMouseDown) {
            double sensitivity = 0.5;
            float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
            float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

            camera->UpdateOrbit(deltaX, deltaY, 0.0f);

            previousX = xPosition;
            previousY = yPosition;
        } else if (rightMouseDown) {
            double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);

            camera->UpdateOrbit(0.0f, 0.0f, deltaZ);

            previousY = yPosition;
        }
    }
}

int main() {
    static constexpr char* applicationName = "Vulkan Grass Rendering";
    InitializeWindow(640, 480, applicationName);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    Instance* instance = new Instance(applicationName, glfwExtensionCount, glfwExtensions);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.tessellationShader = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);

    swapChain = device->CreateSwapChain(surface, 5);

    camera = new Camera(device, 640.f / 480.f);

    VkCommandPoolCreateInfo transferPoolInfo = {};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Transfer];
    transferPoolInfo.flags = 0;

    VkCommandPool transferCommandPool;
    if (vkCreateCommandPool(device->GetVkDevice(), &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkImage grassImage;
    VkDeviceMemory grassImageMemory;
    Image::FromFile(device,
        transferCommandPool,
        "images/grass.jpg",
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        grassImage,
        grassImageMemory
    );

#if ADD_INTERACTIVE_SPHERE
    VkImage sphereImage;
    VkDeviceMemory sphereImageMemory;
    Image::FromFile(device,
        transferCommandPool,
        "images/sphere.jpg",
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        sphereImage,
        sphereImageMemory
    );
#endif

    float planeDim = 15.f;
    float halfWidth = planeDim * 0.5f;
    Model* plane = new Model(device, transferCommandPool,
        {
            { { -halfWidth, 0.0f, halfWidth }, { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f } },
            { { halfWidth, 0.0f, halfWidth }, { 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f } },
            { { halfWidth, 0.0f, -halfWidth }, { 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
            { { -halfWidth, 0.0f, -halfWidth }, { 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f } }
        },
        { 0, 1, 2, 2, 3, 0 }
    );
    plane->SetTexture(grassImage);

#if ADD_INTERACTIVE_SPHERE
    // create sphere mesh
    std::vector<Vertex> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    
    int latitudes = 20;
    int longitudes = 20;
    float sphereRadius = 1.0f;

    for (int lat = 0; lat <= latitudes; ++lat) {
        float theta = lat * 3.14159f / latitudes;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= longitudes; ++lon) {
            float phi = lon * 2.0f * 3.14159f / longitudes;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            glm::vec3 position(sphereRadius * cosPhi * sinTheta, sphereRadius * cosTheta, sphereRadius * sinPhi * sinTheta);
            glm::vec3 color(1.0f, 0.4f, 0.7f);
            glm::vec2 uv(static_cast<float>(lon) / longitudes, static_cast<float>(lat) / latitudes);
            
            sphereVertices.push_back({ position, color, uv });
        }
    }
    
    for (int lat = 0; lat < latitudes; ++lat) {
        for (int lon = 0; lon < longitudes; ++lon) {
            int current = lat * (longitudes + 1) + lon;
            int next = current + longitudes + 1;
            
            sphereIndices.push_back(current);
            sphereIndices.push_back(next);
            sphereIndices.push_back(current + 1);
            
            sphereIndices.push_back(current + 1);
            sphereIndices.push_back(next);
            sphereIndices.push_back(next + 1);
        }
    }
    
    Model* sphere = new Model(device, transferCommandPool, sphereVertices, sphereIndices);
    sphere->SetTexture(sphereImage);
#endif
    
    Blades* blades = new Blades(device, transferCommandPool, planeDim);

    vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

    Scene* scene = new Scene(device);
    scene->AddModel(plane);
#if ADD_INTERACTIVE_SPHERE
    scene->AddModel(sphere);
    scene->SetSphereModel(sphere);
#endif
    scene->AddBlades(blades);

    renderer = new Renderer(device, swapChain, scene, camera);

    glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);

    while (!ShouldQuit()) {
        glfwPollEvents();

#if ADD_INTERACTIVE_SPHERE
        float moveSpeed = 0.01f;
        if (glfwGetKey(GetGLFWWindow(), GLFW_KEY_W) == GLFW_PRESS) {
            scene->MoveSphere(0.0f, 0.0f, -moveSpeed);
        }
        if (glfwGetKey(GetGLFWWindow(), GLFW_KEY_S) == GLFW_PRESS) {
            scene->MoveSphere(0.0f, 0.0f, moveSpeed);
        }
        if (glfwGetKey(GetGLFWWindow(), GLFW_KEY_A) == GLFW_PRESS) {
            scene->MoveSphere(-moveSpeed, 0.0f, 0.0f);
        }
        if (glfwGetKey(GetGLFWWindow(), GLFW_KEY_D) == GLFW_PRESS) {
            scene->MoveSphere(moveSpeed, 0.0f, 0.0f);
        }
#endif
        
        scene->UpdateTime();
        renderer->Frame();
    }

    vkDeviceWaitIdle(device->GetVkDevice());

    vkDestroyImage(device->GetVkDevice(), grassImage, nullptr);
    vkFreeMemory(device->GetVkDevice(), grassImageMemory, nullptr);
    
#if ADD_INTERACTIVE_SPHERE
    vkDestroyImage(device->GetVkDevice(), sphereImage, nullptr);
    vkFreeMemory(device->GetVkDevice(), sphereImageMemory, nullptr);
#endif

    delete scene;
    delete plane;
#if ADD_INTERACTIVE_SPHERE
    delete sphere;
#endif
    delete blades;
    delete camera;
    delete renderer;
    delete swapChain;
    delete device;
    delete instance;
    DestroyWindow();
    return 0;
}
