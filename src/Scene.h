#pragma once

#include <glm/glm.hpp>
#include <chrono>

#include "Model.h"
#include "Blades.h"

using namespace std::chrono;

struct Time {
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

struct SphereData {
    glm::vec4 positionRadius;
};

class Scene {
private:
    Device* device;
    
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;
    
    SphereData sphereData;
    Model* sphereModel;
    
    void* mappedData;

    std::vector<Model*> models;
    std::vector<Blades*> blades;

high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device);
    ~Scene();

    const std::vector<Model*>& GetModels() const;
    const std::vector<Blades*>& GetBlades() const;
    
    void AddModel(Model* model);
    void AddBlades(Blades* blades);

    VkBuffer GetTimeBuffer() const;

    void UpdateTime();
    float GetFPS() const;
    
    void SetSphereModel(Model* model);
    void MoveSphere(float dx, float dy, float dz);
    void UpdateSphereTransform();
    const SphereData& GetSphereData() const;
};
