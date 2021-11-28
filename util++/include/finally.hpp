#pragma once

#include <utility>
#include <type_traits>

template <typename Function>
class FinalAction {
public:
    static_assert(
        !std::is_reference<Function>::value &&
            !std::is_const<Function>::value &&
            !std::is_volatile<Function>::value,
        "FinalAction should store its function by value"
    );

    using StrippedFunction = typename std::remove_cv<
        typename std::remove_reference<Function>::type>::type;

    explicit FinalAction(Function function) noexcept :
        m_function(std::move(function))
    {
    }

    FinalAction(FinalAction &&other) noexcept :
        m_function(std::move(other.m_function)),
        m_invoke(std::exchange(other.m_invoke, false))
    {
    }

    FinalAction(const FinalAction &) = delete;
    FinalAction &operator=(const FinalAction &) = delete;
    FinalAction &operator=(FinalAction &&) = delete;

    ~FinalAction() noexcept
    {
        if (m_invoke) {
            m_function();
        }
    }

private:
    Function m_function;
    bool m_invoke = true;
};

template <typename Function>
FinalAction<typename FinalAction<Function>::StrippedFunction> finally(
    Function &&function
) noexcept
{
    using StrippedFunction = typename FinalAction<Function>::StrippedFunction;

    return FinalAction<StrippedFunction>(std::forward<Function>(function));
}
