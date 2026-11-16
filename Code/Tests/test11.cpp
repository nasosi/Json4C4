#include "TestCheck.h"

#include <cstdlib>

// Hands out blocks with malloc and counts the blocks not handed back.
class CountingSource final : public Json::BlockSource
{
public:
    int outstandingCount = 0;

    void* AllocateBlock( Terathon::uint64 size ) noexcept override
    {
        outstandingCount++;

        return malloc( size_t( size ) );
    }

    void FreeBlock( void* block, Terathon::uint64 ) noexcept override
    {
        outstandingCount--;
        free( block );
    }
};

struct Repeated
{
    double x = 1;
    double y = 2;
};

#define REPEATED_PROTO "a", object.x, "a", object.y
DEFINE_JSON4C4_FUNCTIONS( Repeated, REPEATED_PROTO )

int CountOccurrences( const char* text, const char* pattern )
{
    int count = 0;

    for ( const char* found = strstr( text, pattern ); found; found = strstr( found + 1, pattern ) )
    {
        count++;
    }

    return count;
}

const char* const duplicateDocument = "{\"a\":1,\"a\":[2,3,4],\"b\":2,\"a\":{\"c\":3}}";

void TestDuplicateKeysRejected()
{
    Json::StructuredData sd;

    Json::ParseResult parseResult = ParseText( sd, duplicateDocument );
    CheckStatus( parseResult.status, Json::Status::kDuplicateName, "duplicate keys are rejected by default" );
    Check( parseResult.errorLine == 1 && parseResult.errorColumn == 8, "the error points at the repeated name" );
    Check( sd.GetRoot() == nullptr, "a rejected document leaves no root" );

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":{\"a\":2}}" ).status, Json::Status::kOk, "the same name in different objects is not a duplicate" );
}

void TestDuplicateKeysKept( Json::DuplicateNames policy, double expected, const char* description )
{
    Json::ParseOptions options;
    options.duplicateNames = policy;

    Json::StructuredData sd;
    CheckStatus( ParseText( sd, duplicateDocument, options ).status, Json::Status::kOk, description );

    Json::ObjectView root = sd.GetRoot() ? sd.GetRoot()->AsObject() : Json::ObjectView();
    Check( root.GetMemberCount() == 2, "duplicate keys are stored once" );

    if ( expected != 0 )
    {
        Check( root.FindNumber( "a" ) && *root.FindNumber( "a" ) == expected, description );
    }
    else
    {
        Check( root.FindObject( "a" ).FindNumber( "c" ) != nullptr, description );
    }

    CheckStatus( sd.Write( "Data/Test/out_test11_duplicates.json" ), Json::Status::kOk, "write a document that had duplicate keys" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test11_duplicates.json", text ) && CountOccurrences( text, "\"a\"" ) == 1, "a duplicate key is written once" );
    Check( strstr( text, "\"a\"" ) < strstr( text, "\"b\"" ), "a duplicate key keeps the position of its first occurrence" );

    CountingSource source;

    for ( int a = 0; a < 100; a++ )
    {
        Json::StructuredData repeated( source );
        ParseText( repeated, "{\"a\":1,\"a\":[2,3,4],\"a\":{\"b\":[5]}}", options );
    }

    Check( source.outstandingCount == 0, "duplicate keys do not leak" );
}

void TestOrderSurvivesDirectChanges()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":2,\"c\":3}" ).status, Json::Status::kOk, "parse three members" );

    const Json::Value& root   = *sd.GetRoot();
    Json::EditHandle   edited = sd.Edit( root );

    CheckStatus( sd.RemoveMember( edited, "a" ), Json::Status::kOk, "remove the first member" );
    CheckStatus( sd.RemoveMember( edited, "c" ), Json::Status::kOk, "remove the last member" );

    CheckStatus( sd.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write after removing members" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n  \"b\" : 2\n}", "removed members are not written" );

    Json::StructuredData other;
    CheckStatus( ParseText( other, "{\"x\":0}" ).status, Json::Status::kOk, "parse a second object" );

    Json::EditHandle otherRoot = other.Edit( *other.GetRoot() );

    CheckStatus( other.AddMember( otherRoot, "b", *root.AsObject().Find( "b" ) ), Json::Status::kOk, "copy a member to another object" );
    CheckStatus( sd.RemoveMember( edited, "b" ), Json::Status::kOk, "remove the copied member from its object" );

    CheckStatus( sd.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write the object a member moved out of" );
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n}", "a moved member leaves its old object" );

    CheckStatus( other.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write the object a member moved into" );
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n  \"x\" : 0,\n  \"b\" : 2\n}", "a moved member is written once, last" );
}

void TestDuplicateNames()
{
    Json::StructuredData sd;

    CheckStatus( sd.SerializeFrom( Repeated() ), Json::Status::kDuplicateName, "a proto that repeats a name" );
    Check( sd.GetRoot() == nullptr, "a failed SerializeFrom does not set a root" );

    CountingSource source;

    {
        Json::StructuredData repeated( source );
        repeated.SerializeFrom( Repeated() );
        Check( source.outstandingCount == 0, "a failed SerializeFrom hands every block back" );
    }

    Check( source.outstandingCount == 0, "a repeated name does not leak" );
}

