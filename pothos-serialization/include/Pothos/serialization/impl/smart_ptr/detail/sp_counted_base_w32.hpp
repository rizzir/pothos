#ifndef POTHOS_SMART_PTR_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED
#define POTHOS_SMART_PTR_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  detail/sp_counted_base_w32.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
//  Lock-free algorithm by Alexander Terekhov
//
//  Thanks to Ben Hitchings for the #weak + (#shared != 0)
//  formulation
//

#include <Pothos/serialization/impl/detail/interlocked.hpp>
#include <Pothos/serialization/impl/detail/workaround.hpp>
#include <Pothos/serialization/impl/detail/sp_typeinfo.hpp>

namespace Pothos
{

namespace detail
{

class sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    long use_count_;        // #shared
    long weak_count_;       // #weak + (#shared != 0)

public:

    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
    {
    }

    virtual ~sp_counted_base() // nothrow
    {
    }

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destroy() is called when weak_count_ drops to zero.

    virtual void destroy() // nothrow
    {
        delete this;
    }

    virtual void * get_deleter( sp_typeinfo const & ti ) = 0;
    virtual void * get_untyped_deleter() = 0;

    void add_ref_copy()
    {
        POTHOS_INTERLOCKED_INCREMENT( &use_count_ );
    }

    bool add_ref_lock() // true on success
    {
        for( ;; )
        {
            long tmp = static_cast< long const volatile& >( use_count_ );
            if( tmp == 0 ) return false;

#if defined( POTHOS_MSVC ) && POTHOS_WORKAROUND( POTHOS_MSVC, == 1200 )

            // work around a code generation bug

            long tmp2 = tmp + 1;
            if( POTHOS_INTERLOCKED_COMPARE_EXCHANGE( &use_count_, tmp2, tmp ) == tmp2 - 1 ) return true;

#else

            if( POTHOS_INTERLOCKED_COMPARE_EXCHANGE( &use_count_, tmp + 1, tmp ) == tmp ) return true;

#endif
        }
    }

    void release() // nothrow
    {
        if( POTHOS_INTERLOCKED_DECREMENT( &use_count_ ) == 0 )
        {
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() // nothrow
    {
        POTHOS_INTERLOCKED_INCREMENT( &weak_count_ );
    }

    void weak_release() // nothrow
    {
        if( POTHOS_INTERLOCKED_DECREMENT( &weak_count_ ) == 0 )
        {
            destroy();
        }
    }

    long use_count() const // nothrow
    {
        return static_cast<long const volatile &>( use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED
