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
#error This file must only be included from SparseBitmap.h
#endif

//----------------------------------------------------------------------
template <typename _inttype_t>
SparseBitmap<_inttype_t>::SparseBitmap()
{
}

//----------------------------------------------------------------------
template <typename _inttype_t>
void
SparseBitmap<_inttype_t>::set(_inttype_t start, _inttype_t len)
{
    typename RangeVector::iterator i = this->bitmap_.begin();
    typename RangeVector::iterator j;

    ASSERT(len != 0);

    _inttype_t end = start + len - 1;

    while (1) {
        // case 1: reached the end of the line
        if (i == this->bitmap_.end())
        {
            i = this->bitmap_.insert(i, Range(start, end));
            break;
        }

        // case 2: extend a range to the right
        else if (start >= i->start_ && start <= (i->end_ + 1))
        {
            if (end <= i->end_) return; // nothing to do
            
            i->end_ = end;
            break;
        } 

        // case 3: extend a range to the left
        else if (end >= (i->start_ - 1) && end <= i->end_)
        {
            if (start >= i->start_) return; // nothing to do

            i->start_ = start;
            break;
        }

        // case 4: insert a new entry
        else if (end < i->start_)
        {
            i = this->bitmap_.insert(i, Range(start, end));
            break;
        }

        i++;
    }

    // compact the entries... first we remove any entries to the right
    // that are covered by the newly added entry, then we do the same
    // on the left. note that we must be careful not to reuse an
    // iterator after calling erase(). instead, we use the fact that
    // erase() returns an iterator immediately after the deleted
    // element, and then we adjust i and j accordingly
    
    j = i + 1;
    while (j != this->bitmap_.end()) {
        if (i->end_ >= (j->start_ - 1)) {
            if (i->end_ < j->end_)
                i->end_ = j->end_;
            
            j = this->bitmap_.erase(j);
            i = j - 1;
            
        } else {
            break;
        }
    }

    while (i != this->bitmap_.begin()) {
        j = i - 1;
        
        if (j->end_ >= (i->start_ - 1)) {
            if (i->start_ > j->start_) {
                i->start_ = j->start_;
            }
            
            if (i->end_ < j->end_) {
                i->end_ = j->end_;
            }
            i = this->bitmap_.erase(j);
            
        } else {
            break;
        }
    }

    validate();
}

//----------------------------------------------------------------------
template <typename _inttype_t>
void
SparseBitmap<_inttype_t>::clear(_inttype_t start, _inttype_t len)
{
    typename RangeVector::iterator i = this->bitmap_.begin();
    typename RangeVector::iterator j;

    _inttype_t end = start + len - 1;
    
    while (i != this->bitmap_.end()) {
        // case 1: the entire entry is cleared
        if (start <= i->start_ && end >= i->end_)
        {
            size_t offset = i - this->bitmap_.begin();
            this->bitmap_.erase(i);
            i = this->bitmap_.begin() + offset;
            continue; // don't fall through to incr i
        }

        // case 2: shrink an entry on the left
        else if (start <= i->start_ && end < i->end_) {
            i->start_ = end + 1;
        }

        else if (start > i->start_ && start < i->end_)
        {
            // case 3: shrink an entry on the right
            if (end >= i->end_)
            {
                i->end_ = start - 1;
            }
            
            // case 4: split an entry into two
            else if (end < i->end_)
            {
                j = bitmap_.insert(i + 1, Range(end, i->end_));
                i->end_ = start;
            }
        }

        i++;
    };        
}

