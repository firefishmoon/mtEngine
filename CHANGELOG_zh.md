# mtEngine 渲染系统修改记录

## 概述

本次修改实现了基于 Vulkan 的渲染器对 **Shader/Program 管理**和 **Uniform 更新**的完整支持，并将 `mtGeometry` 的渲染流程从固定 Shader + 硬编码纹理 更新为 **传入 Program + 动态更新 Uniform** 的方式。

---

## 一、`render_types.h` — 核心类型更新

| 修改内容 | 说明 |
|---------|------|
| 新增 `mtProgramHandle` 结构体 | 包含 `id` 和 `INVALID_HANDLE = 0xFFFF`，用于标识 GPU 端的程序对象 |
| 新增 `mtProgram` 结构体 | 包含 `id` 和 `internalData`，CPU 端管理程序生命周期 |
| `mtGeometry` 新增 `programHandle` 字段 | 允许每个几何体绑定一个自定义程序，替代原有的固定 Shader 渲染路径 |

---

## 二、`irenderbackend.h` — 渲染后端接口扩展

新增以下纯虚方法，定义跨后端的统一 API：

```cpp
// Shader 管理
virtual mtShaderHandle createShader(const u8* data, u32 dataSize, u32 stage) = 0;
virtual void destroyShader(mtShaderHandle handle) = 0;

// Program 管理
virtual mtProgramHandle createProgram(mtShaderHandle vertexShader, mtShaderHandle fragmentShader, mtProgramConfig& config) = 0;
virtual void destroyProgram(mtProgramHandle handle) = 0;

// Uniform 更新
virtual void updateUniform(mtProgramHandle program, const std::string& name, const void* data, u32 size) = 0;
```

---

## 三、`rendersystem.h` / `rendersystem.cpp` — 渲染系统实现

### 改动点：

1. **新增成员变量**：
   - `mtShader _shaderPool[MT_SHADER_MAX_COUNT]` — Shader 对象池
   - `mtProgram _programPool[MT_SHADER_MAX_COUNT]` — Program 对象池
   - `u32 _shaderCreateIndex` / `u32 _programCreateIndex` — 分配索引

2. **`initialize()`**：初始化 Shader 和 Program 池（所有 id 置 0）

3. **新增 `createShader()`**：在 Shader 池中查找空闲槽位，调用后端创建 Shader，返回 `mtShaderHandle`

4. **新增 `destroyShader()`**：调用后端销毁 Shader，释放池位

5. **新增 `createProgram()`**：在 Program 池中查找空闲槽位，调用后端创建 Program，返回 `mtProgramHandle`

6. **`draw()` 流程重构**：
   - 安全查找纹理（检查 `textureHandle` 是否有效）
   - 若 `geometry.programHandle` 有效，则通过 `updateUniform()` 动态更新纹理等 Uniform
   - 调用 `updateObject()` 执行绘制

---

## 四、新增 `vulkan_program.h` / `vulkan_program.cpp` — Vulkan 程序管理

### `mtVulkanProgram` 结构体

管理一个 Vulkan 图形程序的所有 GPU 资源：

- `VkPipeline pipeline` — 图形管线
- `VkPipelineLayout pipelineLayout` — 管线布局
- `VkDescriptorSetLayout descriptorSetLayout` — 描述符集布局
- `VkDescriptorPool descriptorPool` — 描述符池
- `VkDescriptorSet descriptorSet` — 描述符集
- `std::vector<VkDescriptorSetLayoutBinding> bindings` — 绑定描述
- `std::vector<VkDescriptorPoolSize> poolSizes` — 池大小配置
- `std::unique_ptr<mtVulkanBuffer> uniformBuffer` — Uniform 缓冲区

### `mtVulkanProgramManager` 类

核心管理类，负责：

1. **Shader 模块管理**（引用计数）：
   - `createShaderModule(data, dataSize, stage)` — 从 SPIR-V 字节创建 `VkShaderModule`
   - `destroyShaderModule(module)` — 引用计数减 1，为 0 时销毁

2. **Program 创建与销毁**：
   - `createProgram(handle, vertModule, fragModule, config)` — 根据 `mtProgramConfig` 中的 Uniform 描述自动创建描述符集布局、描述符池、Uniform Buffer、管线布局和图形管线
   - `destroyProgram(handle)` — 销毁所有关联的 GPU 资源

3. **Uniform 更新**：
   - `updateUniform(programHandle, name, data, size)` — 查找绑定并更新描述符（支持 UBO 和 Combined Image Sampler 两种类型）

4. **管线创建细节**：
   - 使用硬编码的顶点输入格式（Position R32G32B32 + UV R32G32）
   - 支持动态 Viewport/Scissor
   - 从 `mtVulkanMaterialShader` 获取 `VkRenderPass` 句柄
   - Push Constant 用于传递 Model 矩阵

---

## 五、`vulkan_backend.h` / `vulkan_backend.cpp` — Vulkan 后端更新

### 新增成员：

- `std::unique_ptr<mtVulkanProgramManager> _programManager` — 程序管理器
- `mtShader _shaderPool[MT_SHADER_MAX_COUNT]` — Shader 池
- `mtProgram _programPool[MT_SHADER_MAX_COUNT]` — Program 池

