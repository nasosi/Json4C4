// Strings too long for a text or a value fail cleanly: a run of 2 GiB of characters that need no escape fails to write
// with kFileTooLarge and to deserialize into a String<> with kMaximumStringByteCountExceeded, and a std::string or a
// std::map name of 4 GiB fails to serialize with kMaximumStringByteCountExceeded. Each check takes 4 GiB of memory, so
// the test is skipped where size_t has 32 bits.

#include "TestCheck.h"

#include <cstdint>

#if defined( JSON4C4_ENABLE_STD_SUPPORT_INTERNAL ) && ( SIZE_MAX > 0xFFFFFFFFU )

#    include <map>
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

    Terathon::String<> copy;

    CheckStatus( data.DeserializeTo( copy ), Json::Status::kMaximumStringByteCountExceeded, "a string of 2 GiB is too long for a String<>" );
}

void TestLongStdString()
{
    Json::StructuredData data;
    std::string          text( size_t( 1 ) << 32, 'a' );

    CheckStatus( data.SerializeFrom( text ), Json::Status::kMaximumStringByteCountExceeded, "a std::string of 4 GiB is too long for a value" );

    std::map<std::string, double> map;
    map.emplace( static_cast<std::string&&>( text ), 1.0 );

    CheckStatus( data.SerializeFrom( map ), Json::Status::kMaximumStringByteCountExceeded, "a std::map name of 4 GiB is too long for a name" );
}

int main()
{
    TestLongRun();
    TestLongStdString();

    return TestResult();
}

#else

int main()
{
    return testSkippedReturnCode;
}

#endif
