// The 3.0 document: it parses every text as 2.x does and writes the same text, it takes all of its memory from its block
// source and hands all of it back, and lookups and iteration allocate nothing.

#include "TestCheck.h"

#include <cstdint>
#include <cstdlib>

namespace V3 = Json::V3;

using Terathon::uint32;
using Terathon::uint64;

bool    countingAllocations = false;
int64_t allocationCount     = 0;

void* operator new( size_t size )
{
    if ( countingAllocations )
    {
        allocationCount++;
    }

    void* memory = malloc( ( size != 0 ) ? size : 1 );
    if ( !memory )
    {
        abort();
    }

    return memory;
}

void* operator new[]( size_t size )
{
    return operator new( size );
}

void operator delete( void* memory ) noexcept
{
    free( memory );
}

void operator delete[]( void* memory ) noexcept
{
    free( memory );
}

void operator delete( void* memory, size_t ) noexcept
{
    free( memory );
}

void operator delete[]( void* memory, size_t ) noexcept
{
    free( memory );
}

// Hands out blocks with malloc, can refuse one request, and checks that every block comes back once with its size.
class CountingSource final : public V3::BlockSource
{
public:
    int64_t requestCount     = 0;
    int64_t refusedRequest   = -1;
    int64_t outstandingBytes = 0;
    bool    misused          = false;

    void* AllocateBlock( uint64 size ) noexcept override
    {
        if ( ( size % 8 ) != 0 )
        {
            misused = true;
        }

        if ( requestCount++ == refusedRequest )
        {
            return nullptr;
        }

        void* block = malloc( size_t( size ) );

        if ( ( blockCount == maxBlockCount ) || ( ( uintptr_t( block ) % 8 ) != 0 ) )
        {
            misused = true;
        }
        else
        {
            blocks[ blockCount++ ] = Block { block, size };
        }

        outstandingBytes += int64_t( size );

        return block;
    }

    void FreeBlock( void* block, uint64 size ) noexcept override
    {
        int a = 0;

        while ( ( a < blockCount ) && ( blocks[ a ].block != block ) )
        {
            a++;
        }

        if ( ( a == blockCount ) || ( blocks[ a ].size != size ) )
        {
            misused = true;
            return;
        }

        blocks[ a ] = blocks[ --blockCount ];
        outstandingBytes -= int64_t( size );
        free( block );
    }

    bool AllReturned() const
    {
        return ( outstandingBytes == 0 ) && ( blockCount == 0 ) && !misused;
    }

private:
    struct Block
    {
        void*  block;
        uint64 size;
    };

    static constexpr int maxBlockCount = 4096;

    Block blocks[ maxBlockCount ];
    int   blockCount = 0;
};

CountingSource countingSource;

bool SameText( const Terathon::Array<char>& first, const Terathon::Array<char>& second )
{
    return ( first.GetArrayElementCount() == second.GetArrayElementCount() ) &&
           ( memcmp( first.begin(), second.begin(), size_t( first.GetArrayElementCount() ) ) == 0 );
}

int shownMismatches = 0;

