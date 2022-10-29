#pragma once

#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <type_traits>
#include <volk.h>

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

/**
 * The actual thin wrapper. RAII wrapper around the C-style vulkan objects.
 * @tparam T typename of object to be wrapped
 * @tparam CreateInfo VkCreateInfo of object to be wrapped
 * @tparam ParentType parent type of object to be wrapped; use no_parent_t if no
 *parent is present
 * @arg create_info VkCreateInfo struct of object to be created
 **/
template <typename T, typename CreateInfo, typename ParentType ,
          create_fn<T, CreateInfo, ParentType> CreateFn,
          destroy_fn<T, ParentType> DestroyFn>
class thin_wrapper {
public:
  /**
   * Constructor for the thin wrapper object if no parent is present
   * @arg create_info VkCreateInfoStruct for object to be created
   **/
  thin_wrapper(const CreateInfo &create_info)
    requires std::is_same_v<ParentType, no_parent_t>
  {
    try {
      auto result = CreateFn(&create_info, nullptr, &m_object);
      if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string("There was an error during VK object initialization! "
                        "Return Code: ") +
            std::to_string(result) + std::string(" in file ") + __FILE__ +
            std::string(" on line ") + std::to_string(__LINE__));
      }
    } catch (std::exception &_exception) {
      spdlog::error("There was an exception: {0}", _exception.what());
      exit(-1);
    } catch (std::runtime_error &_runtime_error) {
      spdlog::error("There was a runtime error: {0}", _runtime_error.what());
      exit(-1);
    }
  }

  /**
   * Constructor for the thin wrapper object if a parent is present
   * @arg create_info VkCreateInfoStruct for object to be created
   * @arg parent parent of the object; The handle will be copied for later
   *destruction
   **/
  thin_wrapper(const CreateInfo &create_info, const ParentType &parent)
    requires(!std::is_same_v<ParentType, no_parent_t>)
      : m_parent(parent) {
    try {
      auto result = CreateFn(parent, &create_info, nullptr, &m_object);
      if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string("There was an error during VK object initialization! "
                        "Return Code: ") +
            std::to_string(result) + std::string(" in file ") + __FILE__ +
            std::string(" on line ") + std::to_string(__LINE__));
      }
    } catch (std::exception &_exception) {
      spdlog::error("There was an exception: {0}", _exception.what());
      exit(-1);
    } catch (std::runtime_error &_runtime_error) {
      spdlog::error("There was a runtime error: {0}", _runtime_error.what());
      exit(-1);
    }
  }
  thin_wrapper(const thin_wrapper &) = delete;
  thin_wrapper(thin_wrapper &&other) noexcept
    requires(std::is_same_v<ParentType, no_parent_t>)
      : m_object(std::exchange(other.m_object, nullptr)) {}
  thin_wrapper(thin_wrapper &&other) noexcept
    requires(!std::is_same_v<ParentType, no_parent_t>)
      : m_object(std::exchange(other.m_object, nullptr)),
        m_parent(std::exchange(other.m_parent, nullptr)) {}
  thin_wrapper &operator=(const thin_wrapper &) = delete;
  thin_wrapper &operator=(thin_wrapper &&) = delete;
  ~thin_wrapper()
    requires(std::is_same_v<ParentType, no_parent_t> ||
             std::is_same_v<T, VkDevice>)
  {
    DestroyFn(m_object, nullptr);
  }
  ~thin_wrapper()
    requires(!(std::is_same_v<ParentType, no_parent_t> ||
               std::is_same_v<T, VkDevice>))
  {
    DestroyFn(m_parent, m_object, nullptr);
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
