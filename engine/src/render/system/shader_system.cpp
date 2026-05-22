#include "render/system/shader_system.h"
#include "core/loggersystem.h"
#include "render/rendersystem.h"
#include <fstream>
#include <sstream>
#include <vulkan/vulkan.h>

// 着色器资源根目录（相对于 bin 工作目录）
static const char *SHADER_ASSET_DIR = "assets/shaders/";

// ── 单例 ──────────────────────────────────────────────────────
static mtShaderSystem *g_shaderSystem = nullptr;

mtShaderSystem *mtShaderSystem::instance() {
    if (!g_shaderSystem) {
        g_shaderSystem = new mtShaderSystem();
    }
    return g_shaderSystem;
}

// ── 初始化 / 关闭 ─────────────────────────────────────────────
b8 mtShaderSystem::initialize() {
    // _backend = backend;
    return true;
}

b8 mtShaderSystem::shutdown() {
    for (auto &pair : _programs) {
        mtRenderSystem::getInstance()->destroyProgram(pair.second.program);
        mtRenderSystem::getInstance()->destroyShader(pair.second.vertShader);
        mtRenderSystem::getInstance()->destroyShader(pair.second.fragShader);
    }
    _programs.clear();
    delete g_shaderSystem;
    g_shaderSystem = nullptr;
    return true;
}

// ── 引用计数的 program 获取 ───────────────────────────────────
mtProgramHandle mtShaderSystem::acquireProgram(const std::string &shaderName) {
    // 已经加载过
    auto it = _programs.find(shaderName);
    if (it != _programs.end()) {
        it->second.refCount++;
        return it->second.program;
    }

    std::string name, vertFile, fragFile;
    mtProgramConfig config;

    // 1. 解析 .yaml
    std::string yamlPath = std::string(SHADER_ASSET_DIR) + shaderName + ".yaml";
    if (!parseShaderYaml(yamlPath, name, vertFile, fragFile, config)) {
        return {mtShaderHandle::INVALID_HANDLE};
    }

    // 2. 读取 SPIR-V 二进制
    auto readBinary = [](const std::string &path) -> std::vector<u8> {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) {
            MT_LOG_ERROR("Cannot open shader binary: {}", path);
            return {};
        }
        std::streamsize size = f.tellg();
        f.seekg(0);
        std::vector<u8> buf(size);
        if (size > 0 && !f.read(reinterpret_cast<char *>(buf.data()), size)) {
            return {};
        }
        return buf;
    };

    // MT_LOG_INFO("mtShaderSystem loading file {}", vertFile);
    std::vector<u8> vertBytes = readBinary(std::string(SHADER_ASSET_DIR) + vertFile + ".spv");
    // MT_LOG_INFO("mtShaderSystem loading file {}", fragFile);
    std::vector<u8> fragBytes = readBinary(std::string(SHADER_ASSET_DIR) + fragFile + ".spv");
    if (vertBytes.empty() || fragBytes.empty()) {
        return {mtShaderHandle::INVALID_HANDLE};
    }

    // 3. 创建着色器阶段
    mtShaderHandle vertHandle = mtRenderSystem::getInstance()->createShader(
        vertBytes.data(), static_cast<u32>(vertBytes.size()), mtShaderType::VERTEX);
    mtShaderHandle fragHandle = mtRenderSystem::getInstance()->createShader(
        fragBytes.data(), static_cast<u32>(fragBytes.size()), mtShaderType::FRAGMENT);

    if (vertHandle.id == mtShaderHandle::INVALID_HANDLE || fragHandle.id == mtShaderHandle::INVALID_HANDLE) {
        MT_LOG_ERROR("Failed to create shader modules for '{}'", shaderName);
        return {mtProgramHandle::INVALID_HANDLE};
    }

    // 4. 创建 program
    mtProgramHandle progHandle = mtRenderSystem::getInstance()->createProgram(vertHandle, fragHandle, config);
    if (progHandle.id == mtProgramHandle::INVALID_HANDLE) {
        MT_LOG_ERROR("Failed to create program '{}'", shaderName);
        // 销毁已创建的 shader module
        mtRenderSystem::getInstance()->destroyShader(vertHandle);
        mtRenderSystem::getInstance()->destroyShader(fragHandle);
        return {mtProgramHandle::INVALID_HANDLE};
    }

    // 5. 缓存起来
    ShaderProgram sp;
    sp.vertShader = vertHandle;
    sp.fragShader = fragHandle;
    sp.program = progHandle;
    sp.config = config;
    sp.refCount = 1;
    _programs[shaderName] = sp;

    MT_LOG_INFO("Shader program '{}' loaded (program={})", shaderName, progHandle.id);
    return progHandle;
}