// Parses the first length characters of text from an exact-size heap buffer with 2.x and with 3.0, and checks that both
// give the same status and position, that both write the same text, and that a failed parse keeps no block.
void CheckSameAs2x( const char* text, size_t length, const char* description, const Json::ParseOptions& options = Json::ParseOptions() )
{
    char* exact = static_cast<char*>( malloc( ( length != 0 ) ? length : 1 ) );
    memcpy( exact, text, length );

    Json::StructuredData oldData;
    Json::ParseResult    oldResult = oldData.ParseText( exact, length, options );
    Json::ParseResult    newResult;
    bool                 same;

    {
        V3::StructuredData data( countingSource );
        newResult = data.ParseText( exact, length, options );

        same = ( oldResult.status == newResult.status ) && ( oldResult.errorLine == newResult.errorLine ) && ( oldResult.errorColumn == newResult.errorColumn );

        if ( same && ( newResult.status == Json::Status::kOk ) )
        {
            Terathon::Array<char> oldText;
            Terathon::Array<char> newText;
            Terathon::Array<char> indentation;

            oldData.GetRootJsonValue()->Write( oldText, 2, ' ', indentation );
            same = ( data.Write( newText ) == Json::Status::kOk ) && SameText( oldText, newText );
        }
        else if ( newResult.status != Json::Status::kOk )
        {
            same = same && ( data.GetRoot() == nullptr ) && ( countingSource.outstandingBytes == 0 );
        }
    }

    same = same && countingSource.AllReturned();

    if ( !same && ( shownMismatches++ < 10 ) )
    {
        fprintf( stderr,
                 "%s, length %u: 2.x status %u at %d:%d, 3.0 status %u at %d:%d\n",
                 description,
                 unsigned( length ),
                 unsigned( oldResult.status ),
                 int( oldResult.errorLine ),
                 int( oldResult.errorColumn ),
                 unsigned( newResult.status ),
                 int( newResult.errorLine ),
                 int( newResult.errorColumn ) );
    }

    Check( same, description );

    free( exact );
}

void CheckEveryPrefix( const char* text, size_t length, const char* description )
{
    for ( size_t a = 0; a <= length; a++ )
    {
        CheckSameAs2x( text, a, description );
    }
}

void TestDataFiles()
{
    static const char* const files[] = {
        "Data/Examples/car.json",
        "Data/Examples/simple.json",
        "Data/Examples/simple_error.json",
        "Data/Examples/simple_with_array.json",
        "Data/Test/test_file01.json",
        "Data/Test/test_file02.json",
        "Data/Test/test_file04.json",
        "Data/Test/jsonchecker/pass1.json",
        "Data/Test/jsonchecker/pass2.json",
        "Data/Test/jsonchecker/pass3.json",
        "Data/Test/jsonchecker/fail1_PASSES_RFC_8259.json",
        "Data/Test/jsonchecker/fail18_PASSES_RFC_8259.json",
    };

    for ( const char* file : files )
    {
        Terathon::String<> text;
        Check( ReadTextFile( file, text ), file );
        CheckEveryPrefix( text, size_t( text.GetStringLength() ), file );
    }

    char name[ 64 ];

    for ( int a = 2; a <= 33; a++ )
    {
        snprintf( name, sizeof( name ), "Data/Test/jsonchecker/fail%d.json", a );

        Terathon::String<> text;

        if ( ( a != 18 ) && ReadTextFile( name, text ) )
        {
            CheckEveryPrefix( text, size_t( text.GetStringLength() ), name );
        }
    }
}

void TestTokensAtTheEnd()
{
    static const char* const tokens[] = { "true",
                                          "false",
                                          "null",
                                          "\"abc\"",
                                          "\"\\n\\/\\\"\"",
                                          "\"\\u00e9\"",
                                          "\"\\ud83d\\ude00\"",
                                          "\"\xC3\xA9\"",
                                          "\"\xE2\x82\xAC\"",
                                          "\"\xF0\x9F\x98\x80\"",
                                          "-12.5e+3",
                                          "0",
                                          "[1,2]",
                                          "{\"a\":1}",
                                          "  \r\n  1  \r" };

    static const char* const followers[] = { "e", "0", "\"", "\\", "u0041\"", "\\ude00\"", "\x80\x80\x80", "}", "]", "\n", "x" };

    for ( const char* token : tokens )
    {
        for ( const char* follower : followers )
        {
            Terathon::String<> text = token;
            text += follower;

            for ( size_t a = 0; a <= strlen( token ); a++ )
            {
                CheckSameAs2x( text, a, token );
            }
        }
    }
}

