#ifndef TestAllocation_h
#define TestAllocation_h

#include <cstdlib>
#include <new>

#if ( defined( __cpp_exceptions ) || defined( _CPPUNWIND ) ) && !defined( TERATHON_NO_SYSTEM )

#    define JSON4C4_TEST_EXCEPTIONS

#endif

#ifndef JSON4C4_TEST_NO_ALLOCATION_HOOK

#    define JSON4C4_TEST_ALLOCATION_HOOK

inline long   allocationLiveCount      = 0;
inline long   allocationTotalCount     = 0;
inline long   allocationFailCountdown  = -1;
inline size_t allocationFailAboveSize  = 0;
inline bool   allocationFailureEnabled = false;

inline bool AllocationShouldFail( size_t size )
{
    if ( !allocationFailureEnabled )
    {
        return false;
    }

    if ( allocationFailAboveSize != 0 && size >= allocationFailAboveSize )
    {
        return true;
    }

    if ( allocationFailCountdown > 0 && --allocationFailCountdown == 0 )
    {
        return true;
    }

    return false;
}

void* operator new( size_t size )
{
#    ifdef JSON4C4_TEST_EXCEPTIONS

    if ( AllocationShouldFail( size ) )
    {
        throw std::bad_alloc();
    }

#    endif

    void* pointer = malloc( size != 0 ? size : 1 );

    if ( !pointer )
    {
        abort();
    }

    allocationLiveCount++;
    allocationTotalCount++;

    return pointer;
}

void* operator new[]( size_t size )
{
    return operator new( size );
}

void operator delete( void* pointer ) noexcept
{
    if ( pointer )
    {
        allocationLiveCount--;
        free( pointer );
    }
}

void operator delete[]( void* pointer ) noexcept
{
    operator delete( pointer );
}

void operator delete( void* pointer, size_t ) noexcept
{
    operator delete( pointer );
}

void operator delete[]( void* pointer, size_t ) noexcept
{
    operator delete( pointer );
}

#endif

#endif
