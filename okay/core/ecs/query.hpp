#ifndef __QUERY_H__
#define __QUERY_H__

#include <okay/core/ecs/ecs.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace okay {

using ECSBitmask = std::bitset<ECS::MAX_COMPONENTS>;

enum class ECSQueryBitmaskType { Get, Exclude, Optional };

template <typename T>
concept TemplatedECSQueryish = requires(const ECS& ecs) {
    { T::bitmask(ecs) } -> std::same_as<ECSBitmask>;
    typename T::ComponentTypes;
    typename T::Specifier;
};

template <ECSQueryBitmaskType Type, typename... Components>
struct TemplatedECSQuery {
    using ComponentTypes = std::tuple<Components...>;
    using Specifier = std::integral_constant<ECSQueryBitmaskType, Type>;

    static ECSBitmask bitmask(const ECS& ecs) {
        ECSBitmask bitmask = 0;
        ((bitmask |= (ECSBitmask(1) << ecs.getComponentID<Components>().value())), ...);
        return bitmask;
    }
};

namespace query {

template <typename... Components>
using Get = TemplatedECSQuery<ECSQueryBitmaskType::Get, Components...>;

template <typename... Components>
using Exclude = TemplatedECSQuery<ECSQueryBitmaskType::Exclude, Components...>;

template <typename... Components>
using Optional = TemplatedECSQuery<ECSQueryBitmaskType::Optional, Components...>;

template <typename T>
concept IsGetQuery = TemplatedECSQueryish<T> && T::Specifier::value == ECSQueryBitmaskType::Get;

template <typename T>
concept IsExcludeQuery =
    TemplatedECSQueryish<T> && T::Specifier::value == ECSQueryBitmaskType::Exclude;

template <typename T>
concept IsOptionalQuery =
    TemplatedECSQueryish<T> && T::Specifier::value == ECSQueryBitmaskType::Optional;

}  // namespace query

template <typename GetTuple, typename OptionalTuple>
struct ECSQueryItemFromTuples;

template <typename... GetComponents, typename... OptionalComponents>
struct ECSQueryItemFromTuples<std::tuple<GetComponents...>, std::tuple<OptionalComponents...>> {
    struct Type {
        ECSEntity& entity;
        std::tuple<std::reference_wrapper<GetComponents>...> getComponents;
        std::tuple<std::optional<std::reference_wrapper<OptionalComponents>>...> optionalComponents;
    };
};

template <typename Get, typename Exclude = query::Exclude<>, typename Optional = query::Optional<>>
    requires(query::IsGetQuery<Get> && query::IsExcludeQuery<Exclude> &&
             query::IsOptionalQuery<Optional>)
struct ECSQuery {
    using Item = typename ECSQueryItemFromTuples<typename Get::ComponentTypes,
                                                 typename Optional::ComponentTypes>::Type;

    static void initialize(const ECS& ecs) {
        if (_initialized) {
            return;
        }

        _getBitmask = Get::bitmask(ecs);
        _excludeBitmask = Exclude::bitmask(ecs);
        _optionalBitmask = Optional::bitmask(ecs);
        _initialized = true;
    }

    static constexpr ECSBitmask matches(const ECSBitmask& bitmask) {
        if (!_initialized) {
            throw std::runtime_error("ECSQuery not initialized");
        }

        return (_getBitmask & ~_excludeBitmask) == (bitmask & ~_optionalBitmask);
    }

   private:
    static ECSBitmask _getBitmask;
    static ECSBitmask _excludeBitmask;
    static ECSBitmask _optionalBitmask;
    static bool _initialized;
};

}  // namespace okay

#endif  // __QUERY_H__