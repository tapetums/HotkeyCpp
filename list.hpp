#pragma once

//---------------------------------------------------------------------------//
//
// list.hpp
//  簡易リスト
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

namespace tapetums
{
    template<typename T> struct list;
} 

//---------------------------------------------------------------------------//

// 双方向リスト
template<typename T>
struct tapetums::list
{
    list* sentinel;
    list* prev;
    list* next;

    T data;

    list() : sentinel(this), prev(this), next(this) { }
    ~list() { clear(); }

    list(const list&) = delete;
    list& operator =(const list&) = delete;

    list(list&&) noexcept = default;
    list& operator =(list&&) noexcept = default;

    void push_back(const T& data_)
    {
        auto node = new list;
        node->data = data_;

        node->sentinel = sentinel;
        node->perv = sentinel->prev;
        node->next = sentinel;

        sentinel->prev->next = node;
        sentinel->prev = node;
    }

    void push_back(T&& data_)
    {
        auto node = new list;
        std::swap(node->data, data_);

        node->sentinel = sentinel;
        node->prev = sentinel->prev;
        node->next = sentinel;

        sentinel->prev->next = node;
        sentinel->prev = node;
    }

    template<typename... Args>
    void emplace(const Args&... args) { push_back(std::move(T(args...))); }

    template<typename... Args>
    void emplace(Args&&... args) { push_back(std::move(T(args...))); }

    void clear()
    {
        if ( next == sentinel ) { return; }
        delete next;
    }

    T& get() { return data; }

    struct iterator
    {
        list* node;

        iterator(list* node) : node(node) { }

        iterator operator ++() noexcept { return node = node->next; }
        iterator operator --() noexcept { return node = node->prev; }

        constexpr T& operator *()  const noexcept { return node->data; }
        T&           operator *()  noexcept       { return node->data; }

        constexpr T* operator ->() const noexcept { return &node->data; }
        T*           operator ->() noexcept       { return &node->data; }

        bool operator ==(const iterator& lhs) const noexcept { return node->next == lhs.node->next; }
        bool operator !=(const iterator& lhs) const noexcept { return !(*this == lhs); }
    };

    iterator begin() noexcept { return iterator(next); }
    iterator end()   noexcept { return iterator(sentinel); }

    iterator begin() const noexcept { return iterator(next); }
    iterator end()   const noexcept { return iterator(sentinel); }

    iterator erase(const iterator& it)
    {
        it.node->prev->next = it.node->next;
        it.node->next->prev = it.node->prev;

        auto next = it.node->next;
        it.node->next = it.node->sentinel;
        delete it.node;

        return iterator(next);
    }

};

//---------------------------------------------------------------------------//

/*#include <iostream>

namespace tapetums_list
{
    void unittest()
    {
        tapetums::list<char> list;
        for ( auto c = 0x40; c < 0x50; ++c )
        {
            list.push_back(c);
            std::cout << char(c) << " ";
        }

        std::cout << "\n\n";

        for ( auto node = list.begin(); node != list.end(); ++node )
        {
            std::cout << *node << " ";
        }

        std::cout << "\n\n";

        auto it = list.begin();
        const auto end = list.end();
        while ( it != end )
        {
            if ( *it == 'E' || *it == 'F' )
            {
                it = list.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for ( const auto& c: list )
        {
            std::cout << c << " ";
        }
    }
}*/

//---------------------------------------------------------------------------//

// list.hpp