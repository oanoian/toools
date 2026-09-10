/**
 * Alpha1 Framework - Core Threading Implementation
 * Note: Most threading primitives are header-only templates
 * This file contains explicit template instantiations if needed
 */

#include "alpha1/core/threading.h"

namespace Alpha1::Core {

// Explicit template instantiations for common types
template class ConcurrentQueue<int>;
template class ConcurrentQueue<void*>;
template class ConcurrentQueue<std::function<void()>>;

} // namespace Alpha1::Core
