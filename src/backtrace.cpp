#include "backtrace.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunqualified-std-cast-call"
#endif

#define BACKWARD_HAS_BFD 1
#include "backward.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <cstdlib>

namespace manager::backtrace {

void print_trace() noexcept {
    // Load trace
    backward::StackTrace st;
    st.load_here(100);

    // Print trace
    backward::Printer p;
    p.print(st);

    std::abort();
}

}  // namespace manager::backtrace
