#pragma once

#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <type_traits>
#include <volk.h>

#define VK_CHECK(x)                                                            \
  auto result = x;                                                             \
  if (result == VK_SUCCESS) {                                                  \
    spdlog::trace("{0} returned {1}", #x, result);                             \
  } else {                                                                     \
    spdlog::error("{0} returned {1}", #x, result);                             \
  }

#define VK_CHECK_TWC(x, o)                                                     \
  auto result = x;                                                             \
  if (result == VK_SUCCESS) {                                                  \
    spdlog::trace("{0}\n\
      object type: {1}\n\
      return code: {2}",                                                       \
                  #x, o, result);                                              \
  } else {                                                                     \
    spdlog::error("{0}\n\
      object type: {1}\n\
      return code: {2}",                                                       \
                  #x, o, result);                                              \
  }

/**
 * Dummy struct for when no parent is present (mostly for the case of
 * VkInstance)
 * */
struct no_parent_t {};

/**
 * Dummy function for creating a surface.
 * Allows wrapping VkSurfaceKHR.
 * It would probably be best to just put nullptr into each and every parameter;
 * they are just present to ensure compatibility with the thin wrapper.
 * */
VkResult vkCreateSurfaceDummy(VkInstance, const no_parent_t *,
                              const VkAllocationCallbacks *, VkSurfaceKHR *);
namespace bs::thin_wrappers {

/**
 * Trampoline function for use with volk.
 * @arg args Parameter pack for underlying vulkan function
 * @return VkResult returned by the underlying vulkan function
 * */
template <auto *FnPtr, typename T> T vkTrampFn(auto... args) {
  return (*FnPtr)(args...);
}

/**
 * Wrapper function for object creation.
 * @tparam T typename of object to be created
 * @tparam CreateInfo VkCreateInfo struct of object to be created
 * @tparam ParentType parent type of object to be created; use no_parent_t if no
 *parent is present
 **/
template <typename T, typename CreateInfo, typename ParentType>
using create_fn =
    std::conditional_t<std::is_same_v<ParentType, no_parent_t>,
                       VkResult (*)(const CreateInfo *,
                                    const VkAllocationCallbacks *, T *),
                       VkResult (*)(ParentType, const CreateInfo *,
                                    const VkAllocationCallbacks *, T *)>;

/**
 * Wrapper function for object destruction.
 * @tparam T typename of object to be destroyed
 * @tparam ParentType parent type of object to be destroyed; use no_parent_t if
 *no parent is present
 **/
template <typename T, typename ParentType>
using destroy_fn =
    std::conditional_t<std::is_same_v<T, VkInstance> ||
                           std::is_same_v<T, VkDevice>,
                       void (*)(T, const VkAllocationCallbacks *),
                       void (*)(ParentType, T, const VkAllocationCallbacks *)>;

template <typename T, typename CreateInfo, typename ParentType>
using get_fn = void (*)(ParentType, CreateInfo *, T *);

/**
 * The actual thin wrapper. RAII wrapper around the C-style vulkan objects.
 * @tparam T typename of object to be wrapped
 * @tparam CreateInfo VkCreateInfo of object to be wrapped
 * @tparam ParentType parent type of object to be wrapped; use no_parent_t if no
 *parent is present
 * @arg create_info VkCreateInfo struct of object to be created
 **/
template <typename T, typename CreateInfo, typename ParentType,
          std::conditional_t<!std::is_same_v<T, VkQueue>,
                             create_fn<T, CreateInfo, ParentType>,
                             get_fn<T, CreateInfo, ParentType>>
              CreateFn,
          destroy_fn<T, ParentType> DestroyFn>
class thin_wrapper {
public:
  /**
   * Constructor for the thin wrapper object if no parent is present
   * @arg create_info VkCreateInfoStruct for object to be created
   **/
  thin_wrapper(CreateInfo &create_info)
    requires(std::is_same_v<ParentType, no_parent_t> &&
             std::is_same_v<create_fn<T, CreateInfo, ParentType>,
                            decltype(CreateFn)>)
  {
    VK_CHECK_TWC(CreateFn(&create_info, nullptr, &m_object), typeid(T).name());
  }

  /**
   * Constructor for the thin wrapper object if a parent is present
   * @arg create_info VkCreateInfo struct for object to be created
   * @arg parent parent of the object; The handle will be copied for later
   *destruction
   **/
  thin_wrapper(CreateInfo &create_info, ParentType &parent)
    requires(!std::is_same_v<ParentType, no_parent_t> &&
             std::is_same_v<create_fn<T, CreateInfo, ParentType>,
                            decltype(CreateFn)>)
      : m_parent(parent) {
    VK_CHECK_TWC(CreateFn(parent, &create_info, nullptr, &m_object),
                 typeid(T).name());
  }

  /**
   * Constructor for the thin wrapper object if the object is "created" via a
   *getter function (like vkGetDeviceQueue2)
   * @arg create_info VkCreateInfo struct for object to be created
   * @arg parent parent of the object; The handle will be copied for later
   *destruction
   **/
  thin_wrapper(CreateInfo &create_info, ParentType &parent)
    requires(
        std::is_same_v<get_fn<T, CreateInfo, ParentType>, decltype(CreateFn)>)
      : m_parent(parent) {
    CreateFn(parent, &create_info, &m_object);
  }

  /**
   * Constructor for the thin wrapper object if its a standalone thats queried
   *or enumerated (e.g. VkPhysicalDevice)
   **/
  thin_wrapper(T &object) : m_object(object) {}

  thin_wrapper(const thin_wrapper &) = delete;
  thin_wrapper(thin_wrapper &&other) noexcept = delete;
  thin_wrapper &operator=(const thin_wrapper &) = delete;
  thin_wrapper &operator=(thin_wrapper &&) = delete;

  /**
   * Default thin wrapper in case DestroyFn is nullptr (for cases where no
   *explicit destruction is needed)
   **/
  ~thin_wrapper() = default;

  /**
   * Destructor for objects with VkDevice as a parent
   **/
  ~thin_wrapper()
    requires(std::is_same_v<ParentType, VkDevice>)
  {
    DestroyFn(m_parent, m_object, nullptr);
  }

  /**
   * Destructor for objects where VkDevice is not the parent (e.g. VkInstance,
   *VkDevice)
   **/
  ~thin_wrapper()
    requires(!std::is_same_v<ParentType, VkDevice>)
  {
    DestroyFn(m_object, nullptr);
  }

  /**
   * Returns the wrapped C-style object
   **/
  T object() { return m_object; }

  /**
   * returns the wrapped C-style parent
   **/
  ParentType parent() { return m_parent; }

protected:
  T m_object{nullptr};
  [[no_unique_address]] ParentType m_parent;
};
} // namespace bs::thin_wrappers
