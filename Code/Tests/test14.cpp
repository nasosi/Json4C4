#include "TestAllocation.h"
#include "TestCheck.h"

#include <cstdint>
#include <cstdlib>

namespace V3 = Json::V3;

struct Named
{
    Terathon::String<> s;
};

#define NAMED_PROTO "s", object.s
DEFINE_JSON4C4_FUNCTIONS( Named, NAMED_PROTO )

struct Point
{
    double x = 0;
    double y = 0;
};

#define POINT_PROTO "x", object.x, "y", object.y
DEFINE_JSON4C4_FUNCTIONS( Point, POINT_PROTO )

// Hands out blocks with malloc, refuses the requests from refusedFirst to refusedLast, and counts the blocks not handed
// back.
class CountingSource final : public V3::BlockSource
{
public:
    int64_t requestCount     = 0;
    int64_t refusedFirst     = -1;
    int64_t refusedLast      = -1;
    int     outstandingCount = 0;

    void* AllocateBlock( Terathon::uint64 size ) noexcept override
    {
        int64_t request = requestCount++;

        if ( ( request >= refusedFirst ) && ( request <= refusedLast ) )
        {
            return nullptr;
        }

        outstandingCount++;

        return malloc( size_t( size ) );
    }

    void FreeBlock( void* block, Terathon::uint64 ) noexcept override
    {
        outstandingCount--;
        free( block );
    }
};

void RefuseBlocks( CountingSource& source, int64_t first, int64_t last )
{
    source.requestCount = 0;
    source.refusedFirst = first;
    source.refusedLast  = last;
}

void StopRefusing( CountingSource& source )
{
    source.refusedFirst = -1;
    source.refusedLast  = -1;
}

#if defined( JSON4C4_TEST_ALLOCATION_HOOK ) && defined( JSON4C4_TEST_EXCEPTIONS )

void FailAllocationsAbove( size_t size )
{
    allocationFailAboveSize  = size;
    allocationFailCountdown  = -1;
    allocationFailureEnabled = true;
}

void StopFailing()
{
    allocationFailureEnabled = false;
}

void TestStringAssignment()
{
    const char* oldText = "an old string that lives on the heap";

    Named named;
    named.s = oldText;

    Terathon::String<> text = "{\"s\":\"";

    for ( int a = 0; a < 200; a++ )
    {
        text += "x";
    }

    text += "\"}";

    V3::StructuredData sd;
    CheckStatus( ParseText( sd, text ).status, Json::Status::kOk, "parse a long string" );

    FailAllocationsAbove( 150 );
    Json::Status status = sd.DeserializeTo( named );
    StopFailing();

    CheckStatus( status, Json::Status::kException, "deserialize a string when its allocation fails" );
    Check( named.s == oldText, "a failed string assignment keeps the old string" );

    named.s = "replaced";
    Check( named.s == "replaced", "the string is usable after a failed assignment" );
}

void TestArrayReservation()
{
    Terathon::Array<double> numbers;
    numbers.AppendArrayElement( 9.0 );

    Terathon::String<> text = "[";

    for ( int a = 0; a < 100; a++ )
    {
        text += "1,";
    }

    text += "2]";

    V3::StructuredData sd;
    CheckStatus( ParseText( sd, text ).status, Json::Status::kOk, "parse a long array" );

    FailAllocationsAbove( 400 );
    Json::Status status = sd.DeserializeTo( numbers );
    StopFailing();

    CheckStatus( status, Json::Status::kException, "deserialize an array when its reservation fails" );
    Check( numbers.GetArrayElementCount() == 0, "a failed array deserialize leaves an empty array" );

    numbers.AppendArrayElement( 1.0 );
    Check( numbers.GetArrayElementCount() == 1 && numbers[ 0 ] == 1.0, "the array is usable after a failed reservation" );

    CheckStatus( sd.DeserializeTo( numbers ), Json::Status::kOk, "deserialize the array again" );
    Check( numbers.GetArrayElementCount() == 101, "the array deserializes after a failed reservation" );
}

#endif

void TestSerializeFromFailure()
{
    CountingSource     source;
    V3::StructuredData sd( source );

    Point point;
    point.x = 1;
    point.y = 2;

    CheckStatus( sd.SerializeFrom( point ), Json::Status::kOk, "serialize a point" );

    point.x = 10;

    long failures = 0;

    for ( int64_t refused = 0; refused < 20; refused++ )
    {
        RefuseBlocks( source, refused, INT64_MAX );
        Json::Status status = sd.SerializeFrom( point );
        StopRefusing( source );

        if ( status == Json::Status::kOk )
        {
            break;
        }

        failures++;

        CheckStatus( status, Json::Status::kAllocationRefused, "SerializeFrom when a block is refused" );
        Check( ( sd.GetRoot() == nullptr ) && ( source.outstandingCount == 0 ), "a failed SerializeFrom leaves the document empty" );
    }

    Check( failures > 0, "SerializeFrom saw refused blocks" );

    Point roundTrip;
    CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "SerializeFrom succeeds when no block is refused" );
    Check( roundTrip.x == 10 && roundTrip.y == 2, "the new document's values" );
}

