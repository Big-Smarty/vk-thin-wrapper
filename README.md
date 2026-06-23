# vk-thin-wrapper

`vk-thin-wrapper` is an experimental, single-header C++20 helper for building thin RAII wrappers around Vulkan handles.

The project lives in [`vk_thin_wrapper.hpp`](vk_thin_wrapper.hpp). It provides a small template API in the `bs::thin_wrappers` namespace for binding Vulkan create, get, and destroy functions to handle-owning wrapper types.

This is not production-ready. The wrapper is intentionally thin, has sharp lifetime edges, and should be treated as a cautionary experiment rather than a complete Vulkan abstraction.

## What It Provides

- Header-only C++20 wrapper code in `vk_thin_wrapper.hpp`.
- `bs::thin_wrappers::thin_wrapper`, a template RAII wrapper around C-style Vulkan handles.
- `no_parent_t` for handles that do not have a parent handle, such as `VkInstance`.
- `bs::thin_wrappers::vkTrampFn` for adapting Vulkan function pointers, including volk-loaded functions, into template arguments.
- Basic success/error logging through `spdlog`.

There are currently no build files, package files, or automated tests in this repository.

## Requirements

- A C++20-capable clang or gcc toolchain.
- Vulkan headers and loader setup appropriate for your platform.
- [`volk`](https://github.com/zeux/volk).
- [`spdlog`](https://github.com/gabime/spdlog).

## Usage

The intended usage is to create aliases for concrete Vulkan handle wrappers by binding the handle type, create-info type, parent type, create function, and destroy function:

```cpp
namespace thin_wrapper = bs::thin_wrappers;

using Instance = thin_wrapper::thin_wrapper<
    VkInstance,
    VkInstanceCreateInfo,
    no_parent_t,
    thin_wrapper::vkTrampFn<&vkCreateInstance, VkResult>,
    thin_wrapper::vkTrampFn<&vkDestroyInstance, void>>;
```

The wrapper stores the created Vulkan handle and, for child objects, a copy of the parent handle used for destruction. That makes parent-child lifetime ordering the caller's responsibility: parent handles must remain valid for every child wrapper that will destroy through them.

The template also supports getter-style objects, such as queues, through the `get_fn` path. Objects that are queried or enumerated can be wrapped directly from an existing handle, but the wrapper does not add ownership semantics beyond the behavior encoded by the template arguments.

## Caveats

- Vulkan object lifetime rules are not enforced beyond the wrapper's destructor call.
- Parent-child destruction order is not tracked.
- Allocation callbacks are currently passed as `nullptr`.
- Error handling is limited to logging `VkResult` values with `spdlog`; failed creation is not converted into exceptions.
- The API is experimental and may change.

Use this code only after reading the header and understanding the lifetime model.

## Contributing

Contributions, fixes, examples, and issue reports are welcome. If you have an idea for improving the wrapper, open an issue or submit a pull request.

## License

Distributed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

## Contact

- Discord: Big-Smarty#2123
- Email: budaniasco@gmail.com
