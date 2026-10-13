# Flawy – Coding Standards

These rules are binding for every contributor, human or agent.

## Language and tooling

- C++20. Use standard library facilities (`std::span`, `std::format`, `std::string_view`, concepts, `<ranges>`) where they clarify intent.
- The code must compile without warnings under MSVC `/W4 /permissive-`, and under GCC/Clang `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`. Warnings are errors.
- Formatting follows `.clang-format`.

## No comments

- Source code has **no comments**: no line comments, no block comments, no doc comments, no commented-out code, no TODOs.
- Code must explain itself through names, small functions and types. If something seems to need a comment, rename it or extract it instead.
- License headers of vendored code are not our concern. Only Flawy code is covered by this rule.

## Object-oriented design

- Model the engine as cohesive classes with a single responsibility. Free functions are allowed only inside anonymous namespaces in `.cpp` files, as private helpers.
- Use interfaces (abstract base classes with pure virtual methods and a virtual destructor) at module boundaries and where multiple implementations are expected, for example platform or backend abstractions.
- Prefer composition over inheritance. Keep inheritance hierarchies shallow.
- Encapsulate: data members are `private`. Expose behaviour, not state. Use getters only where they are genuinely needed.
- Apply the Rule of Zero by default. If a class owns a resource, follow the Rule of Five, or explicitly delete copy and move.
- Use RAII for every resource: SDL handles, bgfx handles, Jolt objects, AngelScript engines and audio devices are owned by wrapper classes, and those wrapper classes release them in their destructors.
- Do not let third-party types leak into public headers unless the module's purpose requires it. Use the pimpl idiom (`std::unique_ptr<Impl>`) to hide backend types.
- Use dependency injection through constructors, not hidden globals. Singletons are a last resort and must be justified in the plan.

## Naming

| Element | Convention | Example |
|---|---|---|
| Namespaces | PascalCase, rooted at `Flawy` | `Flawy::Renderer` |
| Classes, structs, enums, concepts | PascalCase | `RenderDevice`, `WindowSpecification` |
| Interfaces | PascalCase with an `I` prefix | `IWindow` |
| Methods and functions | PascalCase | `CreateWindow()`, `GetWidth()` |
| Local variables and parameters | Hungarian type prefix + PascalCase | `dDeltaSeconds`, `oSpecification` |
| Data members (including public struct fields) | Hungarian type prefix + PascalCase | `sName`, `uWidth` |
| Private data members | `m_` + Hungarian type prefix + PascalCase | `m_bRunning`, `m_oSpecification` |
| Static data members | `s_` + Hungarian type prefix + PascalCase | `s_spLogger` |
| Global variables (avoid) | `g_` + Hungarian type prefix + PascalCase | `g_uFrameIndex` |
| Constants and `constexpr` | `k` + PascalCase | `kMaxLights` |
| Enum values | PascalCase (always `enum class`) | `TextureFormat::Rgba8` |
| Files | PascalCase, one primary class per file | `RenderDevice.hpp` / `RenderDevice.cpp` |

### Hungarian notation

Every variable, parameter and data member uses a type prefix. The scope prefix (`m_`, `s_`, `g_`) comes first, then the type prefix, then the PascalCase name. For `auto` variables, the prefix follows the deduced type. References and `const` do not change the prefix; it describes the referenced type.

| Prefix | Type | Example |
|---|---|---|
| `b` | `bool` | `bRunning` |
| `i` | signed integers (`int`, `int32_t`, `int64_t`, ...) | `iVersion` |
| `u` | unsigned integers (`unsigned`, `uint32_t`, `size_t`, ...) | `uIndex` |
| `f` | `float` | `fSpeed` |
| `d` | `double` | `dDeltaSeconds` |
| `c` | `char` | `cSeparator` |
| `s` | `std::string` | `sName` |
| `sv` | `std::string_view` | `svName` |
| `sz` | null-terminated C string (`const char*`) | `szTitle` |
| `e` | `enum` / `enum class` value | `eFormat` |
| `h` | opaque API handle (bgfx, SDL, Jolt body ids, ...) | `hProgram` |
| `p` | raw non-owning pointer | `pWindow` |
| `up` | `std::unique_ptr` | `upRenderer` |
| `sp` | `std::shared_ptr` | `spLogger` |
| `wp` | `std::weak_ptr` | `wpOwner` |
| `opt` | `std::optional` | `optPath` |
| `v` | `std::vector` | `vEntities` |
| `a` | `std::array` / C array | `aCorners` |
| `span` | `std::span` | `spanVertices` |
| `map` | `std::map` / `std::unordered_map` | `mapTextures` |
| `set` | `std::set` / `std::unordered_set` | `setVisited` |
| `fn` | callables (`std::function`, lambdas, function pointers) | `fnCallback` |
| `t` | `std::chrono` time points and durations | `tFrameStart` |
| `vec`, `mat`, `quat` | GLM vectors, matrices and quaternions | `vecPosition`, `matView` |
| `o` | any other class or struct instance | `oSpecification`, `oLogger` |

## Files and structure

- Public headers go in `Flawy/include/Flawy/<Module>/`, and implementations in `Flawy/src/<Module>/`. Private headers go in `Flawy/src/<Module>/` only.
- Use `#pragma once` in every header.
- Include order: the matching header first, then Flawy headers, then third-party headers, then standard headers. Separate the groups with blank lines.
- Prefer forward declarations in headers. Never put `using namespace` in a header.
- Put namespace braces on their own lines and indent the namespace contents (see the existing code).

## Correctness and safety

- Mark single-argument constructors `explicit`, and mark non-throwing functions `noexcept`.
- Mark value-returning queries `[[nodiscard]]`.
- Use `const` wherever possible, and `constexpr` where possible.
- Do not use raw owning pointers or manual `new`/`delete`. Use `std::unique_ptr` by default and `std::shared_ptr` only for genuinely shared ownership.
- Do not use C-style casts. Use `static_cast` and similar, and make narrowing explicit.
- Handle errors with exceptions for unrecoverable setup failures and return values (`std::optional`, `std::expected`-like types) for expected failures. Log through `Flawy::Core::Logger`.
- Do not make platform `#ifdef`s in general code. Isolate platform specifics behind interfaces.

## Performance awareness

- Hot paths (per-frame and per-entity code) avoid heap allocations, virtual calls in tight loops and needless copies.
- Data that is iterated per frame lives in contiguous storage (ECS components, `std::vector`).
