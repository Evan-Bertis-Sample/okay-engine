#ifndef __OKAY_LEVEL_MANAGER_H__
#define __OKAY_LEVEL_MANAGER_H__

#include <okay/core/system/okay_system.hpp>
#include <okay/core/level/okay_level.hpp>

#include <string>

namespace okay {

struct OkayLevelManagerSettings {
    std::string LevelRoot;
};

class OkayLevelManager : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayLevelManager> create(const OkayLevelManager& settings) {
        return std::make_unique<OkayLevelManager>(settings);
    };

    OkayLevelManager(const OkayLevelManagerSettings& settings) : _settings(settings) {}

    void setInitialLevel(OkayLevel level);

   private:
    OkayLevelManagerSettings _settings;
};

};  // namespace okay

#endif  // __OKAY_LEVEL_MANAGER_H__