void TestAppendAfterFailure()
{
    CountingSource     source;
    V3::StructuredData sd( source );
    CheckStatus( ParseText( sd, "[1,2,3,4]" ).status, Json::Status::kOk, "parse an array" );

    const V3::Value& root      = *sd.GetRoot();
    long             failures  = 0;
    long             successes = 0;
    bool             unchanged = true;

    for ( int a = 0; a < 40; a++ )
    {
        Terathon::String<> before = WriteText( sd );

        RefuseBlocks( source, a % 3, INT64_MAX );
        Json::Status status = sd.AppendElement( root, double( a ) );
        StopRefusing( source );

        if ( status == Json::Status::kAllocationRefused )
        {
            failures++;
            unchanged = unchanged && ( strcmp( WriteText( sd ), before ) == 0 );
        }
        else
        {
            CheckStatus( status, Json::Status::kOk, "append to an array" );
            successes++;
        }
    }

    Check( failures > 0, "appending saw refused blocks" );
    Check( unchanged && ( long( root.AsArray().GetElementCount() ) == 4 + successes ), "failed appends leave the array unchanged" );

    CheckStatus( sd.Write( "Data/Test/out_test14_append.json" ), Json::Status::kOk, "an array that saw failed appends can be written" );

    V3::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test14_append.json" ).status, Json::Status::kOk, "reparse the array" );
}

// Refuses each block request of the parse in turn, and then every request from it on.
void TestParseFailures( const char* text, Json::Status expected = Json::Status::kOk, const Json::ParseOptions& options = Json::ParseOptions() )
{
    CountingSource     source;
    Terathon::String<> written;

    {
        V3::StructuredData sd( source );
        CheckStatus( ParseText( sd, text, options ).status, expected, "parse without refusals" );
        written = WriteText( sd );
    }

    int64_t requests = source.requestCount;
    long    failures = 0;

    for ( int64_t refused = 0; refused < 2 * requests; refused++ )
    {
        {
            V3::StructuredData sd( source );

            RefuseBlocks( source, refused % requests, ( refused < requests ) ? refused : INT64_MAX );
            Json::ParseResult parseResult = ParseText( sd, text, options );
            StopRefusing( source );

            Check( parseResult.status == Json::Status::kAllocationRefused || parseResult.status == expected, "a parse with a refused block reports it" );
            Check( ( parseResult.status == Json::Status::kOk ) == ( sd.GetRoot() != nullptr ), "a failed parse leaves no root" );
            Check( ( parseResult.status != Json::Status::kOk ) || ( strcmp( WriteText( sd ), written ) == 0 ),
                   "a parse that asks again at a smaller size gives the same document" );

            failures += ( parseResult.status == Json::Status::kAllocationRefused ) ? 1 : 0;
        }

        Check( source.outstandingCount == 0, "a parse with a refused block does not leak" );
    }

    Check( failures > 0, "parsing saw refused blocks" );
}

int main()
{
#if defined( JSON4C4_TEST_ALLOCATION_HOOK ) && defined( JSON4C4_TEST_EXCEPTIONS )
    TestStringAssignment();
    TestArrayReservation();
#endif

    TestSerializeFromFailure();
    TestAppendAfterFailure();
    TestParseFailures( "{\"a\":[1,2,3,{\"b\":\"a string that is longer than the local buffer\"}],\"c\":{\"d\":null,\"e\":true},\"f\":\"\\u00e9\"}" );

    // An object large enough for the name index, with names repeated among its first members and past them.
    Terathon::String<> large = "{";

    for ( int a = 0; a < 40; a++ )
    {
        char member[ 32 ];
        snprintf( member, sizeof( member ), "%s\"k%d\":[%d]", ( a != 0 ) ? "," : "", ( a % 13 == 12 ) ? a - 10 : a, a );
        large += member;
    }

    large += "}";

    Json::ParseOptions keepFirst;
    Json::ParseOptions keepLast;
    keepFirst.duplicateNames = Json::DuplicateNames::kKeepFirst;
    keepLast.duplicateNames  = Json::DuplicateNames::kKeepLast;

    TestParseFailures( large, Json::Status::kOk, keepFirst );
    TestParseFailures( large, Json::Status::kOk, keepLast );
    TestParseFailures( large, Json::Status::kDuplicateName );

    return TestResult();
}
