/*
 * Kerem Güre
 * S015664
 * Department of Computer Science
 */
#include <iostream>
#include <vector>
#include <utility>

/* Tiny Ranges Implementation by Furkan KIRAC
 * as part of CS409/509 - Advanced C++ Programming course in Ozyegin University
 * Supports transforming and filtering ranges and,
 * to<CONTAINER>() method for eagerly rendering the range into a CONTAINER.
 * Date: 20191222
 */

// predicates
namespace predicates {
    auto less_than = [](auto threshold) { return [=](auto value) { return value < threshold; }; };
    auto greater_than = [](auto threshold) { return [=](auto value) { return value > threshold; }; };
    auto all_of = [](auto ... preds) { return [=](auto value) { return (preds(value) && ...); }; };
}

namespace actions {
    auto multiply_by = [](auto coef) { return [=](auto value) { return value * coef; }; };
    auto if_then = [](auto predicate, auto action) {
        return [=](auto value) {
            if (predicate(value))
                value = action(value);
            return value;
        };
    };
}

namespace views {
    auto ints = [](int k = 0) { return [k]() mutable { return k++; }; };
    auto odds = []() {
        return [k = 1]() mutable {
            auto r = k;
            k += 2;
            return r;
        };
    };
}


namespace algorithms {
    // ---[ RANGE implementation
    template<typename Iterator>
    struct Range {
        using LazyIterator = Iterator; // required for accessing the used Iterator type from other locations
        using iterator = Iterator; //required for making chain events.
        Iterator m_begin;
        Iterator m_end;

        auto begin() { return m_begin; }

        auto end() { return m_end; }

        //added these lines for "to" function to work with "const".
        auto begin() const { return m_begin; }

        auto end() const { return m_end; }
    };

    template<typename Iterator>
    Range(Iterator, Iterator) -> Range<Iterator>;

    // ---[ TRANSFORM implementation
    template<typename Iterator, typename Callable>
    struct TransformingIterator : Iterator {
        using OriginalIterator = Iterator; // required for accessing the used Iterator type from other locations

        Callable callable;

        TransformingIterator(Iterator iterator, Callable callable) : Iterator{iterator}, callable{callable} {}

        Iterator &get_orig_iter() { return ((Iterator &) *this); }

        const Iterator &get_orig_iter() const { return ((Iterator &) *this); }

        //added const for correctness
        auto operator*() const { return callable(*get_orig_iter()); }
    };

    auto transform = [](auto action) {
        return [=](auto &&container) {
            using Container = std::decay_t<decltype(container)>;
            using Iterator = typename Container::iterator;
            using IteratorX = TransformingIterator<Iterator, decltype(action)>;
            return Range{IteratorX{container.begin(), action}, IteratorX{container.end(), action}};
        };
    };

    // ---[ FILTER implementation: implement your "Filtering Iterator" here

    template<typename Iterator, typename Callable>
    struct FilteringIterator : Iterator {

        using OriginalIterator = Iterator;

        Callable callable;


        FilteringIterator(Iterator iterator, Callable callable) : Iterator{iterator}, callable(callable) {}


        Iterator &get_orig_iter() { return ((Iterator &) *this); }

        const Iterator &get_orig_iter() const { return ((Iterator &) *this); }


        //we can directly dereference since != operator already checked if filter passed or not!
        auto operator*() const { return *get_orig_iter(); }

//        bool operator!=(FilteringIterator &&end) { return (*this).operator!=(end); }
        // extra r-value capturing is not necessary since "end" is not written to, make it const & to capture both l and r values.

        //since we need to filter "lazily" each of the actions/filtering must happen before the for-loop but within the for-loop!
        bool operator!=(const FilteringIterator &end) {
            auto &it = get_orig_iter();
            const auto &it_end = end.get_orig_iter();

            if (it == it_end) //if begin and end iterators are the same then we reached the end. return false.
                return false;

            if (!callable(*it)) {//if the current element does not pass the filter, try the next one.
                ++it;
                return (*this) != end;
            }
            return true; //not the end and, passed. All good!
        }

    };

    //similar to the transform function.
    auto filter = [](auto action) {
        return [=](auto &&container) {
            using Container = std::decay_t<decltype(container)>;
            using Iterator = typename Container::iterator;
            using IteratorY = FilteringIterator<Iterator, decltype(action)>;
            return Range{IteratorY{container.begin(), action}, IteratorY{container.end(), action}};
        };
    };

