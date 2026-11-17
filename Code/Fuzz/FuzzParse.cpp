#include <Json4C4/C4Json.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace Json = C4::Json;

using Terathon::Array;
using Terathon::String;

struct Entry
{
    float    value = 0;
    String<> string;
};

#define ENTRY_PROTO Json::optional, "double", object.value, "string", object.string

DEFINE_JSON4C4_FUNCTIONS( Entry, ENTRY_PROTO )

struct Sample
{
    Array<double>             doubleArray;
    Array<Entry>              objectArray;
    Json::ObjectMap<String<>> stringMap;
    String<>                  version;
};

#define SAMPLE_PROTO                                                                                                                                           \
    Json::optional, "doubleArray", object.doubleArray, Json::optional, "objectArray", object.objectArray, Json::optional, "stringMap", object.stringMap,       \
        "version", object.version

DEFINE_JSON4C4_FUNCTIONS( Sample, SAMPLE_PROTO )

// Aborts unless the text a document writes parses into a document that writes the same text.
void RoundTrip( const Json::StructuredData& document )
{
    Array<char> first;

    if ( document.Write( first ) != Json::Status::kOk )
    {
        return;
    }

    Json::StructuredData reparsed;
    Array<char>          second;
    Terathon::int32      length = first.GetArrayElementCount();

    if ( ( reparsed.ParseText( first.begin(), Terathon::uint64( length ) ).status != Json::Status::kOk ) || ( reparsed.Write( second ) != Json::Status::kOk ) ||
         ( second.GetArrayElementCount() != length ) || ( memcmp( first.begin(), second.begin(), size_t( length ) ) != 0 ) )
    {
        abort();
    }
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size )
{
    const char*        text = reinterpret_cast<const char*>( data );
    Json::ParseOptions options;

    options.duplicateNames     = Json::DuplicateNames( size % 3 );
    options.maxValueCount      = 0x1000;
    options.maxStringByteCount = 0x10000;

    Json::StructuredData document;

    if ( document.ParseText( text, size, options ).status != Json::Status::kOk )
    {
        return 0;
    }

    RoundTrip( document );

    Sample sample;

    Json::Validate( document, sample );

    if ( Json::Read( text, size, sample, options ).status == Json::Status::kOk )
    {
        Json::StructuredData written;

        if ( written.SerializeFrom( sample ) == Json::Status::kOk )
        {
            RoundTrip( written );
        }
    }

    const Json::Value& root   = *document.GetRoot();
    Json::EditHandle   handle = document.Edit( root );
    Json::Status       status = root.AsObject() ? document.AddMember( handle, "copy", root ) : document.AppendElement( handle, root );

    if ( status == Json::Status::kOk )
    {
        Array<char> edited;
        document.Write( edited );
    }

    return 0;
}