void TestLongTails()
{
    Terathon::String<> letters;
    Terathon::String<> spaces;

    for ( int a = 0; a < 5000; a++ )
    {
        letters += "a";
        spaces += " ";
    }

    Terathon::String<> texts[] = { Terathon::String<>( "[\"" ) + letters,
                                   Terathon::String<>( "[\"x\",\"" ) + letters,
                                   Terathon::String<>( "[\"" ) + letters + "\"]",
                                   Terathon::String<>( "[\"" ) + letters + "\\u00e9",
                                   Terathon::String<>( "[1]" ) + spaces,
                                   spaces + "[1]",
                                   spaces,
                                   Terathon::String<>( "[1," ) + spaces };

    for ( const Terathon::String<>& text : texts )
    {
        CheckSameAs2x( text, size_t( text.GetStringLength() ), "a text with a long tail" );
    }
}

// Forty members k0 to k39, except that member 22 is named k5, members 30 and 36 are named k25, and with early set member
// 10 is named k3. Member 12 holds a large object that repeats i3.
Terathon::String<> MakeRepeatingObject( bool early )
{
    Terathon::String<> text = "{";
    char               member[ 64 ];

    for ( int a = 0; a < 40; a++ )
    {
        int name = ( a == 22 ) ? 5 : ( ( a == 30 ) || ( a == 36 ) ) ? 25 : ( early && ( a == 10 ) ) ? 3 : a;

        snprintf( member, sizeof( member ), "%s\"k%d\":%d", ( a != 0 ) ? "," : "", name, a );
        text += member;

        if ( a == 12 )
        {
            text += ",\"inner\":{";

            for ( int b = 0; b < 20; b++ )
            {
                snprintf( member, sizeof( member ), "%s\"i%d\":%d", ( b != 0 ) ? "," : "", ( b == 19 ) ? 3 : b, b );
                text += member;
            }

            text += "}";
        }
    }

    return text + "}";
}

void TestDuplicateNames()
{
    static const Json::DuplicateNames modes[] = { Json::DuplicateNames::kReject, Json::DuplicateNames::kKeepFirst, Json::DuplicateNames::kKeepLast };

    for ( Json::DuplicateNames mode : modes )
    {
        Json::ParseOptions options;
        options.duplicateNames = mode;

        for ( int early = 0; early < 2; early++ )
        {
            Terathon::String<> text = MakeRepeatingObject( early != 0 );
            CheckSameAs2x( text, size_t( text.GetStringLength() ), "repeated names in large objects", options );
        }

        const char* small  = "{\"a\":1,\"b\":2,\"a\":3}";
        const char* nested = "[{\"a\":1,\"a\":{\"b\":1,\"b\":2}}]";

        CheckSameAs2x( small, strlen( small ), "repeated names in a small object", options );
        CheckSameAs2x( nested, strlen( nested ), "repeated names in nested small objects", options );

        // 300 names from 0 to 99, so that every name repeats, with large and small objects inside.
        Terathon::String<> many = "{";
        char               member[ 64 ];

        for ( int a = 0; a < 300; a++ )
        {
            snprintf( member, sizeof( member ), "%s\"n%d\":%s", ( a != 0 ) ? "," : "", ( a * 37 ) % 100, ( a % 50 == 0 ) ? "{\"x\":[1,{\"y\":2}]}" : "true" );
            many += member;
        }

        many += "}";
        CheckSameAs2x( many, size_t( many.GetStringLength() ), "many repeated names", options );
    }
}

void TestLimits()
{
    const char* text = "{\"ab\":[1,\"cd\",{\"e\":\"\\u00e9\",\"f\":[[]]}],\"g\":null}";

    for ( int a = -1; a <= 12; a++ )
    {
        Json::ParseOptions options;

        options.maxValueCount = a;
        CheckSameAs2x( text, strlen( text ), "a value limit", options );

        options                    = Json::ParseOptions();
        options.maxStringByteCount = a;
        CheckSameAs2x( text, strlen( text ), "a string byte limit", options );

        options                 = Json::ParseOptions();
        options.maxNestingDepth = a;
        CheckSameAs2x( text, strlen( text ), "a nesting limit", options );
    }
}