void mtShaderSystem::releaseProgram(const std::string &shaderName) {
    auto it = _programs.find(shaderName);
    if (it == _programs.end())
        return;

    if (--it->second.refCount == 0) {
        mtRenderSystem::getInstance()->destroyProgram(it->second.program);
        mtRenderSystem::getInstance()->destroyShader(it->second.vertShader);
        mtRenderSystem::getInstance()->destroyShader(it->second.fragShader);
        _programs.erase(it);
        MT_LOG_INFO("Shader program '{}' released", shaderName);
    }
}

// ── YAML 解析 (yaml-cpp) ──────────────────────────────────────
static mtUniformType parseUniformType(const std::string &typeStr) {
    if (typeStr == "f32")
        return mtUniformType::F32;
    if (typeStr == "vec2")
        return mtUniformType::VEC2;
    if (typeStr == "vec3")
        return mtUniformType::VEC3;
    if (typeStr == "vec4")
        return mtUniformType::VEC4;
    if (typeStr == "s8")
        return mtUniformType::S8;
    if (typeStr == "u8")
        return mtUniformType::U8;
    if (typeStr == "s16")
        return mtUniformType::S16;
    if (typeStr == "u16")
        return mtUniformType::U16;
    if (typeStr == "s32")
        return mtUniformType::S32;
    if (typeStr == "u32")
        return mtUniformType::U32;
    if (typeStr == "mat3")
        return mtUniformType::MAT3;
    if (typeStr == "mat4")
        return mtUniformType::MAT4;
    if (typeStr == "samp" || typeStr == "sampler2D")
        return mtUniformType::SAMPER2D;
    MT_LOG_WARN("Unknown uniform type '{}', fallback to F32", typeStr);
    return mtUniformType::F32;
}

b8 mtShaderSystem::parseShaderYaml(const std::string &yamlPath, std::string &outName, std::string &outVert,
                                   std::string &outFrag, mtProgramConfig &outConfig) {
    try {
        YAML::Node doc = YAML::LoadFile(yamlPath);

        if (!doc["name"]) {
            MT_LOG_ERROR("Missing 'name' in shader yaml: {}", yamlPath);
            return false;
        }
        outName = doc["name"].as<std::string>();

        if (!doc["vert"]) {
            MT_LOG_ERROR("Missing 'vert' in shader yaml: {}", yamlPath);
            return false;
        }
        outVert = doc["vert"].as<std::string>();

        if (!doc["frag"]) {
            MT_LOG_ERROR("Missing 'frag' in shader yaml: {}", yamlPath);
            return false;
        }
        outFrag = doc["frag"].as<std::string>();

        if (doc["uniforms"] && doc["uniforms"].IsSequence()) {
            for (const auto &uniNode : doc["uniforms"]) {
                mtUniform uni;

                if (uniNode["name"]) {
                    uni.name = uniNode["name"].as<std::string>();
                } else {
                    MT_LOG_WARN("Uniform entry missing 'name' in {}, skipping", yamlPath);
                    continue;
                }

                uni.type = uniNode["type"] ? parseUniformType(uniNode["type"].as<std::string>()) : mtUniformType::F32;

                uni.scope =
                    uniNode["scope"] ? static_cast<mtUniformScope>(uniNode["scope"].as<u32>()) : mtUniformScope::GLOBAL;

                uni.location = static_cast<u32>(outConfig.uniforms.size());

                outConfig.uniforms.push_back(uni);
            }
        }

        MT_LOG_INFO("Parsed shader yaml: {} (vert={}, frag={}, uniforms={})", outName, outVert, outFrag,
                    outConfig.uniforms.size());
        return true;

    } catch (const YAML::Exception &e) {
        MT_LOG_ERROR("YAML parse error in {}: {}", yamlPath, e.what());
        return false;
    }
}
