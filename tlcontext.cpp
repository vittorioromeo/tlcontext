// Copyright (c) 2023-2023 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#define TLCONTEXT_DEBUG 1
#include "tlcontext.hpp"

// Set up assertions.
// #define NDEBUG 1
#include <cassert>

// Set up context data classes.
struct int_ctx_data
{
    int value;

    explicit int_ctx_data(int x) : value(x)
    {}
};

// Set up context helper type aliases.
using int_ctx = tlcontext::helper<int_ctx_data>;

static void f0();
static void f1();
static void f2();
static void client();
static void fpa0();
static void fpa1();

int main()
{
    int_ctx::global_guard gg(1);
    assert(int_ctx::get_global().value == 1);
    assert(int_ctx::get_top().value == 1);

    {
        int_ctx::global_guard gg(5);

        assert(int_ctx::get_global().value == 5);
        assert(int_ctx::get_top().value == 5);

        f0();

        {
            int_ctx::local_guard lg(10);
            assert(int_ctx::get_local().value == 10);
            assert(int_ctx::get_top().value == 10);

            f1();

            assert(int_ctx::get_local().value == 10);
            assert(int_ctx::get_top().value == 10);
        }

        assert(int_ctx::get_global().value == 5);
        assert(int_ctx::get_top().value == 5);
    }

    assert(int_ctx::get_global().value == 1);
    assert(int_ctx::get_top().value == 1);

    client();

    fpa0();
}

void f0()
{
    assert(int_ctx::get_global().value == 5);
    assert(int_ctx::get_top().value == 5);
}

void f1()
{
    assert(int_ctx::get_local().value == 10);
    assert(int_ctx::get_top().value == 10);

    {
        int_ctx::local_guard lg(15);
        assert(int_ctx::get_local().value == 15);
        assert(int_ctx::get_top().value == 15);

        f2();

        assert(int_ctx::get_local().value == 15);
        assert(int_ctx::get_top().value == 15);
    }

    assert(int_ctx::get_local().value == 10);
    assert(int_ctx::get_top().value == 10);
}

void f2()
{
    assert(int_ctx::get_local().value == 15);
    assert(int_ctx::get_top().value == 15);
}

//
//
//
// Metric collection example
// ----------------------------------------------------------------------------

#include <chrono>
#include <iostream>
#include <string_view>

using clock_type = std::chrono::steady_clock;

struct metrics_ctx_data
{
    std::string_view label;
    clock_type::time_point tp;
};

using metrics_ctx = tlcontext::helper<metrics_ctx_data>;

class metrics_guard
{
private:
    inline static int depth = 0;
    metrics_ctx::local_guard guard;

public:
    [[nodiscard]] explicit metrics_guard(std::string_view label)
        : guard(label, clock_type::now())
    {
        ++depth;
    }

    ~metrics_guard()
    {
        const metrics_ctx_data& top = metrics_ctx::get_top();
        const auto elapsed = clock_type::now() - top.tp;

        const auto us =
            std::chrono::duration_cast<std::chrono::microseconds>(elapsed)
                .count();

        std::cout << std::string(depth * 4, '-') << " " << top.label << " took "
                  << us << "us\n";

        --depth;
    }
};

struct simulator
{
    void step0()
    {
        // TODO
    }

    void step1a()
    {}

    void step1b()
    {}

    void step1()
    {
        {
            metrics_guard mg{"step1a"};
            step1a();
        }

        {
            metrics_guard mg{"step1b"};
            step1b();
        }
    }

    void step2()
    {
        // TODO
    }
};

void client()
{
    simulator s;

    metrics_guard mg{"client"};

    {
        metrics_guard mg{"step0"};
        s.step0();
    }

    {
        metrics_guard mg{"step1"};
        s.step1();
    }

    {
        metrics_guard mg{"step2"};
        s.step2();
    }
}

//
//
//
// Polymorphic allocator example
// ----------------------------------------------------------------------------

#include <memory_resource>
#include <vector>
#include <iostream>
#include <cstddef>

struct pmr_context_data
{
    std::pmr::memory_resource* _mr;
};

using pmr_context = tlcontext::helper<pmr_context_data>;

void fpa1()
{
    std::pmr::memory_resource* mr = pmr_context::get_top()._mr;
    std::cout << "using memory resource " << static_cast<void*>(mr) << '\n';

    std::pmr::vector<int> v({0, 1, 2, 3, 4, 5}, mr);

    for(int x : v)
    {
        std::cout << x;
    }

    std::cout << '\n';
}

void fpa0()
{
    pmr_context::global_guard gg{std::pmr::new_delete_resource()};
    fpa1();

    {
        alignas(int) std::byte buffer[512];
        std::pmr::monotonic_buffer_resource mbr{buffer, 512};

        pmr_context::local_guard lg{&mbr};
        fpa1();
    }
}