// Strings and names may hold zeros, written as \u0000. A zero byte in the text is still refused.
void TestZeros()
{
    const char         text[] = "{\"s\":\"a\\u0000b\",\"a\\u0000b\":1,\"a\\u0000c\":2,\"a\":3}";
    V3::StructuredData data( countingSource );

    CheckStatus( data.ParseText( text, sizeof( text ) - 1 ).status, Json::Status::kOk, "parse strings and names with zeros" );

    V3::ObjectView object = data.GetRoot() ? data.GetRoot()->AsObject() : V3::ObjectView();
    V3::StringView string = object.FindString( "s" );

    Check( ( string.GetLength() == 3 ) && ( string.GetText()[ 1 ] == 0 ) && ( string.GetText()[ 2 ] == 'b' ), "a string keeps its zero" );
    Check( ( object.GetMemberCount() == 4 ) && object.FindNumber( "a" ) && ( *object.FindNumber( "a" ) == 3 ), "names differing after a zero are distinct" );
    Check( object.Find( "a\0c", 3 ) && ( *object.Find( "a\0c", 3 )->AsNumber() == 2 ), "a name with a zero is found by its length" );
    Check( !object.Find( "a\0", 2 ), "a name is not found by a part of it" );

    Terathon::Array<char> written;
    Check( data.Write( written, 0 ) == Json::Status::kOk, "write strings with zeros" );

    const char expected[] = "{\n\"s\" : \"a\\u0000b\",\n\"a\\u0000b\" : 1,\n\"a\\u0000c\" : 2,\n\"a\" : 3\n}";
    Check( ( written.GetArrayElementCount() == int( sizeof( expected ) - 1 ) ) && ( memcmp( written.begin(), expected, sizeof( expected ) - 1 ) == 0 ),
           "a zero is written as \\u0000" );

    const char* repeat = "{\"a\\u0000\":1,\"a\\u0000\":2}";
    CheckStatus( data.ParseText( repeat, strlen( repeat ) ).status, Json::Status::kDuplicateName, "a repeated name with a zero" );
    CheckStatus( data.ParseText( "[\"a\0b\"]", 7 ).status, Json::Status::kStringIllegalChar, "a zero byte in a string" );

    Terathon::String<> large = "{";
    char               member[ 64 ];

    for ( int a = 0; a < 40; a++ )
    {
        snprintf( member, sizeof( member ), "%s\"k\\u0000%d\":%d", ( a != 0 ) ? "," : "", a, a );
        large += member;
    }

    Terathon::String<> repeated = large + ",\"k\\u00007\":0}";
    Terathon::String<> distinct = large + ",\"k\\u000080\":0}";

    CheckStatus( data.ParseText( repeated, uint64( repeated.GetStringLength() ) ).status,
                 Json::Status::kDuplicateName,
                 "a repeated name with a zero in a large object" );
    CheckStatus( data.ParseText( distinct, uint64( distinct.GetStringLength() ) ).status, Json::Status::kOk, "a large object of names with zeros" );

    object = data.GetRoot() ? data.GetRoot()->AsObject() : V3::ObjectView();
    Check( object.Find( "k\0"
                        "39",
                        4 ) &&
               object.Find( "k\0"
                            "80",
                            4 ) &&
               !object.Find( "k", 1 ),
           "names with zeros in a large object are found" );
}

