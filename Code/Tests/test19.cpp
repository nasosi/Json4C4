#include "TestCheck.h"

#include <chrono>

namespace V3 = Json::V3;

V3::ObjectView Root( const V3::StructuredData& sd )
{
    return sd.GetRoot() ? sd.GetRoot()->AsObject() : V3::ObjectView();
}

// Names that share a long prefix and come in falling order, the order that is slowest to keep sorted one name at a time.
void TestManyNames()
{
    const int count = 200000;

    Terathon::Array<char> text;
    char                  member[ 128 ];

    Terathon::String<> prefix;

    for ( int a = 0; a < 64; a++ )
    {
        prefix += "p";
    }

    text.AppendArrayElement( '{' );

    for ( int a = 0; a < count; a++ )
    {
        int length = snprintf( member, sizeof( member ), "%s\"%s%06d\":%d", ( a != 0 ) ? "," : "", static_cast<const char*>( prefix ), count - a, a );

        for ( int b = 0; b < length; b++ )
        {
            text.AppendArrayElement( member[ b ] );
        }
    }

    text.AppendArrayElement( '}' );

    auto start = std::chrono::steady_clock::now();

    V3::StructuredData sd;
    CheckStatus( sd.ParseText( text.begin(), Terathon::uint64( text.GetArrayElementCount() ) ).status, Json::Status::kOk, "parse an object of many names" );

    V3::ObjectView root  = Root( sd );
    bool           found = root.GetMemberCount() == Terathon::uint32( count );

    for ( int a = 0; found && ( a < count ); a++ )
    {
        snprintf( member, sizeof( member ), "%s%06d", static_cast<const char*>( prefix ), count - a );
        found = root.FindNumber( member ) && ( *root.FindNumber( member ) == a );
    }

    double seconds = std::chrono::duration<double>( std::chrono::steady_clock::now() - start ).count();

    Check( found, "every one of many names is found" );
    Check( seconds < 30, "many names that share a prefix are parsed and found without quadratic work" );
    fprintf( stdout, "%d names parsed and found in %.3f s\n", count, seconds );

    // A repeat of the first name at the very end.
    snprintf( member, sizeof( member ), ",\"%s%06d\":0}", static_cast<const char*>( prefix ), count );
    text.RemoveLastArrayElement();

    int repeatColumn = text.GetArrayElementCount() + 2;

    for ( const char* c = member; *c; c++ )
    {
        text.AppendArrayElement( *c );
    }

    Json::ParseResult result = sd.ParseText( text.begin(), Terathon::uint64( text.GetArrayElementCount() ) );
    CheckStatus( result.status, Json::Status::kDuplicateName, "a repeat at the end of many names" );
    Check( ( result.errorLine == 1 ) && ( result.errorColumn == repeatColumn ), "the repeat at the end is reported where it is" );
}

struct LargeObject
{
    Terathon::String<> text;
    int                nameColumn[ 40 ];
};

// Forty members k0 to k39 with values 0 to 39, except that member 22 is named k5 and members 30 and 36 are named k25,
// and member 10 is named k3 when early is set.
LargeObject MakeLargeObject( bool early = false )
{
    LargeObject object;
    char        member[ 32 ];

    object.text = "{";

    for ( int a = 0; a < 40; a++ )
    {
        int name = ( a == 22 ) ? 5 : ( ( a == 30 ) || ( a == 36 ) ) ? 25 : ( early && ( a == 10 ) ) ? 3 : a;

        if ( a != 0 )
        {
            object.text += ",";
        }

        object.nameColumn[ a ] = object.text.GetStringLength() + 1;

        snprintf( member, sizeof( member ), "\"k%d\":%d", name, a );
        object.text += member;
    }

    object.text += "}";

    return object;
}

void CheckMembers( V3::ObjectView root, double k5, double k25, const char* description )
{
    static const int removed[] = { 22, 30, 36 };

    bool ok       = root.GetMemberCount() == 37;
    int  position = 0;
    char name[ 16 ];

    for ( int a = 0; ok && ( a < 40 ); a++ )
    {
        if ( ( a == removed[ 0 ] ) || ( a == removed[ 1 ] ) || ( a == removed[ 2 ] ) )
        {
            continue;
        }

        snprintf( name, sizeof( name ), "k%d", a );

        double expected = ( a == 5 ) ? k5 : ( a == 25 ) ? k25 : a;

        ok = ( root.begin()[ position++ ].GetName() == name ) && root.FindNumber( name ) && ( *root.FindNumber( name ) == expected );
    }

    Check( ok, description );
}

void TestRepeatedNames()
{
    LargeObject object = MakeLargeObject();

    V3::StructuredData sd;
    Json::ParseResult  result = ParseText( sd, object.text );

    CheckStatus( result.status, Json::Status::kDuplicateName, "repeated names in a large object are rejected" );
    Check( ( result.errorLine == 1 ) && ( result.errorColumn == object.nameColumn[ 22 ] ), "the earliest repeat is reported" );

    Json::ParseOptions options;

    options.duplicateNames = Json::DuplicateNames::kKeepFirst;
    CheckStatus( ParseText( sd, object.text, options ).status, Json::Status::kOk, "keep the first of repeated names in a large object" );
    CheckMembers( Root( sd ), 5, 25, "the first values of repeated names stay, in document order" );

    options.duplicateNames = Json::DuplicateNames::kKeepLast;
    CheckStatus( ParseText( sd, object.text, options ).status, Json::Status::kOk, "keep the last of repeated names in a large object" );
    CheckMembers( Root( sd ), 22, 36, "the last values of repeated names take the first ones' places" );

    // A repeat among the first members is reported before one past them.
    LargeObject early = MakeLargeObject( true );

    result = ParseText( sd, early.text );
    CheckStatus( result.status, Json::Status::kDuplicateName, "a repeat among the first members of a large object" );
    Check( result.errorColumn == early.nameColumn[ 10 ], "a repeat among the first members is reported first" );
}

// Appends a member to an object being written and returns the column of its name.
int AppendMember( Terathon::String<>& text, const char* name, const char* value )
{
    if ( text.GetStringLength() > 1 )
    {
        text += ",";
    }

    int column = text.GetStringLength() + 1;

    text += "\"";
    text += name;
    text += "\":";
    text += value;

    return column;
}

// Members named prefix0 to prefix(count - 1) with their positions as values, then a repeat of repeatedName if given.
Terathon::String<> MakeMembers( const char* prefix, int count, const char* repeatedName, int* repeatColumn, int columnBase )
{
    Terathon::String<> text = "{";
    char               name[ 16 ];
    char               value[ 16 ];

    for ( int a = 0; a < count; a++ )
    {
        snprintf( name, sizeof( name ), "%s%d", prefix, a );
        snprintf( value, sizeof( value ), "%d", a );
        AppendMember( text, name, value );
    }

    if ( repeatedName )
    {
        snprintf( value, sizeof( value ), "%d", count );
        *repeatColumn = columnBase + AppendMember( text, repeatedName, value ) - 1;
    }

    return text;
}

