#include <Json4C4/C4Json.h>

namespace Json = C4::Json;
using String   = Terathon::String<>;

struct MapExample
{
    String                  version;
    Json::ObjectMap<String> map;
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

    jSd.DeserializeTo(object);

    if (object.version != "1.0.1")
    {
        fprintf(stderr, "Error deserializing version string");
        return 1;
    }

    if (object.map.FindMapElement("str1")->data != "str1v")
    {
        fprintf(stderr, "Error deserializing str1 string");
        return 1;
    }
    if (object.map.FindMapElement("str2")->data != "str2v")
    {
        fprintf(stderr, "Error deserializing str2 string");
        return 1;
    }

    if (object.map.FindMapElement("str3")->data != "str3v")
    {
        fprintf(stderr, "Error deserializing str3 string");
        return 1;
    }

    return 0;
}