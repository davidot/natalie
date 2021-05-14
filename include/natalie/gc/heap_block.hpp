#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/cell.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

constexpr const size_t HEAP_BLOCK_SIZE = 32 * 1024;
constexpr const size_t HEAP_BLOCK_MASK = ~(HEAP_BLOCK_SIZE - 1);
constexpr const size_t HEAP_CELL_COUNT_MAX = HEAP_BLOCK_SIZE / 16; // 16 bytes is the smallest cell we will allocate

class HeapBlock {
public:
    NAT_MAKE_NONCOPYABLE(HeapBlock);

    HeapBlock(size_t size)
        : m_cell_size { size }
        , m_total_count { (HEAP_BLOCK_SIZE - sizeof(HeapBlock)) / m_cell_size }
        , m_free_count { m_total_count } {
        memset(m_memory, 0, HEAP_BLOCK_SIZE - sizeof(HeapBlock));
    }

    ~HeapBlock() { }

    static HeapBlock *from_cell(const Cell *cell) {
        return reinterpret_cast<HeapBlock *>((intptr_t)cell & HEAP_BLOCK_MASK);
    }

    Cell *cell_from_index(size_t index) {
        void *cell = &m_memory[index * m_cell_size];
        return static_cast<Cell *>(cell);
    }

    ssize_t index_from_cell(const Cell *cell) const {
        for (size_t i = 0; i < m_total_count; ++i) {
            if (reinterpret_cast<const Cell *>(&m_memory[i * m_cell_size]) == cell)
                return i;
        }
        return -1;
    }

    bool has_free() {
        return m_free_count > 0;
    }

    void *next_free() {
        assert(has_free());
        for (size_t i = 0; i < m_total_count; ++i) {
            if (!m_used_map[i]) {
                m_used_map[i] = true;
                --m_free_count;
                void *cell = &m_memory[i * m_cell_size];
                //printf("Cell %p allocated from block %p (size %zu) at index %zu\n", cell, this, m_cell_size, i);
                return cell;
            }
        }
        NAT_UNREACHABLE();
    }

    bool is_cell_in_use(const Cell *cell) const {
        for (size_t i = 0; i < m_total_count; ++i) {
            if (reinterpret_cast<const Cell *>(&m_memory[i * m_cell_size]) == cell)
                return m_used_map[i];
        }
        return false;
    }

    void add_cell_to_free_list(const Cell *cell) {
        for (size_t i = 0; i < m_total_count; ++i) {
            if (reinterpret_cast<const Cell *>(&m_memory[i * m_cell_size]) == cell)
                m_used_map[i] = false;
        }
    }

    // returns m_total_count if no more cells remain
    size_t next_free_index_from(size_t index) {
        while (index < m_total_count) {
            if (m_used_map[index])
                break;
            ++index;
        }
        return index;
    }

    size_t total_count() const { return m_total_count; }

    class iterator {
    public:
        iterator(HeapBlock *block, size_t index)
            : m_block { block }
            , m_index { index } {
            m_ptr = m_block->cell_from_index(m_index);
        }

        iterator operator++() {
            m_index = m_block->next_free_index_from(m_index + 1);
            m_ptr = m_block->cell_from_index(m_index);
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            m_index = m_block->next_free_index_from(m_index + 1);
            m_ptr = m_block->cell_from_index(m_index);
            return i;
        }

        Cell *&operator*() {
            return m_ptr;
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_block == i2.m_block && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_block != i2.m_block || i1.m_index != i2.m_index;
        }

    private:
        HeapBlock *m_block;
        Cell *m_ptr { nullptr };
        size_t m_index { 0 };
    };

    iterator begin() {
        auto index = next_free_index_from(0);
        return iterator { this, index };
    }

    iterator end() {
        return iterator { this, m_total_count };
    }

private:
    size_t m_cell_size;
    size_t m_total_count { 0 };
    size_t m_free_count { 0 };
    bool m_used_map[HEAP_CELL_COUNT_MAX] {};
    alignas(Cell) char m_memory[];
};

}
