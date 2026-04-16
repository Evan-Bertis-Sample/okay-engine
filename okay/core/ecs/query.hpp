#ifndef __QUERY_H__
#define __QUERY_H__

#include <okay/core/ecs/ecstore.hpp>
#include <okay/core/util/option.hpp>

#include <bitset>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace okay {

using ECSComponentMask = std::bitset<EntityComponentStore::MAX_COMPONENTS>;

enum class ECSQueryBitmaskType { Get, Exclude, Optional };

template <typename T>
concept TemplatedECSQueryish = requires(const EntityComponentStore& ecs) {
    { T::bitmask(ecs) } -> std::same_as<ECSComponentMask>;
    typename T::ComponentTypes;
    typename T::Specifier;
};

template <typename... Ts>
struct TypePack {};

template <ECSQueryBitmaskType Type, typename... Components>
struct TemplatedECSQuery {
    using ComponentTypes = std::tuple<Components...>;
    using ComponentPack = TypePack<Components...>;
    using Specifier = std::integral_constant<ECSQueryBitmaskType, Type>;

    static ECSComponentMask bitmask(const EntityComponentStore& store) {
        ECSComponentMask bitmask{};
        ((bitmask.set(store.template getComponentID<Components>().value())), ...);
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
        ECSEntity entity;
        std::tuple<GetComponents&...> components;
        std::tuple<Option<OptionalComponents&>...> optional;
    };
};

template <typename Get, typename Exclude = query::Exclude<>, typename Optional = query::Optional<>>
    requires(query::IsGetQuery<Get> && query::IsExcludeQuery<Exclude> &&
             query::IsOptionalQuery<Optional>)
struct ECSQuery {
    using Item = typename ECSQueryItemFromTuples<typename Get::ComponentTypes,
                                                 typename Optional::ComponentTypes>::Type;

    static void initialize(const EntityComponentStore& store) {
        _getBitmask = Get::bitmask(store);
        _excludeBitmask = Exclude::bitmask(store);
        _optionalBitmask = Optional::bitmask(store);
    }

    static bool matches(const ECSComponentMask& bitmask) {
        return (bitmask & _getBitmask) == _getBitmask && (bitmask & _excludeBitmask).none();
    }

    static Item createItem(ECSEntity entity, EntityComponentStore& store) {
        return createItemImpl(std::move(entity),
                              store,
                              typename Get::ComponentPack{},
                              typename Optional::ComponentPack{});
    }

   private:
    template <typename... GetComponents, typename... OptionalComponents>
    static Item createItemImpl(ECSEntity entity,
                               EntityComponentStore& store,
                               TypePack<GetComponents...>,
                               TypePack<OptionalComponents...>) {
        return Item{.entity = std::move(entity),
                    .components = std::forward_as_tuple(
                        store.template getComponent<GetComponents>(entity).value()...),
                    .optional = std::make_tuple(
                        store.template getComponent<OptionalComponents>(entity).value()...)};
    }

    inline static ECSComponentMask _getBitmask{};
    inline static ECSComponentMask _excludeBitmask{};
    inline static ECSComponentMask _optionalBitmask{};
};

}  // namespace okay

#endif  // __QUERY_H__