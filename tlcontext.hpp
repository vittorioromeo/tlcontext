// Copyright (c) 2023-2023 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

//
//
//
// Debug mode includes
// ----------------------------------------------------------------------------

// Define the macro below to enable assertions in the library, preventing UB in
// case of misuse, but adding run-time overhead.
#ifdef TLCONTEXT_DEBUG
#include <cstdlib>
#include <cstdio>
#endif

//
//
//
// Implementation details (private API)
// ----------------------------------------------------------------------------

namespace tlcontext::impl {

template <typename T>
constinit inline T* global_top_ptr{nullptr};

template <typename T>
constinit thread_local T* local_top_ptr{nullptr};

// Assertion functions, only available in debug mode.
#ifdef TLCONTEXT_DEBUG
inline void abort_if(bool predicate, const char* msg) noexcept
{
    if(!predicate) [[likely]]
    {
        return;
    }

    std::printf("TLCONTEXT FATAL ERROR: '%s'\n", msg);
    std::fflush(stdout);

    std::abort();
}
#endif

// RAII guard for `TType` contexts of type `T`.
template <typename T, bool TLocal>
class [[nodiscard]] guard
{
private:
    T _data;
    T* _prev;

public:
    template <typename... Ts>
    [[nodiscard, gnu::always_inline]] explicit guard(Ts&&... xs) noexcept(
        noexcept(T{static_cast<Ts&&>(xs)...}))
        : _data{static_cast<Ts&&>(xs)...}
    {
        T*& ptr_ref = (TLocal ? local_top_ptr<T> : global_top_ptr<T>);

        _prev = ptr_ref;
        ptr_ref = &_data;
    }

    [[gnu::always_inline]] ~guard() noexcept
    {
        (TLocal ? local_top_ptr<T> : global_top_ptr<T>) = _prev;
    }

    guard(const guard&) = delete;
    guard(guard&&) = delete;
};

} // namespace tlcontext::impl

//
//
//
// Public API
// ----------------------------------------------------------------------------

namespace tlcontext {

template <typename T>
struct helper
{
    helper() = delete;

    helper(const helper&) = delete;
    helper(helper&&) = delete;

    // A `local_guard` pushes a new context of type `T` on the thread-local
    // stack on construction, and pops it on destruction.
    using local_guard = impl::guard<T, true /* local */>;

    // A `global_guard` creates a new context of type `T` on the static buffer
    // on construction, and destroys it on destruction.
    using global_guard = impl::guard<T, false /* global */>;

    // Returns the context on top of the thread-local stack. The behavior is
    // undefined if there are no contexts of type `T` on the stack.
    [[nodiscard, gnu::always_inline]] inline static T& get_local() noexcept
    {
        T* const ptr = impl::local_top_ptr<T>;

#ifdef TLCONTEXT_DEBUG
        impl::abort_if(ptr == nullptr, "tried using inactive local context");
#endif

        return *ptr;
    }

    // Returns the global context. The behavior is undefined if there is no
    // global context of type `T`.
    [[nodiscard, gnu::always_inline]] inline static T& get_global() noexcept
    {
        T* const ptr = impl::global_top_ptr<T>;

#ifdef TLCONTEXT_DEBUG
        impl::abort_if(ptr == nullptr, "tried using inactive global context");
#endif

        return *ptr;
    }

    // Either returns the local context on top of the stack or the global
    // context, if there's no local context available. The behavior is undefined
    // if neither a local nor a global context is available.
    [[nodiscard, gnu::always_inline]] inline static T& get_top() noexcept
    {
        if(T* const local_ptr = impl::local_top_ptr<T>)
        {
            return *local_ptr;
        }

        T* const global_ptr = impl::global_top_ptr<T>;

#ifdef TLCONTEXT_DEBUG
        impl::abort_if(global_ptr == nullptr, "no available context");
#endif

        return *global_ptr;
    }
};

} // namespace tlcontext