void TestAccountedMap()
{
    Json::StructuredData sd;

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":2,\"c\":3}" ).status, Json::Status::kOk, "parse three members" );

    const Json::Value& root   = *sd.GetRoot();
    Json::EditHandle   edited = sd.Edit( root );

    CheckStatus( sd.RemoveMember( edited, "b" ), Json::Status::kOk, "remove a member" );
    CheckStatus( sd.RemoveMember( edited, "missing" ), Json::Status::kNameNotPresent, "remove a missing member" );

    CheckStatus( sd.Write( "Data/Test/out_test11_accounted.json" ), Json::Status::kOk, "write after removing a member" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test11_accounted.json" ).status, Json::Status::kOk, "reparse after removing a member" );

    Json::ObjectView reparsedRoot = reparsed.GetRoot()->AsObject();
    Check( reparsedRoot.GetMemberCount() == 2 && reparsedRoot.FindNumber( "a" ) && reparsedRoot.FindNumber( "c" ) && !reparsedRoot.FindNumber( "b" ),
           "the removed member is not written" );

    CheckStatus( sd.AddMember( edited, "z", 5.0 ), Json::Status::kOk, "insert a new member" );
    CheckStatus( sd.AddMember( edited, "z", 6.0 ), Json::Status::kDuplicateName, "a duplicate insert is refused" );

    Check( root.AsObject().FindNumber( "z" ) && *root.AsObject().FindNumber( "z" ) == 5.0, "the first inserted member stays" );

    CheckStatus( sd.SerializeAt( edited, Json::ObjectMap<double>() ), Json::Status::kOk, "serialize an empty map onto the object" );
    Check( root.AsObject() && root.AsObject().GetMemberCount() == 0, "purge removes every member" );

    CheckStatus( sd.Write( "Data/Test/out_test11_accounted.json" ), Json::Status::kOk, "write after purging" );
    CheckStatus( reparsed.Parse( "Data/Test/out_test11_accounted.json" ).status, Json::Status::kOk, "reparse after purging" );
    Check( reparsed.GetRoot()->AsObject() && reparsed.GetRoot()->AsObject().GetMemberCount() == 0, "an empty object is written" );
}

Terathon::String<> LargeObjectText( int count, const char* extraMember )
{
    Terathon::String<> text = "{";
    char               member[ 32 ];

    for ( int a = 0; a < count; a++ )
    {
        snprintf( member, sizeof( member ), "%s\"k%d\":%d", ( a != 0 ) ? "," : "", a, a );
        text += member;
    }

    text += extraMember;
    text += "}";

    return text;
}

void TestLargeObject()
{
    const int count = 100;
    char      name[ 16 ];

    Json::StructuredData sd;
    CheckStatus( ParseText( sd, LargeObjectText( count, "" ) ).status, Json::Status::kOk, "parse an object with many members" );

    Json::ObjectView root  = sd.GetRoot()->AsObject();
    bool             found = ( root.GetMemberCount() == Terathon::uint32( count ) ) && !root.Find( "k100" );

    for ( int a = 0; a < count; a++ )
    {
        snprintf( name, sizeof( name ), "k%d", a );
        found = found && root.FindNumber( name ) && ( *root.FindNumber( name ) == a );
    }

    Check( found, "every member of a large object is found" );

    int  position = 0;
    bool ordered  = true;

    for ( const Json::Member& member : root )
    {
        snprintf( name, sizeof( name ), "k%d", position++ );
        ordered = ordered && ( member.GetName() == name );
    }

    Check( ordered, "a large object keeps document order" );

    CheckStatus( ParseText( sd, LargeObjectText( count, ",\"k50\":-1" ) ).status, Json::Status::kDuplicateName, "a duplicate in a large object is rejected" );

    Json::ParseOptions keepLast;
    keepLast.duplicateNames = Json::DuplicateNames::kKeepLast;

    CheckStatus( ParseText( sd, LargeObjectText( count, ",\"k50\":-1" ), keepLast ).status, Json::Status::kOk, "keep the last duplicate in a large object" );

    root = sd.GetRoot()->AsObject();
    Check( ( root.GetMemberCount() == Terathon::uint32( count ) ) && ( *root.FindNumber( "k50" ) == -1 ) && ( root.begin()[ 50 ].GetName() == "k50" ),
           "the last value of a duplicate takes the first one's place" );

    Json::EditHandle edited = sd.Edit( *sd.GetRoot() );

    CheckStatus( sd.RemoveMember( edited, "k10" ), Json::Status::kOk, "remove a member of a large object" );
    CheckStatus( sd.RemoveMember( edited, "k20" ), Json::Status::kOk, "remove another member of a large object" );

    found = ( root.GetMemberCount() == Terathon::uint32( count - 2 ) ) && !root.Find( "k10" ) && !root.Find( "k20" );

    for ( int a = 0; a < count; a++ )
    {
        snprintf( name, sizeof( name ), "k%d", a );
        found = found && ( ( a == 10 ) || ( a == 20 ) || root.Find( name ) );
    }

    Check( found, "members removed from a large object are gone and the others are still found" );

    CheckStatus( sd.AddMember( edited, "k20", 20.0 ), Json::Status::kOk, "insert a removed member again" );
    Check( ( root.GetMemberCount() == Terathon::uint32( count - 1 ) ) && ( root.begin()[ count - 2 ].GetName() == "k20" ) &&
               ( root.Find( "k20" ) == &root.begin()[ count - 2 ].GetValue() ) && ( *root.FindNumber( "k20" ) == 20 ),
           "a member inserted again goes last and is found" );
}

int main()
{
    TestDuplicateKeysRejected();
    TestDuplicateKeysKept( Json::DuplicateNames::kKeepFirst, 1, "the first value of a duplicate key is kept" );
    TestDuplicateKeysKept( Json::DuplicateNames::kKeepLast, 0, "the last value of a duplicate key is kept" );
    TestOrderSurvivesDirectChanges();
    TestDuplicateNames();
    TestAccountedMap();
    TestLargeObject();

    return TestResult();
}
