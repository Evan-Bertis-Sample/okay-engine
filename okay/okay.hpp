#ifndef __OKAY_H__
#define __OKAY_H__

// This file is auto-generated. Do not edit manually.

// okay/core/asset
#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>

// okay/core/asset/generic
#include <okay/core/asset/generic/font_loader.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>

// okay/core/asset/mesh
#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/asset/mesh/obj_loader.hpp>

// okay/core/ecs
#include <okay/core/ecs/builtins.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/ecs/ecs_util.hpp>
#include <okay/core/ecs/ecstore.hpp>
#include <okay/core/ecs/query.hpp>

// okay/core/ecs/components
#include <okay/core/ecs/components/camera_component.hpp>
#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/render_component.hpp>
#include <okay/core/ecs/components/text_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/components/ui_component.hpp>

// okay/core/ecs/systems
#include <okay/core/ecs/systems/camera_system.hpp>
#include <okay/core/ecs/systems/light_system.hpp>
#include <okay/core/ecs/systems/renderer_system.hpp>
#include <okay/core/ecs/systems/ui_system.hpp>

// okay/core/engine
#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/event.hpp>
#include <okay/core/engine/logger.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/engine/time.hpp>

// okay/core/renderer
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/gpu.hpp>
#include <okay/core/renderer/imgui_impl.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/math_types.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/primitive.hpp>
#include <okay/core/renderer/render_pipeline.hpp>
#include <okay/core/renderer/render_target.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/renderer/renderer_util.hpp>
#include <okay/core/renderer/shader.hpp>
#include <okay/core/renderer/surface.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>

// okay/core/renderer/materials
#include <okay/core/renderer/materials/lit.hpp>
#include <okay/core/renderer/materials/ui_rect.hpp>
#include <okay/core/renderer/materials/unlit.hpp>

// okay/core/renderer/passes
#include <okay/core/renderer/passes/scene_pass.hpp>

// okay/core/tween
#include <okay/core/tween/i_okay_tween.hpp>
#include <okay/core/tween/tween.hpp>
#include <okay/core/tween/tween_collection.hpp>
#include <okay/core/tween/tween_easing.hpp>
#include <okay/core/tween/tween_engine.hpp>
#include <okay/core/tween/tween_sequence.hpp>

// okay/core/ui
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/render_resources.hpp>
#include <okay/core/ui/text_layout.hpp>
#include <okay/core/ui/text_mesh_builder.hpp>
#include <okay/core/ui/ui.hpp>

// okay/core/util
#include <okay/core/util/dirty_set.hpp>
#include <okay/core/util/format.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/property.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/util/singleton.hpp>
#include <okay/core/util/string.hpp>
#include <okay/core/util/type.hpp>
#include <okay/core/util/variant.hpp>

#endif  // __OKAY_H__
