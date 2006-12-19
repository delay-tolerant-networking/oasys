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
template<typename _T, typename _Ret, typename _Comp>
struct CompFunctor {
    Eq(const _T& t, _Ret (_T::*m_fcn_ptr)() const) 
        : t_(t), m_fcn_ptr_(m_fcn_ptr) {}
    
    bool operator()(const _T& other) const {
        _Comp comparator;
	return comparator(t_.*m_fcn_ptr_(), other.*m_fcn_ptr_());
    }
    
    const _T& t_;
    _Ret (_T::*m_fcn_ptr_)() const;
};

/*!
 * This function takes an object t of type _T and a member accessor
 * function f and returns the functor:
 *
 *      bool F(const _T& other) { 
 *          return other.f() -OP- t.f(); 
 *      }
 */
template<typename _T, typename _Ret, typename _Comp>
CompFunctor<_T, _Ret, _Comp> 
comp_functor(const _T& t, _Ret (_T::*m_fcn_ptr)() const)
{
    return CompFunctor<_T, _Ret, _Comp>(t, m_fcn_ptr);
}

/*!
 * @{ This next set of functions implement the above for the standard
 * operator overloads.
 */
#define MAKE_FUNCTOR(_name, _operator)
template<typename _T, typename _Ret>
CompFunctor<_T, _Ret, _operator>
_name(const _T& t, _Ret (_T::*m_fcn_ptr)() const)
{
    return CompFunctor<_T, _Ret, operator>(t, m_fcn_ptr);
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