void TestViews()
{
    Terathon::String<> text = "{\"list\":[1,\"two\",true,null,[],{}],\"empty\":\"\"";
    char               member[ 64 ];

    for ( int a = 0; a < 100; a++ )
    {
        snprintf( member, sizeof( member ), ",\"k%d\":%d", a, a );
        text += member;
    }

    text += "}";

    V3::StructuredData data( countingSource );
    CheckStatus( data.ParseText( text, uint64( text.GetStringLength() ) ).status, Json::Status::kOk, "parse a document for the views" );

    const V3::Value* root   = data.GetRoot();
    V3::ObjectView   object = root ? root->AsObject() : V3::ObjectView();

    Check( object && ( object.GetMemberCount() == 102 ) && !root->AsArray() && !root->AsString() && !root->AsNumber(), "the root is an object" );

    bool found = true;

    for ( int a = 0; a < 100; a++ )
    {
        snprintf( member, sizeof( member ), "k%d", a );

        const double* number = object.FindNumber( member );
        found                = found && number && ( *number == a ) && ( object.begin()[ a + 2 ].GetName() == member );
    }

    Check( found, "every name of a large object is found, in document order" );
    Check( !object.Find( "k" ) && !object.Find( "k100" ) && !object.Find( "" ) && !object.Find( nullptr ) && !object.Find( "k1", 1 ),
           "absent names are not found" );
    Check( !object.FindString( "k1" ) && !object.FindArray( "k1" ) && !object.FindObject( "list" ) && !object.FindBool( "k1" ) && !object.FindNull( "k1" ),
           "a value of another type is not found" );

    V3::StringView empty = object.FindString( "empty" );
    Check( empty && ( empty.GetLength() == 0 ) && ( empty == "" ) && ( empty != "x" ), "an empty string is present" );

    V3::ArrayView list = object.FindArray( "list" );
    Check( list && ( list.GetElementCount() == 6 ) && ( *list[ 0 ].AsNumber() == 1 ) && ( list[ 1 ].AsString() == "two" ) && *list[ 2 ].AsBool() &&
               list[ 3 ].IsNull() && list[ 4 ].AsArray() && ( list[ 4 ].AsArray().GetElementCount() == 0 ) && list[ 5 ].AsObject() &&
               ( list[ 5 ].AsObject().GetMemberCount() == 0 ),
           "array elements" );

    int count = 0;

    for ( const V3::Value& element : list )
    {
        count += ( element.GetType() == V3::Type::kNull ) ? 10 : 1;
    }

    Check( count == 15, "iterate over an array" );
    Check( !V3::ArrayView() && ( V3::ArrayView().GetElementCount() == 0 ) && !V3::ObjectView().Find( "a" ) && !V3::StringView(), "empty views" );
    Check( ( V3::StringView( "ab", 2 ) == V3::StringView( "abc", 2 ) ) && ( V3::StringView( "ab", 2 ) != "abc" ) && ( V3::StringView( "ab", 2 ) != "a" ),
           "compare strings" );
}

// Lookups and iteration ask the block source for nothing, and a parse makes no other allocation.
void TestNoOtherAllocations()
{
    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/jsonchecker/pass1.json", text ), "read pass1.json" );

    V3::StructuredData data( countingSource );

    countingAllocations = true;
    allocationCount     = 0;

    Json::Status status = data.ParseText( text, uint64( text.GetStringLength() ) ).status;

    int64_t requests = countingSource.requestCount;
    int     visited  = 0;

    const V3::Value* root = data.GetRoot();

    for ( const V3::Value& element : root ? root->AsArray() : V3::ArrayView() )
    {
        for ( const V3::Member& member : element.AsObject() )
        {
            visited += element.AsObject().Find( member.GetName() ) ? 1 : 0;
        }
    }

    countingAllocations = false;

    CheckStatus( status, Json::Status::kOk, "parse pass1.json" );
    Check( visited > 20, "visit the members of pass1.json" );
    Check( allocationCount == 0, "a parse, lookups and iteration make no allocation outside the block source" );
    Check( countingSource.requestCount == requests, "lookups and iteration ask the block source for nothing" );
}

