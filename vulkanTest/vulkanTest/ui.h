#ifndef UI_H
#include <cstdio>
#include "vulkan_app.h"

class UI {
public:
  UI(){};
  UI(VulkanApp *vkApp);
  void render(uint32_t i, uint32_t imageIndex);
  void renderMaterialsUI(Material &material);

   static void check_vk_result(VkResult err) {
     if (err == 0)
       return;
     fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
     if (err < 0)
       abort();
   }

private:
  VulkanApp *vkApp;
};
#endif // !UI_H