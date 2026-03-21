#pragma once

#include "defines.h"
#include "common/singleton.h"
#include "render/render_types.h"
#include <map>

class MT_API mtTextureSystem : public Singleton<mtTextureSystem> {
public:
    b8 initialize() override;
    b8 shutdown() override;

    mtTextureHandle acquireTexture(const std::string& name, mtTextureInfo& outInfo, bool autoRelease);

    void releaseTexture(const std::string& name);
private:
    b8 loadTextureFromFile(mtTextureHandle& handle, const std::string& name, mtTextureInfo& outInfo);

    struct mtTextureReferences {
        mtTextureHandle handle;
        b8 autoRelease;
        u32 references;
    };

    std::map<const std::string, mtTextureReferences> _textureMap;
};
