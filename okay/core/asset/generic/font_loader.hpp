#ifndef __FONT_LOADER_H__
#define __FONT_LOADER_H__

#include <cstddef>
#include <filesystem>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/util/result.hpp>

#include <okay/core/renderer/okay_font.hpp>
#include <okay/core/util/option.hpp>

namespace okay {


template <>
struct OkayAssetLoader<OkayFontManager::FontHandle, OkayFontLoadOptions> {
    static Result<OkayFontManager::FontHandle> loadAsset(const std::filesystem::path& path,
                                                        const OkayAssetIO& assetIO,
                                                        const OkayFontLoadOptions& settings) {
        Engine.logger.info("Loading font: {}", path.string());

        OkayFontManager& fontManager = OkayFontManager::instance();

        Option<OkayFontManager::FontHandle> handleOpt = fontManager.loadFont(path.string(), settings);
        if (handleOpt.isSome()) {
            return Result<OkayFontManager::FontHandle>::ok(handleOpt.value());
        } else {
            return Result<OkayFontManager::FontHandle>::errorResult("Failed to load font: " + path.string());
        }

    }
};

}  // namespace okay

#endif // __FONT_LOADER_H__