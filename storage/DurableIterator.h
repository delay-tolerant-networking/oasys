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


#ifndef __OASYS_DURABLE_STORE_INTERNAL_HEADER__
#error DurableIterator.h must only be included from within DurableStore.h
#endif


/**
 * Abstract base class for a table iterator. Note: It is important
 * that iterators do NOT outlive the tables they point into.
 */
class DurableIterator {
public:
    // virtual destructor
    virtual ~DurableIterator() {}

    /**
     * Advance the pointer. An initialized iterator will be pointing
     * right before the first element in the list, so iteration code
     * will always be:
     *
     * @code
     * MyKeyType key;
     * DurableIterator* i = table->iter();
     * while(i->next() == 0) {
     *    i->get(&key);
     *    // ... do stuff
     * }
     * @endcode
     *
     * @return DS_OK, DS_NOTFOUND if no more elements, DS_ERR if an
     * error occurred while iterating.
     */
    virtual int next() = 0;

    /**
     * Unserialize the current element into the given key object.
     */
    virtual int get_key(SerializableObject* key) = 0;
};

//----------------------------------------------------------------------------
/*!
 * Template class for a "filtered" iterator which only iterates over
 * desired patterns.
 *
 * _filter: struct { accept: DurableIterator -> bool }
 */
template<typename _filter>
class DurableFilterIterator : public DurableIterator {
public:
    DurableFilterIterator(DurableIterator* itr)
        : itr_(itr) {}

    // virtual from DurableIterator
    int next() 
    {
        for (;;) 
        {
            int ret = itr_->next();
            if (ret != DS_OK) {
                return ret;
            }
            
            if (_filter::accept(itr_)) {
                return DS_OK;
            }
        }
    }

    int get_key(SerializableObject* key) 
    {
        return itr_->get_key(key);
    }
    
private:
    DurableIterator* itr_;
};
