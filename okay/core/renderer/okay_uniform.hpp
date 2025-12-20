#ifndef __OKAY_UNIFORM_H__
#define __OKAY_UNIFORM_H__

#include <glm/glm.hpp>  // you use glm types
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <okay/core/util/result.hpp>

// forward declare / include OkayTexture
// #include <okay/core/renderer/okay_texture.hpp>

namespace okay {

template <std::size_t N>
struct FixedString {
    char value[N]{};

    constexpr FixedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = str[i];
    }

    constexpr std::string_view sv() const { return {value, N - 1}; }  // drop '\0'
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

enum class UniformKind { Float, Int, Vec2, Vec3, Vec4, Mat3, Mat4, Texture };


template <class T>
constexpr UniformKind UniformKindOf() {
    if constexpr (std::is_same_v<T, float>)
        return UniformKind::Float;
    else if constexpr (std::is_same_v<T, int>)
        return UniformKind::Int;
    else if constexpr (std::is_same_v<T, glm::vec2>)
        return UniformKind::Vec2;
    else if constexpr (std::is_same_v<T, glm::vec3>)
        return UniformKind::Vec3;
    else if constexpr (std::is_same_v<T, glm::vec4>)
        return UniformKind::Vec4;
    else if constexpr (std::is_same_v<T, glm::mat3>)
        return UniformKind::Mat3;
    else if constexpr (std::is_same_v<T, glm::mat4>)
        return UniformKind::Mat4;
    else if constexpr (std::is_same_v<T, OkayTexture>)
        return UniformKind::Texture;
    else
        static_assert(dependent_false_v<T>, "Unsupported uniform type");
}

// Name is an NTTP value, typically FixedString<N>
template <class T, auto Name>
struct OkayMaterialUniform {
    using ValueType = T;

    static constexpr auto name = Name;
    static constexpr UniformKind kind = UniformKindOf<T>();
    static constexpr unsigned int invalidLocation() { return 0xFFFFFFFFu; }

    constexpr std::string_view name_sv() const { return name.sv(); }

    void set(const T& v) {
        if (_value != v) _dirty = true;
        _value = v;
    }
    const T& get() const { return _value; }
    bool isDirty() const { return _dirty; }

   private:
    T _value{};
    bool _dirty{true};
    unsigned int _location{invalidLocation()};
};

// Name matching
template <auto Name, class U>
struct UniformNameMatches : std::false_type {};

template <auto Name, class T, auto UName>
struct UniformNameMatches<Name, OkayMaterialUniform<T, UName>>
    : std::bool_constant<(Name.sv() == UName.sv())> {};

// Index of uniform by name
template <auto Name, class... Uniforms>
struct UniformIndex;

template <auto Name, class First, class... Rest>
struct UniformIndex<Name, First, Rest...> {
    static constexpr std::size_t value =
        UniformNameMatches<Name, First>::value ? 0 : 1 + UniformIndex<Name, Rest...>::value;
};

template <auto Name>
struct UniformIndex<Name> {
    static_assert(dependent_false_v<decltype(Name)>, "Uniform name not found in collection.");
    static constexpr std::size_t value = 0;
};

template <class... Uniforms>
struct OkayMaterialUniformCollection {
    std::tuple<Uniforms...> uniforms;

    template <auto Name>
    decltype(auto) get() {
        constexpr std::size_t idx = UniformIndex<Name, Uniforms...>::value;
        return std::get<idx>(uniforms);
    }

    template <auto Name>
    decltype(auto) get() const {
        constexpr std::size_t idx = UniformIndex<Name, Uniforms...>::value;
        return std::get<idx>(uniforms);
    }

    void setUniforms() {
        // Iterate over all uniforms and set them if dirty
        (setUniformIfDirty(std::get<Uniforms>(uniforms)), ...);
    }

    private:
    template <class U>
    void setUniformIfDirty(U& uniform) {
        if (!uniform.isDirty()) return;

        // actually set the uniform in OpenGL here


        uniform.clearDirty();
    }
};

}  // namespace okay

#endif  // __OKAY_UNIFORM_H__