// Large objects inside large objects, each settling its own repeated names when it closes.
void TestNestedLargeObjects()
{
    V3::StructuredData sd;
    int                repeatColumn = 0;

    // The outer object repeats o0 after a large inner object.
    Terathon::String<> outer = MakeMembers( "o", 17, nullptr, nullptr, 0 );
    Terathon::String<> inner = MakeMembers( "i", 30, nullptr, nullptr, 0 ) + "}";
    AppendMember( outer, "o17", inner );
    AppendMember( outer, "o18", "18" );
    repeatColumn = AppendMember( outer, "o0", "19" );
    outer += "}";

    Json::ParseResult result = ParseText( sd, outer );
    CheckStatus( result.status, Json::Status::kDuplicateName, "a repeat after a large inner object" );
    Check( result.errorColumn == repeatColumn, "a repeat after a large inner object is reported where it is" );

    // The outer object repeats o1 before an inner object that repeats i3, and the inner object closes first.
    outer = MakeMembers( "o", 17, nullptr, nullptr, 0 );
    AppendMember( outer, "o1", "17" );
    int innerColumn = AppendMember( outer, "o18", "" ) + 6;
    outer += MakeMembers( "i", 20, "i3", &repeatColumn, innerColumn );
    outer += "}}";

    result = ParseText( sd, outer );
    CheckStatus( result.status, Json::Status::kDuplicateName, "repeats in an outer and an inner large object" );
    Check( result.errorColumn == repeatColumn, "the repeat of the inner object, which closes first, is reported" );

    Json::ParseOptions options;
    options.duplicateNames = Json::DuplicateNames::kKeepLast;
    CheckStatus( ParseText( sd, outer, options ).status, Json::Status::kOk, "keep the last of repeats in nested large objects" );

    V3::ObjectView root        = Root( sd );
    V3::ObjectView innerObject = root.FindObject( "o18" );

    Check( ( root.GetMemberCount() == 18 ) && root.FindNumber( "o1" ) && ( *root.FindNumber( "o1" ) == 17 ),
           "the outer object keeps the last value of its repeat" );
    Check( ( innerObject.GetMemberCount() == 20 ) && innerObject.FindNumber( "i3" ) && ( *innerObject.FindNumber( "i3" ) == 20 ) &&
               innerObject.FindNumber( "i19" ),
           "the inner object keeps the last value of its repeat" );
}

void TestBuiltByHand()
{
    const int count = 100;
    char      name[ 16 ];

    V3::StructuredData sd;
    CheckStatus( ParseText( sd, "{}" ).status, Json::Status::kOk, "parse an empty object" );

    const V3::Value& root   = *sd.GetRoot();
    V3::ObjectView   object = root.AsObject();

    for ( int a = 0; a < count; a++ )
    {
        int key = ( a * 37 ) % count;

        snprintf( name, sizeof( name ), ( key % 3 == 0 ) ? "\xC3\xA9%d" : "m%d", key );
        CheckStatus( sd.AddMember( root, name, double( key ) ), Json::Status::kOk, "insert a member by hand" );
    }

    bool found = object.GetMemberCount() == Terathon::uint32( count );

    for ( int a = 0; a < count; a++ )
    {
        int key = ( a * 37 ) % count;

        snprintf( name, sizeof( name ), ( key % 3 == 0 ) ? "\xC3\xA9%d" : "m%d", key );
        found = found && ( object.begin()[ a ].GetName() == name ) && object.FindNumber( name ) && ( *object.FindNumber( name ) == key );
    }

    Check( found, "members inserted by hand are found and keep their order" );

    CheckStatus( sd.AddMember( root, "m50", 0.0 ), Json::Status::kDuplicateName, "a repeated name inserted by hand is refused" );

    for ( int key = 1; key < count; key += 3 )
    {
        snprintf( name, sizeof( name ), "m%d", key );

        if ( key % 2 )
        {
            CheckStatus( sd.RemoveMember( root, name ), Json::Status::kOk, "remove a member by hand" );
        }
        else
        {
            CheckStatus( sd.RemoveMember( root, name, Terathon::uint32( strlen( name ) ) ), Json::Status::kOk, "remove a member by hand with its length" );
        }
    }

    found = object.GetMemberCount() == Terathon::uint32( count - 33 );

    for ( int key = 0; key < count; key++ )
    {
        snprintf( name, sizeof( name ), ( key % 3 == 0 ) ? "\xC3\xA9%d" : "m%d", key );
        found = found && ( ( key % 3 == 1 ) ? !object.Find( name ) : ( object.FindNumber( name ) && ( *object.FindNumber( name ) == key ) ) );
    }

    Check( found, "members removed by hand are gone and the others are still found" );
}

int main()
{
    TestManyNames();
    TestRepeatedNames();
    TestNestedLargeObjects();
    TestBuiltByHand();

    return TestResult();
}
