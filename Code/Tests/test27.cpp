// Strings too long for a text fail cleanly: a run of 2 GiB of characters that need no escape fails to write with
// kFileTooLarge. The check takes 4 GiB of memory, so the test is skipped where size_t has 32 bits.

#include "TestCheck.h"

#include <cstdint>

#if defined( JSON4C4_ENABLE_STD_SUPPORT_INTERNAL ) && ( SIZE_MAX > 0xFFFFFFFFU )

#    include <string>

void TestLongRun()
{
    Json::StructuredData data;

    {
        std::string text( size_t( 1 ) << 31, 'a' );

        CheckStatus( data.SerializeFrom( text ), Json::Status::kOk, "serialize a string of 2 GiB" );
    }

    Terathon::Array<char> output;

    CheckStatus( data.Write( output ), Json::Status::kFileTooLarge, "a string of 2 GiB is too long to write" );
}

int main()
{
    TestLongRun();

    return TestResult();
}

#else

int main()
{
    return testSkippedReturnCode;
}

#endif
