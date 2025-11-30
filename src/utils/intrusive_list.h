#pragma once

namespace engine {

    template <typename T>
    struct IntrusiveListNode {
        using Type = T;
        IntrusiveListNode<T> * next = nullptr;
        IntrusiveListNode<T> * prev = nullptr;

        inline T * get() noexcept { return static_cast<T*>(this); }
        inline const T * get() const noexcept { return static_cast<T*>(this); }
        
        inline T & operator*() noexcept { return static_cast<T&>(*this); }
        inline const T & operator*() const noexcept { return static_cast<T&>(*this); }
    };

    template <typename T>
    struct IntrusiveList {
        using Node = IntrusiveListNode<T>;
        struct Iterator {
            Node * value = nullptr;
            Iterator& operator++() noexcept {
                value = value->next;
                return *this;
            }
            Iterator& operator--() noexcept {
                value = value->prev;
                return *this;
            }

            bool operator==(const Iterator &) const noexcept = default;
            Node::Type * operator->() noexcept { return value->get(); }
            Node::Type & operator*() noexcept { return value->operator*(); }
            const Node::Type * operator->() const noexcept { return value->get(); }
            const Node::Type & operator*() const noexcept { return value->operator*(); }

            const Node::Type * get() const noexcept { return value->get(); }
            Node::Type * get() noexcept { return value->get(); }
            const Node * node() const noexcept { return value; }
        };

        Iterator begin() const noexcept {
            return {.value = first};
        }

        Iterator end() const noexcept {
            return {.value = last->next};
        }

        size_t size() const noexcept {
            size_t result = 0u;
            for (const auto & value : *this) {
                ++result;
            }
            return result;
        }

        bool empty() const noexcept {
            return first == nullptr;
        }

        void clear() noexcept {
            first = nullptr;
            last = nullptr;
        }

        void emplace(Node * node) noexcept {
            node->next = nullptr;
            node->prev = nullptr;
            if (first == nullptr) {
                first = node;
            } else {
                node->prev = last;
                last->next = node;
            }
            last = node;
        }

        void remove(Node * node) noexcept {
            if (node->prev) {
                node->prev->next = node->next;
            }
            if (node->next) {
                node->next->prev = node->prev;
            }
            if (last == node) {
                last = node->prev;
            }
            if (first == node) {
                first = node->next;
            }
        }

        // sorting the list (e.g., using Merge Sort)
        template <typename Comparator>
        void stable_sort(Comparator && cmp) noexcept {
            // https://www.tutorialspoint.com/merge-sort-for-doubly-linked-list-using-cplusplus
            first = mergeSort(first, cmp);
            // update tail after sorting if needed, by traversing to the end
            Node * current = first;
            if (current) {
                while (current->next) {
                    current = current->next;
                }
                last = current;
            } else {
                last = nullptr;
            }
        }

    private:
        template <typename Comparator>
        Node * mergeSort(Node * h, const Comparator & cmp) noexcept {
            if (h == nullptr || h->next == nullptr) {
                return h; // base case: 0 or 1 node
            }

            Node * middle = getMiddle(h);
            Node * nextOfMiddle = middle->next;
            middle->next = nullptr; // split the list

            Node * left = mergeSort(h, cmp);
            Node * right = mergeSort(nextOfMiddle, cmp);

            return merge(left, right, cmp);
        }

        // helper function to find the middle node
        Node * getMiddle(Node * h) const noexcept {
            Node* slow = h;
            Node* fast = h;
            while (fast->next != nullptr && fast->next->next != nullptr) {
                slow = slow->next;
                fast = fast->next->next;
            }
            return slow;
        }

        // helper function to merge two sorted lists
        template <typename Comparator>
        Node * merge(Node * first, Node * second, const Comparator & cmp) noexcept {
            if (first == nullptr) {
                return second;
            }
            if (second == nullptr) {
                return first;
            }

            Node* result = nullptr;
            if (!cmp(second->operator*(), first->operator*())) {
                result = first;
                result->next = merge(first->next, second, cmp);
                if (result->next) {
                    result->next->prev = result;
                }
                result->prev = nullptr;
            } else {
                result = second;
                result->next = merge(first, second->next, cmp);
                if (result->next) {
                    result->next->prev = result;
                }
                result->prev = nullptr;
            }
            return result;
        }

    public:
        Node * first = nullptr;
        Node * last = nullptr;
    };

}