// Each request of a parse refused in turn fails the parse and leaves no block behind.
void TestRefusedBlocks()
{
    Terathon::String<> text = "{\"numbers\":[";
    char               member[ 64 ];

    for ( int a = 0; a < 5000; a++ )
    {
        snprintf( member, sizeof( member ), "%s%d", ( a != 0 ) ? "," : "", a );
        text += member;
    }

    text += "],\"long\":\"";

    for ( int a = 0; a < 500; a++ )
    {
        text += "\\u00e9";
    }

    text += "\"";

    for ( int a = 0; a < 40; a++ )
    {
        snprintf( member, sizeof( member ), ",\"k%d\":[%d,{\"x\":\"y\"}]", a, a );
        text += member;
    }

    text += "}";

    int64_t requests;

    {
        V3::StructuredData data( countingSource );

        countingSource.requestCount = 0;
        CheckStatus( data.ParseText( text, uint64( text.GetStringLength() ) ).status, Json::Status::kOk, "parse a document of several blocks" );
        requests = countingSource.requestCount;

        Check( requests >= 4, "the document and the stack grow" );
        Check( countingSource.outstandingBytes > 0, "the document keeps its blocks" );

        data.Clear();
        Check( countingSource.AllReturned() && !data.GetRoot(), "Clear hands every block back" );
    }

    bool failedClosed = true;

    for ( int64_t refused = 0; refused < requests; refused++ )
    {
        V3::StructuredData data( countingSource );

        countingSource.requestCount   = 0;
        countingSource.refusedRequest = refused;

        Json::ParseResult result = data.ParseText( text, uint64( text.GetStringLength() ) );

        failedClosed = failedClosed && ( result.status == Json::Status::kAllocationRefused ) && !data.GetRoot() && countingSource.AllReturned();
    }

    countingSource.refusedRequest = -1;

    Check( failedClosed, "every refused request fails the parse and leaves no block" );

    {
        V3::StructuredData data( countingSource );
        CheckStatus( data.ParseText( text, uint64( text.GetStringLength() ) ).status, Json::Status::kOk, "parse again" );
    }

    Check( countingSource.AllReturned(), "the destructor hands every block back" );
}

void TestFiles()
{
    {
        V3::StructuredData   data( countingSource );
        Json::StructuredData oldData;

        CheckStatus( data.Parse( "Data/Examples/car.json" ).status, Json::Status::kOk, "parse a file" );
        CheckStatus( data.Write( "test21_car.json", 4, ' ' ), Json::Status::kOk, "write a file" );
        CheckStatus( oldData.Parse( "test21_car.json" ).status, Json::Status::kOk, "parse the written file with 2.x" );

        Terathon::Array<char> oldText;
        Terathon::Array<char> newText;
        Terathon::Array<char> indentation;

        oldData.GetRootJsonValue()->Write( oldText, 4, ' ', indentation );
        data.Write( newText, 4, ' ' );
        Check( SameText( oldText, newText ), "the written file reads back the same" );

        CheckStatus( data.Parse( "Data/Examples/missing.json" ).status, Json::Status::kFileOpenError, "a missing file" );
        Check( !data.GetRoot(), "a failed parse clears the document" );

        Terathon::Array<char> output;
        CheckStatus( data.Write( output ), Json::Status::kInvalidStructuredData, "write an empty document" );

        Terathon::Array<char> buffer = MakeTextBuffer( "[1,2]", 5 );
        CheckStatus( data.Parse( buffer ).status, Json::Status::kOk, "parse a buffer" );
        CheckStatus( data.ParseText( "[1]", Terathon::uint64( 0x7FFFFFFFU ) ).status, Json::Status::kFileTooLarge, "a text longer than the limit" );
        CheckStatus( data.ParseText( nullptr, 3 ).status, Json::Status::kTextBufferContentsInvalid, "no text" );
    }

    remove( "test21_car.json" );

    Check( countingSource.AllReturned(), "reading a file hands its blocks back" );
}

int main()
{
    TestDataFiles();
    TestTokensAtTheEnd();
    TestLongTails();
    TestDuplicateNames();
    TestLimits();
    TestZeros();
    TestViews();
    TestNoOtherAllocations();
    TestRefusedBlocks();
    TestFiles();

    return TestResult();
}
