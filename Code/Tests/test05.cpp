#include <Json4C4/C4Json.h>

namespace Json = C4::Json;

struct MapExample
{
    std::string                        version;
    std::map<std::string, std::string> map;
};

#define PROTO "version", object.version, "stringMap", object.map
DEFINE_JSON4C4_FUNCTIONS(MapExample, PROTO)

int main()
{
    Json::StructuredData jSd;

    auto parseResult = jSd.Parse("Data/Test/test_file04.json");

    if (parseResult.status != Json::Status::kOk)
    {
        Terathon::String<256> errorDescription = Json::ParseResultToString(parseResult);

        fprintf(stderr, "JSON parse error: %s\n", static_cast<const char*>(errorDescription));

        return 1;
    }

    MapExample object;

    if ( Json::Validate( jSd, object ) != Json::Status::kOk )
    {
        fprintf(stderr, "Error validating JSON structured data.");
        return 1;
    }

    jSd.DeserializeTo(object);

    if (object.version != "1.0.1")
    {
        fprintf(stderr, "Error deserializing version string");
        return 1;
    }

    if (object.map["str1"] != "str1v")
    {
        fprintf(stderr, "Error deserializing str1 string");
        return 1;
    }
    if (object.map["str2"] != "str2v")
    {
        fprintf(stderr, "Error deserializing str2 string");
        return 1;
    }

    if (object.map["str3"] != "str3v")
    {
        fprintf(stderr, "Error deserializing str3 string");
        return 1;
    }

    return 0;
}