#include <type_traits>

namespace yb::task {

template <class T>
struct NewPlacementConstructor
{
  template <class ...Args>
  static void Construct(void* ptr, Args&&... args)
  {
    new (ptr)(typename std::remove_reference<T>::type)(std::forward<Args>(args)...);
  }

  static T* GetPtr(void* ptr)
  {
    return static_cast<T*>(ptr);
  }
};

}