    // ---[ TO implementation: implement your "render into a container" method here

    template<template<typename...> typename Container>
    //template-template used to capture the class without its template<>.
    auto to() {
        return [&](const auto &range) -> decltype(auto) {
            const auto orig_iter = range.begin().get_orig_iter();
            //get an element from the iterator, by using decltype gets its type and initialize a Container with that type.
            auto c = Container<std::decay_t<decltype(*(orig_iter))>>{};
            for (auto e : range) //eagerly push the iterator to the container.
                c.push_back(e);
            return c;
        };
    }


    //OUT OF CONTEXT IMPLEMENTATION
    template<typename Iterator>
    struct EveryNth : Iterator {

        using OriginalIterator = Iterator;


        size_t iter_step;
        size_t start_from;

        EveryNth(Iterator iterator, size_t iter_step, size_t start_from) : Iterator{iterator}, iter_step{iter_step},
                                                                           start_from{start_from} {
            auto &it = get_orig_iter();
            it += start_from;
        }

        Iterator &get_orig_iter() { return ((Iterator &) *this); }

        const Iterator &get_orig_iter() const { return ((Iterator &) *this); }


        bool operator!=(const EveryNth &end) {
            auto &it = get_orig_iter();
            auto &it_end = end.get_orig_iter();

            if (it + (iter_step - 1) >= it_end)
                return false;
            it += (iter_step - 1);
            return true;
        }

        auto operator*() const {
            return *(get_orig_iter());
        }
    };

    auto everyNth = [](size_t n, size_t startFromIndex = 0) {
        return [=](auto &&container) {
            using Container = std::decay_t<decltype(container)>;
            using Iterator = typename Container::iterator;
            using IteratorY = EveryNth<Iterator>;
            return Range{IteratorY{container.begin(), n, startFromIndex},
                         IteratorY{container.end(), n, startFromIndex}};
        };
    };

}

//added SFINAE for RANGE check.
template<typename RANGE, typename ACTION, typename = typename std::decay_t<RANGE>::iterator>
auto operator|(RANGE &&range, ACTION &&action) { return action(std::forward<RANGE>(range)); }
//template<typename RANGE, typename ACTION>
//auto operator|(CONTAINER &&container, RANGE &&range) { return range(std::forward<CONTAINER>(container)); }


using namespace predicates;
using namespace actions;
using namespace algorithms;

int main(int argc, char *argv[]) {
    auto new_line = [] { std::cout << std::endl; };

// https://en.cppreference.com/w/cpp/language/range-for

    auto v = std::vector<double>{};
    auto odd_gen = views::odds();
    for (int i = 0; i < 5; ++i)
        v.push_back(odd_gen() * 2.5);
    // v contains {2.5, 7.5, 12.5, 17.5, 22.5} here


    auto range = v | transform(if_then(all_of(greater_than(2.0), less_than(15.0)), multiply_by(20)));
    for (auto a : range) // transformation is applied on the range as the for loop progresses
        std::cout << a << std::endl;
    // original v is not changed. prints {50.0, 150.0, 250.0, 17.5, 22.5}

    new_line();

    for (auto a : v | filter(greater_than(15))) // filter is applied lazily as the range is traversed
        std::cout << a << std::endl;
    // prints 17.5 and 22.5 to the console

    new_line();

    for (auto a : v | filter(less_than(15))) // filter is applied lazily as the range is traversed
        std::cout << a << std::endl;
    // prints 2.5, 7.5, 12.5 to the console

    new_line();
    auto u = std::vector<int>{10, 20, 30};

    //Note that you can chain as many transforms and filters as you wish.
    auto u_transformed = std::vector<int>{10, 20, 30, 40, 50, 60, 70, 80, 90} | transform(multiply_by(2)) |
                         filter(greater_than(20)) |
                         transform(multiply_by(10)) | to<std::vector>();
    for (auto a : u_transformed) // u_transformed is an std::vector<int> automatically because of to<std::vector>
        std::cout << a << std::endl;


    new_line();
    auto v2 = std::vector<int>{10, 20, 30, 40};
    auto r = v2 | everyNth(2);
    for (auto a : r)
        std::cout << a << std::endl;

    return 0;
}