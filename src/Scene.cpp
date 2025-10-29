#include "Scene.h"
#include "BufferUtils.h"
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(Device* device) : device(device), sphereModel(nullptr) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));
    
    sphereData.positionRadius = glm::vec4(0.0f, 1.5f, 0.0f, 1.0f);
}

const std::vector<Model*>& Scene::GetModels() const {
    return models;
}

const std::vector<Blades*>& Scene::GetBlades() const {
  return blades;
}

void Scene::AddModel(Model* model) {
    models.push_back(model);
}

void Scene::AddBlades(Blades* blades) {
  this->blades.push_back(blades);
}

void Scene::UpdateTime() {
    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    duration<float> nextDeltaTime = duration_cast<duration<float>>(currentTime - startTime);
    startTime = currentTime;

    time.deltaTime = nextDeltaTime.count();
    time.totalTime += time.deltaTime;

    memcpy(mappedData, &time, sizeof(Time));
}

VkBuffer Scene::GetTimeBuffer() const {
    return timeBuffer;
}

void Scene::SetSphereModel(Model* model) {
    sphereModel = model;
    UpdateSphereTransform();
}

void Scene::MoveSphere(float dx, float dy, float dz) {
    sphereData.positionRadius.x += dx;
    sphereData.positionRadius.y += dy;
    sphereData.positionRadius.z += dz;
    UpdateSphereTransform();
}

void Scene::UpdateSphereTransform() {
    if (sphereModel != nullptr) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(sphereData.positionRadius.x, sphereData.positionRadius.y, sphereData.positionRadius.z));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(sphereData.positionRadius.w));
        sphereModel->UpdateModelMatrix(modelMatrix);
    }
}

const SphereData& Scene::GetSphereData() const {
    return sphereData;
}

Scene::~Scene() {
    vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
    vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
    vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);
}