### 接口实现：

| 方法 | 实现 |
|------|------|
| `createShader()` | 调用 `_programManager->createShaderModule()`，在池中注册 |
| `destroyShader()` | 查找池中对应模块，调用 `_programManager->destroyShaderModule()` |
| `createProgram()` | 查找顶/片段 Shader 模块，调用 `_programManager->createProgram()` |
| `destroyProgram()` | 调用 `_programManager->destroyProgram()`，释放池位 |
| `updateUniform()` | 委托给 `_programManager->updateUniform()` |

### `updateObject()` 重构：

- **自定义程序路径**：若 `geometry.programHandle` 有效，绑定自定义管线、描述符集、推送常量
- **内置 Shader 路径**：否则使用原有的 `mtVulkanMaterialShader` 流程
- 两种路径共享同一份顶点/索引缓冲区绑定和 `vkCmdDrawIndexed` 调用

### 初始化：

- 在 `initialize()` 中创建 `_programManager` 实例
- 在 `renderPrepare()` 中同步设备端的交换链尺寸

---

## 六、`vulkan_device.h` / `vulkan_device.cpp` — 设备扩展

- 新增 `_swapChainWidth`、`_swapChainHeight` 成员变量
- 新增 `getSwapChainWidth()`、`getSwapChainHeight()`、`setSwapChainDimensions()` 访问器
- 供 `mtVulkanProgramManager` 创建管线时获取当前交换链尺寸

---

## 七、`vulkan_swapchain.h` / `vulkan_swapchain.cpp` — 交换链扩展

- 新增 `_width`、`_height` 成员
- 在 `create()` 中记录尺寸
- 新增 `getWidth()`、`getHeight()` 访问器

---

## 八、`vulkan_material_shader.h` / `vulkan_material_shader.cpp` — 材质着色器

- 声明并实现 `getRenderPassHandle()` 方法，返回内置渲染管线的 `VkRenderPass` 句柄
- 供自定义程序管线创建时使用（避免循环依赖，通过 `.cpp` 文件实现内联方法）

---

## 九、渲染流程对比

### 旧流程（固定 Shader）：
```
draw(geometry)
  → updateGlobalState(projection, view)     // 更新全局 UBO
  → updateObject(geometry)                   // 内置材质着色器处理纹理 + 绘制
      → materialShader.updateObject()        //   设置 push constant model
      → materialShader.use()                 //   绑定管线
      → vkCmdDrawIndexed()                   //   绘制
```

### 新流程（动态 Program + Uniform）：
```
draw(geometry)
  → updateGlobalState(projection, view)     // 更新全局 UBO（内置 Shader）
  → updateUniform(program, "u_DiffuseTexture", ...)  // 动态更新程序 Uniform
  → updateObject(geometry)                   // 根据 programHandle 分发
      → [自定义程序路径]
      |   → vkCmdBindPipeline(customPipeline)
      |   → vkCmdBindDescriptorSets()
      |   → vkCmdPushConstants(model)
      |   → vkCmdDrawIndexed()
      → [内置 Shader 路径]
          → materialShader.updateObject()
          → materialShader.use()
          → vkCmdDrawIndexed()
```

---

## 十、文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `engine/src/render/render_types.h` | 修改 | 新增 `mtProgramHandle`、`mtProgram`，`mtGeometry` 加 `programHandle` |
| `engine/src/render/irenderbackend.h` | 修改 | 新增 `createShader/destroyShader/createProgram/destroyProgram/updateUniform` 纯虚接口 |
| `engine/src/render/rendersystem.h` | 修改 | 新增 Shader/Program 池和方法声明 |
| `engine/src/render/rendersystem.cpp` | 修改 | 实现 Shader/Program 管理，更新 `draw()` 流程 |
| `engine/src/render/vulkan/vulkan_program.h` | **新增** | `mtVulkanProgram` + `mtVulkanProgramManager` 声明 |
| `engine/src/render/vulkan/vulkan_program.cpp` | **新增** | 实现 Shader 管理、Program 创建/销毁、Uniform 更新 |
| `engine/src/render/vulkan/vulkan_backend.h` | 修改 | 新增成员变量和方法声明 |
| `engine/src/render/vulkan/vulkan_backend.cpp` | 修改 | 实现所有新增接口方法，重构 `updateObject()` |
| `engine/src/render/vulkan/vulkan_device.h` | 修改 | 新增交换链尺寸字段和访问器 |
| `engine/src/render/vulkan/vulkan_device.cpp` | — | 无变更（已有结构保持） |
| `engine/src/render/vulkan/vulkan_swapchain.h` | 修改 | 新增尺寸字段和访问器 |
| `engine/src/render/vulkan/vulkan_swapchain.cpp` | 修改 | 在 `create()` 中记录尺寸 |
| `engine/src/render/vulkan/shaders/vulkan_material_shader.h` | 修改 | 声明 `getRenderPassHandle()` |
| `engine/src/render/vulkan/shaders/vulkan_material_shader.cpp` | 修改 | 实现 `getRenderPassHandle()` |