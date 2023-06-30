#include "ui.h"
// #include "vulkan_app.h"

UI::UI(VulkanApp *vkApp) {

  this->vkApp = vkApp;
  /* 1: create descriptor pool for IMGUI
   the size of the pool is very oversize, but it's copied from imgui demo itself. */
  VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets                    = 1000;
  pool_info.poolSizeCount              = std::size(pool_sizes);
  pool_info.pPoolSizes                 = pool_sizes;

  VkDescriptorPool imguiPool;
  VK_CHECK(vkCreateDescriptorPool(vkApp->device, &pool_info, nullptr, &imguiPool));

  // 2: initialize imgui library

  // this initializes the core structures of imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // this initializes imgui for GLFW
  ImGui_ImplGlfw_InitForVulkan(vkApp->window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = vkApp->instance;
  init_info.PhysicalDevice            = vkApp->physicalDevice;
  init_info.Device                    = vkApp->device;
  init_info.QueueFamily               = vkApp->findQueueFamilies(vkApp->physicalDevice).graphicsFamily.value();
  init_info.Queue                     = vkApp->graphicsQueue;
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = imguiPool;
  init_info.Subpass                   = 0;
  init_info.MinImageCount             = 2;
  init_info.ImageCount                = 2; // wd.ImageCount;
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator                 = nullptr;
  init_info.CheckVkResultFn           = check_vk_result;

  ImGui_ImplVulkan_Init(&init_info, vkApp->renderPass);

  // execute a gpu command to upload imgui font textures
  // immediate_submit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });
  Command         cmd           = Command(vkApp->device, vkApp->commandPool, vkApp->graphicsQueue);
  VkCommandBuffer commandBuffer = cmd.beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
  cmd.endSingleTimeCommands();

  // clear font textures from cpu data
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void UI::render(uint32_t i, uint32_t imageIndex) {

  // Start a new render pass to draw the UI overlay on top of the ray traced image
  VkClearValue clear_values[2];
  clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
  clear_values[1].depthStencil = {0.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass               = vkApp->renderPass;
  renderPassBeginInfo.framebuffer              = vkApp->swapChainFramebuffers[imageIndex];
  renderPassBeginInfo.renderArea.extent.width  = vkApp->appSettings.width;
  renderPassBeginInfo.renderArea.extent.height = vkApp->appSettings.height;
  renderPassBeginInfo.clearValueCount          = 2;
  renderPassBeginInfo.pClearValues             = clear_values;

  vkCmdBeginRenderPass(vkApp->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Our state
  bool   show_demo_window    = true;
  bool   show_another_window = false;
  ImVec4 clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Start the Dear ImGui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to
  // learn more about Dear ImGui!).
  if (show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
  {
    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    if (ImGui::SliderInt("Path depth", (int *)&vkApp->constants.maxDepth, 0, 10)) {
      vkApp->constants.frameNumber = 0;
    }
    if (ImGui::SliderInt("Samples ", (int *)&vkApp->constants.numSamples, 0, 10)) {
      vkApp->constants.frameNumber = 0;
    }

    ImGui::Text("Frame = %d", vkApp->constants.frameNumber);
    ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color
    ImGui::End();
  }

  {
    // Start ImGui frame
    ImGui::Begin("Material Editor");

    // Display the UI for editing the materials
    if (ImGui::CollapsingHeader("Materials")) {
      for (size_t i = 0; i < vkApp->materials.size(); i++) {
        if (ImGui::BeginCombo(std::string("Material " + std::to_string(i + 1)).c_str(), nullptr)) {
          renderMaterialsUI(vkApp->materials[i]);
          ImGui::EndCombo();
        }
      }
    }

    // End ImGui frame
    ImGui::End();
  }

  // 3. Show another simple window.
  if (show_another_window) {
    ImGui::Begin("Another Window", &show_another_window);
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }

  ImGui::Render();
  auto       &io        = ImGui::GetIO();
  ImDrawData *draw_data = ImGui::GetDrawData();

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, vkApp->commandBuffers[i]);
  vkCmdEndRenderPass(vkApp->commandBuffers[i]);
}

void UI::renderMaterialsUI(Material &material) {
  ImGui::Text("Material Type");
  const char *materialTypes[]  = {"Specular", "Rough", "Diffuse"};
  const char *ndfTypes[]       = {"GGX", "Beckmann", "Phong"};
  const int   numMaterialTypes = sizeof(materialTypes) / sizeof(materialTypes[0]);
  const int   numNdfTypes      = sizeof(ndfTypes) / sizeof(ndfTypes[0]);

  if (ImGui::Combo("##MaterialType", (int *)&material.materialType, materialTypes, numMaterialTypes)) {
    // Handle material type change
    vkApp->state.materialsHaveChanged = true;
  }

  if (ImGui::Combo("##Ndf", (int *)&material.ndf, ndfTypes, numNdfTypes)) {
    // Handle material type change
    vkApp->state.materialsHaveChanged = true;
  }

  ImGui::Text("Diffuse Color");
  if (ImGui::ColorEdit3("##DiffuseColor", glm::value_ptr(material.diffuse))) {
    // Handle diffuse color change
    vkApp->state.materialsHaveChanged = true;
  }

  ImGui::Text("Specular Color");
  if (ImGui::ColorEdit3("##SpecularColor", glm::value_ptr(material.specular))) {
    // Handle specular color change
    vkApp->state.materialsHaveChanged = true;
  }

  ImGui::Text("Emission Color");
  if (ImGui::ColorEdit3("##EmissionColor", glm::value_ptr(material.emission))) {
    // Handle emission color change
    vkApp->state.materialsHaveChanged = true;
  }

  ImGui::Text("Roughness");
  if (ImGui::SliderFloat("##Roughness", &material.roughness, 0.0f, 1.0f)) {
    // Handle roughness change
    vkApp->state.materialsHaveChanged = true;
  }

  ImGui::Text("Shininess");
  if (ImGui::SliderFloat("##Shininess", &material.shininess, 0.0f, 100.0f)) {
    // Handle shininess change
    vkApp->state.materialsHaveChanged = true;
  }
}
