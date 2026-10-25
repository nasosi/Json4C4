#include "TestAllocation.h"
#include "TestCheck.h"

#if defined( JSON4C4_TEST_ALLOCATION_HOOK ) && defined( JSON4C4_TEST_EXCEPTIONS )

struct Named
{
    Terathon::String<> s;
};

#    define NAMED_PROTO "s", object.s
DEFINE_JSON4C4_FUNCTIONS( Named, NAMED_PROTO )

struct Point
{
    double x = 0;
    double y = 0;
};

#    define POINT_PROTO "x", object.x, "y", object.y
DEFINE_JSON4C4_FUNCTIONS( Point, POINT_PROTO )

void FailAllocationsAbove( size_t size )
{
    allocationFailAboveSize  = size;
    allocationFailCountdown  = -1;
    allocationFailureEnabled = true;
}

void FailAllocationNumber( long number )
{
    allocationFailAboveSize  = 0;
    allocationFailCountdown  = number;
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

    Json::StructuredData sd;
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

    Json::StructuredData sd;
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

void TestSerializeFromFailure()
{
    Json::StructuredData sd;

    Point point;
    point.x = 1;
    point.y = 2;

    CheckStatus( sd.SerializeFrom( point ), Json::Status::kOk, "serialize a point" );

    point.x = 10;

    long failures = 0;

    for ( long number = 1; number < 20; number++ )
    {
        FailAllocationNumber( number );
        Json::Status status = sd.SerializeFrom( point );
        StopFailing();

        if ( status == Json::Status::kOk )
        {
            break;
        }

        failures++;

        CheckStatus( status, Json::Status::kException, "SerializeFrom when an allocation fails" );

        Point roundTrip;
        CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "the old document survives a failed SerializeFrom" );
        Check( roundTrip.x == 1 && roundTrip.y == 2, "the old document's values survive a failed SerializeFrom" );
    }

    Check( failures > 0, "SerializeFrom saw allocation failures" );
}

void TestAppendAfterFailure()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "[1,2,3,4]" ).status, Json::Status::kOk, "parse an array" );

    Json::Value* root      = sd.GetRootJsonValue();
    long         failures  = 0;
    long         successes = 0;

    for ( int a = 0; a < 40; a++ )
    {
        FailAllocationNumber( 1 + a % 3 );
        Json::Status status = Json::Serialize( root, "", double( a ) );
        StopFailing();

        if ( status == Json::Status::kException )
        {
            failures++;
        }
        else
        {
            CheckStatus( status, Json::Status::kOk, "append to an array" );
            successes++;
        }
    }

    Check( failures > 0, "appending saw allocation failures" );

    const Terathon::Array<Json::Value*>* elements = root->GetDataAsPointerTo<Terathon::Array<Json::Value*>>();
    Check( elements->GetArrayElementCount() == 4 + successes, "failed appends leave the array unchanged" );

    CheckStatus( sd.Write( "Data/Test/out_test14_append.json" ), Json::Status::kOk, "an array that saw failed appends can be written" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test14_append.json" ).status, Json::Status::kOk, "reparse the array" );
}

void TestParseFailures()
{
    const char* text = "{\"a\":[1,2,3,{\"b\":\"a string that is longer than the local buffer\"}],\"c\":{\"d\":null,\"e\":true},\"f\":\"\\u00e9\"}";

    long allocationsBefore = allocationTotalCount;

    {
        Json::StructuredData sd;
        CheckStatus( ParseText( sd, text ).status, Json::Status::kOk, "parse without failures" );
    }

    long allocationsPerParse = allocationTotalCount - allocationsBefore;
    long liveBefore          = allocationLiveCount;
    long failures            = 0;

    for ( long number = 1; number <= allocationsPerParse; number++ )
    {
        Terathon::Array<char> buffer = MakeTextBuffer( text, strlen( text ) );

        {
            Json::StructuredData sd;

            FailAllocationNumber( number );
            Json::ParseResult parseResult = sd.Parse( buffer );
            StopFailing();

            Check( parseResult.status == Json::Status::kException || parseResult.status == Json::Status::kOk,
                   "a parse with a failing allocation reports an exception" );
            Check( ( parseResult.status == Json::Status::kOk ) == ( sd.GetRootJsonValue() != nullptr ), "a failed parse leaves no root" );

            failures += ( parseResult.status == Json::Status::kException ) ? 1 : 0;
        }

        buffer.PurgeArray();

        Check( allocationLiveCount == liveBefore, "a parse with a failing allocation does not leak" );
    }

    Check( failures > 0, "parsing saw allocation failures" );
}

int main()
{
    TestStringAssignment();
    TestArrayReservation();
    TestSerializeFromFailure();
    TestAppendAfterFailure();
    TestParseFailures();

    return TestResult();
}

#else

int main()
{
    fprintf( stdout, "Allocation failure checks need exceptions and a replaceable operator new; skipped\n" );

    return testSkippedReturnCode;
}

#endif
