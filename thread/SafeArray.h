/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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
#ifndef _OASYS_SAFEARRAY_H_
#define _OASYS_SAFEARRAY_H_

#include "Atomic.h"

/**
 * Class that provides a simple fixed-size array of 32-bit values, but
 * with non-locking thread safe semantics. This makes it suitable for
 * things which may need to be set or read from within a signal
 * handler.
 */
namespace oasys {

template<size_t _sz, typename _Type, unsigned int _emptyval>
class SafeArray {
public:
    /// Constructor that initializes every slot to the empty value
    SafeArray()
    {
        STATIC_ASSERT(sizeof(_Type) == 4, Safe_Array_Type_Is_32_Bits);
        
        for (size_t i = 0; i < _sz; ++i) {
            array_[i] = _emptyval;
        }
        size_ = _sz;
    }

    /**
     * Insert finds the first empty slot and puts the given value in
     * there. Note that there is no guarantee that it will really find
     * the _first_ empty slot, since there could be a race.
     *
     * @returns the index of the insertion or -1 if it failed to find
     * a free slot
     */
    int insert(_Type val)
    {
        _Type oldval;
        for (size_t i = 0; i < _sz; ++i) {
            oldval = (_Type)atomic_cmpxchg32(&array_[i],
                                             (unsigned int)_emptyval,
                                             (unsigned int)val);
            if (oldval == _emptyval) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Remove finds the slot occupied by the given value and resets it
     * to the empty value.
     *
     * @returns the old index of the value or -1 if it wasn't in there
     */
    int remove(_Type val)
    {
        _Type oldval;
        for (size_t i = 0; i < _sz; ++i) {
            oldval = (_Type)atomic_cmpxchg32(&array_[i],
                                             (unsigned int)val,
                                             (unsigned int)_emptyval);
            if (oldval == val) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Array operator.
     */
    _Type operator[](size_t i)
    {
        return array_[i];
    }

    /// @{
    /// Accessors
    size_t              size()  const { return size_; }
    const _Type* array() const { return array_; }
    /// @}

protected:
    
    /// The size of the array, copied from the template parameter so
    /// classes that want to iterate over the array can do so
    size_t size_;

    /// The array of values
    _Type array_[_sz];
};

} // namespace oasys

#endif /* _OASYS_SAFEARRAY_H_ */
