#ifndef __FCNADAPTERS_H__
#define __FCNADAPTERS_H__

namespace oasys {

/*! \file These are various adaptors for performing common operations
 * with the STL algorithms (and their friends). It is possible to
 * construct these operations with the STL -- however, the syntax is
 * awkward and difficult to dicipher (can be said for all of the
 * internals of the STL).
 *
 * The lower case function should be used instead of the classes
 * directly.
 */

//! See below for the function you should use
template<typename _Value, typename _Class, typename _Comp>
struct CompFunctor {
    CompFunctor(const _Value& value, 
                _Value        (_Class::*m_fcn_ptr)() const,
                const _Comp&  comparator) 
        : value_(value), 
          m_fcn_ptr_(m_fcn_ptr),
          comparator_(comparator) {}
    
    bool operator()(const _Class& other) const {
        const _Value& other_val = (other.*m_fcn_ptr_)();
	return comparator_(value_, other_val);
    }
    
    const _Value& value_;
    _Value (_Class::*m_fcn_ptr_)() const;
    const _Comp&  comparator_;
};

/*!
 * This function takes an object t of type _T and a member accessor
 * function f and returns the functor:
 *
 *      bool F(const _T& other) { 
 *          return other.f() -OP- t.f(); 
 *      }
 */
template<typename _Value, typename _Class, typename _Comp>
CompFunctor<_Value, _Class, _Comp> 
comp_functor(const _Class& t, 
             _Value        (_Class::*m_fcn_ptr)() const,
             _Comp         comparator)
{
    return CompFunctor<_Value, _Class, _Comp>(t, m_fcn_ptr, comparator);
}

/*!
 * @{ This next set of functions implement the above for the standard
 * operator overloads.
 */
#define MAKE_FUNCTOR(_name, _operator)                          \
template<typename _Value, typename _Class>                      \
CompFunctor<_Value, _Class, _operator<_Value> >                 \
_name(const _Value& value, _Value (_Class::*m_fcn_ptr)() const) \
{                                                               \
    _operator<_Value> comp;                                     \
    return CompFunctor<_Value, _Class, _operator<_Value> >      \
        (value, m_fcn_ptr, comp);                               \
}

MAKE_FUNCTOR(eq_functor,  std::equal_to);
MAKE_FUNCTOR(neq_functor, std::not_equal_to);
MAKE_FUNCTOR(gt_functor,  std::greater);
MAKE_FUNCTOR(lt_functor,  std::less);
MAKE_FUNCTOR(gte_functor, std::greater_equal);
MAKE_FUNCTOR(lte_functor, std::less_equal);

#undef MAKE_FUNCTOR

//! @}

} // namespace oasys

#endif /* __FCNADAPTERS_H__ */
