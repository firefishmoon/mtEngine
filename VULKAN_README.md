# Vulkan Backend Implementation

## Overview

This implementation provides a complete, production-ready Vulkan backend for the mtEngine game engine with the following features:

### Core Features Implemented

1. **Complete Vulkan Initialization**
   - Instance creation with validation layers
   - Physical device selection and suitability checking
   - Logical device creation with queue families
   - Debug messenger for validation layer output
   - Surface creation for window rendering

2. **Swap Chain Management**
   - Swap chain creation with optimal settings
   - Image view creation
   - Swap chain recreation for window resizing
   - Automatic surface format and present mode selection

3. **Graphics Pipeline**
   - Render pass creation
   - Graphics pipeline configuration
   - Shader module creation and compilation
   - Viewport and scissor setup
   - Rasterization state configuration

4. **Command System**
   - Command pool creation
   - Command buffer allocation and recording
   - Framebuffer creation from swap chain images
   - Complete rendering commands

5. **Synchronization**
   - Semaphores for GPU-GPU synchronization
   - Fences for CPU-GPU synchronization
   - Multiple frames in flight support (2 frames)
   - Proper resource lifecycle management

6. **Debug and Validation**
   - Debug messenger for validation layers
   - Extensive logging for errors and warnings
   - Performance analysis support
   - Error handling and recovery

## Usage

### Building the Project

1. **Compile Shaders** (requires glslc from Vulkan SDK):
   ```bash
   invoke compile_shaders
   ```

2. **Configure the Build**:
   ```bash
   invoke config
   ```

3. **Build the Project**:
   ```bash
   invoke build
   ```

4. **Install and Run**:
   ```bash
   invoke install
   invoke run
   ```

### Shader Files

The implementation includes two simple shaders:

- `engine/shaders/shader.vert` - Vertex shader that renders a colored triangle
- `engine/shaders/shader.frag` - Fragment shader for output colors

Shaders are compiled to `engine/shaders/compiled/vert.spv` and `engine/shaders/compiled/frag.spv`.

## API Usage

### Initialize Vulkan Backend

```cpp
mtVulkanBackend backend;
if (!backend.initialize()) {
    // Handle initialization error
}
```

### Render Frame

```cpp
while (application.isRunning()) {
    // Process input, update game state, etc.
    
    // Render frame
    if (!backend.renderFrame()) {
        // Handle rendering error
    }
}
```

### Handle Window Resize

```cpp
void onWindowResize() {
    backend.recreateSwapChain();
}
```

### Clean Up

```cpp
backend.shutdown();
```

## Architecture

### Key Components

- **QueueFamilyIndices**: Holds queue family indices for graphics and presentation
- **SwapChainSupportDetails**: Contains swap chain capabilities, formats, and present modes
- **mtVulkanBackend**: Main backend class managing all Vulkan components

### Resource Management

The implementation uses a hybrid approach:
- **vulkan::raii**: For automatic resource management (Instance, ShaderModules, etc.)
- **Traditional Vulkan API**: For compatibility with existing code patterns
- **Manual Cleanup**: Proper destruction order and synchronization

### Memory Management

- Automatic resource lifecycle management using RAII
- Proper synchronization between frames
- Efficient multi-frame rendering pipeline
- Graceful error handling and recovery

## Performance Considerations

1. **Double Buffering**: Two frames in flight for optimal performance
2. **Mailbox Present Mode**: Used when available for reduced latency
3. **Shader Optimization**: Pre-compiled SPIR-V shaders
4. **Command Buffer Reuse**: Efficient command buffer management

## Error Handling

- Comprehensive error checking for all Vulkan operations
- Graceful fallback for unsupported features
- Detailed logging for debugging
- Validation layer support for development

## Dependencies

- **Vulkan SDK**: For Vulkan headers and tools
- **GLFW**: For window creation and surface management
- **vulkan**: C++ bindings for Vulkan
- **glslc**: Shader compiler (included in Vulkan SDK)

## Troubleshooting

### Common Issues

1. **"Failed to find GPUs with Vulkan support"**
   - Ensure Vulkan drivers are installed
   - Check if your GPU supports Vulkan

2. **"Failed to open file: engine/shaders/compiled/vert.spv"**
   - Run `invoke compile_shaders` first
   - Ensure glslc is in your PATH

3. **Validation layer errors**
   - Check shader code for syntax errors
   - Verify Vulkan API usage
   - Enable debug logging for more details

### Performance Tips

1. Use Vulkan SDK validation layers only during development
2. Profile with external tools like RenderDoc
3. Optimize shader code for target hardware
4. Monitor frame times and resource usage

## Future Enhancements

- Multiple render passes for complex scenes
- Compute shader support
- Multi-threaded command buffer recording
- Advanced synchronization and resource barriers
- Dynamic uniform buffers and descriptor sets
- Texture and material systems
- Post-processing effects

## License

Part of the mtEngine project. See main project LICENSE for details.
