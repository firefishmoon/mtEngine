#pragma once

#include "defines.h"
#include "render/irenderbackend.h"
#include "render/render_types.h"
#include "yaml-cpp/yaml.h"
#include <string>
#include <unordered_map>
#include <vector>

class MT_API mtShaderSystem {
  public:
    b8 initialize();
    b8 shutdown();

    // 通过名字获取/加载一个 program（引用计数）
    mtProgramHandle acquireProgram(const std::string &shaderName);
    void releaseProgram(const std::string &shaderName);

    // 单例访问
    static mtShaderSystem *instance();

  private:
    struct ShaderProgram {
        mtShaderHandle vertShader = {mtShaderHandle::INVALID_HANDLE};
        mtShaderHandle fragShader = {mtShaderHandle::INVALID_HANDLE};
        mtProgramHandle program = {mtProgramHandle::INVALID_HANDLE};
        mtProgramConfig config;
        u32 refCount = 0;
    };

    // mtIRenderBackend *_backend = nullptr;
    std::unordered_map<std::string, ShaderProgram> _programs;

    // ----- YAML 解析 (yaml-cpp) -----
    static b8 parseShaderYaml(const std::string &yamlPath, std::string &outName, std::string &outVert,
                              std::string &outFrag, mtProgramConfig &outConfig);
};
