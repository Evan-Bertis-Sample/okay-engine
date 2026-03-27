#ifndef __FONT_LOADER_H__
#define __FONT_LOADER_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/renderer/font.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/result.hpp>

#include <filesystem>

namespace okay {

template <>
struct AssetLoader<FontManager::FontHandle, FontLoadOptions> {
    static Result<FontManager::FontHandle> loadAsset(const std::filesystem::path& path,
                                                     const AssetIO& assetIO,
                                                     const FontLoadOptions& settings) {
        Engine.logger.info("Loading font: {}", path.string());

        FontManager& fontManager = FontManager::instance();

        Option<FontManager::FontHandle> handleOpt = fontManager.loadFont(path.string(), settings);
        if (handleOpt.isSome()) {
            return Result<FontManager::FontHandle>::ok(handleOpt.value());
        } else {
            return Result<FontManager::FontHandle>::errorResult("Failed to load font: " +
                                                                path.string());
        }
    }
};

}  // namespace okay

#endif  // __FONT_LOADER_H__