//----------------------------------------------------------------------
template <typename _inttype_t>
bool
SparseBitmap<_inttype_t>::is_set(_inttype_t start, _inttype_t len)
{
    typename RangeVector::iterator i;
    for (i = this->bitmap_.begin(); i != this->bitmap_.end(); ++i) {
        if (start >= i->start_ && (start + len - 1) <= i->end_) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
_inttype_t
SparseBitmap<_inttype_t>::num_contiguous()
{
    typename RangeVector::iterator i = this->bitmap_.begin();
    if (i == this->bitmap_.end()) {
        return 0;
    }
    return i->end_ - i->start_ + 1;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
_inttype_t
SparseBitmap<_inttype_t>::num_set()
{
    _inttype_t ret = 0;
    typename RangeVector::iterator i;
    for (i= this->bitmap_.begin(); i != this->bitmap_.end(); ++i) {
        ret += i->end_ - i->start_ + 1;
    }
    return ret;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
int
SparseBitmap<_inttype_t>::format(char* bp, size_t buflen) const
{
    size_t len = 0;
    size_t ret;

    ret = snprintf(bp, buflen, "[ ");
    buflen = (ret < buflen) ? (buflen - ret) : 0;
    bp     += ret;
    len    += ret;

    typename RangeVector::const_iterator i;
    for (i = this->bitmap_.begin(); i != this->bitmap_.end(); ++i) {
        if (i->start_ == i->end_) {
            ret = snprintf(bp, buflen, "%ld ", (long int)i->start_);
        } else {
            ret = snprintf(bp, buflen, "%ld..%ld ",
                           (long int)i->start_, (long int)i->end_);
        }
        
        buflen = (ret < buflen) ? (buflen - ret) : 0;
        bp     += ret;
        len    += ret;
    }
    
    ret = snprintf(bp, buflen, "]");
    buflen = (ret < buflen) ? (buflen - ret) : 0;
    bp     += ret;
    len    += ret;

    return len;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
void
SparseBitmap<_inttype_t>::validate()
{
#ifndef NDEBUG
    typename RangeVector::iterator i = this->bitmap_.begin();
    typename RangeVector::iterator j = i + 1;

    ASSERT(i->start_ <= i->end_);
    
    while (j != this->bitmap_.end()) {
        ASSERT(j->start_ <= j->end_);
        ASSERT(i->end_ < j->start_);
        ++i; ++j;
    }
#endif
}

//----------------------------------------------------------------------
template <typename _inttype_t>
SparseBitmap<_inttype_t>::iterator::iterator()
    : iter_(), offset_(0)
{
}

//----------------------------------------------------------------------
template <typename _inttype_t>
SparseBitmap<_inttype_t>::iterator::iterator(typename RangeVector::iterator iter,
                                             _inttype_t offset)
    : iter_(iter), offset_(offset)
{
}

//----------------------------------------------------------------------
template <typename _inttype_t>
_inttype_t
SparseBitmap<_inttype_t>::iterator::operator*()
{
    return iter_->start_ + offset_;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator&
SparseBitmap<_inttype_t>::iterator::operator++()
{
    if (iter_->start_ + offset_ == iter_->end_) {
        iter_++;
        offset_ = 0;
    } else {
        offset_++;
    }
    return *this;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::iterator::operator++(int)
{
    typename SparseBitmap::iterator tmp = *this;
    ++*this;
    return tmp;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::iterator::operator+(int diff)
{
    typename SparseBitmap::iterator tmp = *this;

    // XXX/demmer this is quite inefficient if diff is large, but
    // typically we only use this operator for small numbers (i.e. 1)
    while (diff != 0) {
        tmp++;
        diff--;
    }
    return tmp;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator&
SparseBitmap<_inttype_t>::iterator::operator--()
{
    // checking for an offset_ of zero covers both the case where
    // we're pointing at the start_ of some iterator as well as the
    // case where we're pointing at end()
    if (offset_ == 0) {
        iter_--;
        offset_ = iter_->end_ - iter_->start_;
    } else {
        offset_--;
    }
    return *this;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::iterator::operator--(int)
{
    typename SparseBitmap::iterator tmp = *this;
    --*this;
    return tmp;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::iterator::operator-(int diff)
{
    typename SparseBitmap::iterator tmp = *this;

    // XXX/demmer this is quite inefficient if diff is large, but
    // typically we only use this operator for small numbers (i.e. 1)
    while (diff != 0) {
        tmp--;
        diff--;
    }
    return tmp;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
bool
SparseBitmap<_inttype_t>::iterator::operator==(const iterator& other)
{
    return (iter_ == other.iter_) && (offset_ == other.offset_);
}

//----------------------------------------------------------------------
template <typename _inttype_t>
bool
SparseBitmap<_inttype_t>::iterator::operator!=(const iterator& other)
{
    return (iter_ != other.iter_) || (offset_ != other.offset_);
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator&
SparseBitmap<_inttype_t>::iterator::skip_contiguous()
{
    offset_ = iter_->end_ - iter_->start_;
    return *this;
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::begin()
{
    return iterator(bitmap_.begin(), 0);
}

//----------------------------------------------------------------------
template <typename _inttype_t>
typename SparseBitmap<_inttype_t>::iterator
SparseBitmap<_inttype_t>::end()
{
    return iterator(bitmap_.end(), 0);
}
