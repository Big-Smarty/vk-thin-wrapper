#pragma once

#include <functional>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <type_traits>

#include <volk.h>

struct no_parent_t {};

VkResult vkCreateSurfaceDummy(VkInstance, const no_parent_t *,
                              const VkAllocationCallbacks *, VkSurfaceKHR *);
namespace bs::thin_wrappers {
template <auto *FnPtr, typename T> T vkTrampFn(auto... args) {
  return (*FnPtr)(args...);
}
template <typename T, typename CreateInfo, typename ParentType>
using create_fn =
    std::conditional_t<std::is_same_v<ParentType, no_parent_t>,
                       VkResult (*)(const CreateInfo *,
                                    const VkAllocationCallbacks *, T *),
                       VkResult (*)(ParentType, const CreateInfo *,
                                    const VkAllocationCallbacks *, T *)>;
template <typename T, typename ParentType>
using destroy_fn =
    std::conditional_t<std::is_same_v<T, VkInstance> ||
                           std::is_same_v<T, VkDevice>,
                       void (*)(T, const VkAllocationCallbacks *),
                       void (*)(ParentType, T, const VkAllocationCallbacks *)>;
template <typename T, typename CreateInfo, typename ParentType,
          create_fn<T, CreateInfo, ParentType> CreateFn,
          destroy_fn<T, ParentType> DestroyFn>
class thin_wrapper {
public:
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
    requires std::is_same_v<ParentType, no_parent_t>
  {
    DestroyFn(m_object, nullptr);
  }
  ~thin_wrapper()
    requires(!std::is_same_v<ParentType, no_parent_t>)
  {
    DestroyFn(m_parent, m_object, nullptr);
  }

  T object() { return m_object; }
  ParentType parent() { return m_parent; }

protected:
  T m_object{nullptr};
  [[no_unique_address]] ParentType m_parent{nullptr};
};
} // namespace bs::thin_wrappers
