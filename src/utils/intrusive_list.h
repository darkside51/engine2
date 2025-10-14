#pragma once

namespace engine {

    template <typename T>
    struct IntrusiveListNode {
        T * next = nullptr;
        T * prev = nullptr;

        struct Iterator {
            T * value = nullptr;
            Iterator& operator++() noexcept {
                value = value->next;
                return *this;
            }
            Iterator& operator--() noexcept {
                value = value->prev;
                return *this;
            }

            bool operator==(const Iterator &) const noexcept = default;
            T * operator->() noexcept { return value; }
            T & operator*() noexcept { return *value; }
            const T * operator->() const noexcept { return value; }
            const T & operator*() const noexcept { return *value; }

            const T * get() const noexcept { return value; }
            T * get() noexcept { return value; }
        };
    };

}