#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"

class GameRenderer : public VulkanApplication {
public:
    GameRenderer(Input& input)
        : VulkanApplication(input) {}
    void bindScene(Scene const & scene);

private:
    void loadModels(const prt::vector<Model>& models);

    void loadSkybox(const prt::array<Texture, 6>& skybox);

    void createRenderJobs(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices);
};

#endif