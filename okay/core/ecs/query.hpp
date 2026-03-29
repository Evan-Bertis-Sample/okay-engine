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

using ECSQueryBitmask = std::uint32_t;

enum class ECSQueryBitmaskType { Get, Exclude, Optional };

template <typename T>
concept ECSQueryType = requires(const ECS& ecs) {
    { T::bitmask(ecs) } -> std::same_as<ECSQueryBitmask>;
    typename T::ComponentTypes;
    typename T::Specifier;
};

template <ECSQueryBitmaskType Type, typename... Components>
struct SimpleECSQuery {
    using ComponentTypes = std::tuple<Components...>;
    using Specifier = std::integral_constant<ECSQueryBitmaskType, Type>;

    static ECSQueryBitmask bitmask(const ECS& ecs) {
        ECSQueryBitmask bitmask = 0;
        ((bitmask |= (ECSQueryBitmask(1) << ecs.getComponentID<Components>().value())), ...);
        return bitmask;
    }
};

namespace query {

template <typename... Components>
using Get = SimpleECSQuery<ECSQueryBitmaskType::Get, Components...>;

template <typename... Components>
using Exclude = SimpleECSQuery<ECSQueryBitmaskType::Exclude, Components...>;

template <typename... Components>
using Optional = SimpleECSQuery<ECSQueryBitmaskType::Optional, Components...>;

template <typename T>
concept IsGetQuery = ECSQueryType<T> && T::Specifier::value == ECSQueryBitmaskType::Get;

template <typename T>
concept IsExcludeQuery = ECSQueryType<T> && T::Specifier::value == ECSQueryBitmaskType::Exclude;

template <typename T>
concept IsOptionalQuery = ECSQueryType<T> && T::Specifier::value == ECSQueryBitmaskType::Optional;

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

   private:
    constexpr ECSQueryBitmask& getBitmask(ECSQueryBitmaskType type) {
        switch (type) {
            case ECSQueryBitmaskType::Get:
                return _getBitmask;
            case ECSQueryBitmaskType::Exclude:
                return _excludeBitmask;
            case ECSQueryBitmaskType::Optional:
                return _optionalBitmask;
        }

        throw std::runtime_error("Invalid ECSQueryBitmaskType");
    }

    ECSQueryBitmask _getBitmask{0};
    ECSQueryBitmask _excludeBitmask{0};
    ECSQueryBitmask _optionalBitmask{0};
};

}  // namespace okay

#endif  // __QUERY_H__