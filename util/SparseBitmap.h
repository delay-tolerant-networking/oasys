/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _OASYS_SPARSE_BITMAP_H_
#define _OASYS_SPARSE_BITMAP_H_

#include <vector>
#include "config.h"
#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * This class represents a bitmap, where the implementation is biased
 * towards space efficiency rather than lookup time. To that end, the
 * implementation uses a vector of bitmap entries, each storing a start
 * and an end value.
 *
 * Note that in all cases, ranges must be well-formed, i.e. the end
 * must always be greater than start.
 */
template <typename _inttype_t>
class SparseBitmap {
public:
    /**
     * Constructor.
     */
    SparseBitmap();

    /**
     * Set the given bit range to true.
     */
    void set(_inttype_t start, _inttype_t len = 1);

    /**
     * Set the given bit range to false.
     */
    void clear(_inttype_t start, _inttype_t len = 1);

    /**
     * Check if the given range is set.
     */
    bool is_set(_inttype_t start, _inttype_t len = 1);

    /**
     * Array operator overload for syntactic sugar.
     */
    bool operator[](_inttype_t val)
    {
        return is_set(val, 1);
    }

    /**
     * Return the total number of set bits.
     */
    _inttype_t num_set();

    /**
     * Return whether or not the bitmap is empty
     */
    bool empty() { return bitmap_.empty(); }

    /**
     * Return the number of range entries (for testing only).
     */
    size_t num_entries() { return bitmap_.size(); }

    /**
     * Clear the whole bitmap.
     */
    void clear() { bitmap_.clear(); }
    
    /**
     * Return the total number of contiguous bits on the left of the
     * range.
     */
    _inttype_t num_contiguous();

protected:
    struct Range {
        Range(_inttype_t start, _inttype_t end)
            : start_(start), end_(end) {}
        
        _inttype_t start_;
        _inttype_t end_;
    };
    
    typedef std::vector<Range> RangeVector;
    RangeVector bitmap_;

    void validate();

public:
    /**
     * An STL-like iterator class. However, to keep in-line with the
     * sparse nature of the class, incrementing the iterator advances
     * only through over the set bits in the bitmap, and the
     * dereference operator returns the offset of the set bit.
     *
     * For example, if bits 1, 5, and 10 are set, then dereferencing
     * the return from ::begin() returns 1, incrementing and
     * dereferencing returns 5, etc.
     */
    class iterator {
    public:
        /**
         * Constructor to initialize an empty iterator.
         */
        iterator();

        /**
         * Dereference operator returns the current bit offset.
         */
        _inttype_t operator*();

        /**
         * Prefix increment operator.
         */
        iterator& operator++();

        /**
         * Postfix increment operator.
         */
        iterator operator++(int);

        /**
         * Addition operator.
         */
        iterator operator+(int diff);

        /**
         * Prefix decrement operator.
         */
        iterator& operator--();

        /**
         * Postfix decrement operator.
         */
        iterator operator--(int);
        
        /**
         * Subtraction operator.
         */
        iterator operator-(int diff);

        /**
         * Equality operator.
         */
        bool operator==(const iterator& other);

        /**
         * Inequality operator.
         */
        bool operator!=(const iterator& other);

        /**
         * Advance past any contiguous bits, returning an iterator at
         * the last contiguous bit that's set. The iterator must not
         * be pointing at end() for this to be called.
         */
        iterator& skip_contiguous();

    private:
        friend class SparseBitmap<_inttype_t>;

        /// Private constructor used by begin() and end()
        iterator(typename RangeVector::iterator iter,
                 _inttype_t offset);

        /// iterator to the current Range
        typename RangeVector::iterator iter_;
        
        /// offset from start_ in the range
        _inttype_t offset_;
    };

    /**
     * Return an iterator at the start of the vector
     */
    iterator begin();

    /**
     * Return an iterator at the end of the vector
     */
    iterator end();

    /**
     * Syntactic sugar to get the first bit set.
     */
    _inttype_t first() { return *begin(); }

    /**
     * Syntactic sugar to get the last bit set.
     */
    _inttype_t last() { return *--end(); }
};

#include "SparseBitmap.tcc"

} // namespace oasys

#endif /* _OASYS_SPARSE_BITMAP_H_ */
