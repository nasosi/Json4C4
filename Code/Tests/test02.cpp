#include <Json4C4/C4Json.h>

namespace Json = C4::Json;

struct ObjectInObjectArray
{
    double             d = 0.0;
    Terathon::String<> string;

#define OBJECT_IN_OBJECT_ARRAY_PROTO "double", d, "string", string
    DEFINE_JSON4C4_MEMBER_FUNCTIONS(OBJECT_IN_OBJECT_ARRAY_PROTO)
};

struct Object
{
    Terathon::Array<ObjectInObjectArray> objectInObjectArrayArray;

    bool b = true;
};
#define OBJECT_PROTO "bool", object.b, "objectArray", object.objectInObjectArrayArray
DEFINE_JSON4C4_FUNCTIONS(Object, OBJECT_PROTO)

struct RootObject
{
    Object                              object;
    double                              d = 0;
    Terathon::Array<double>             doubleArray;
    Terathon::Array<Terathon::String<>> stringArray;

#define ROOT_OBJECT_PROTO "double", d, "doubleArray", doubleArray, "stringArray", stringArray, "object", object
    DEFINE_JSON4C4_MEMBER_FUNCTIONS(ROOT_OBJECT_PROTO)
};

int main()
{
    Json::StructuredData jSd;

    auto parseResult = jSd.Parse("Data/Test/test_file02.json");

    if (parseResult.status != Json::Status::kOk)
    {
        Terathon::String<256> errorDescription = Json::ParseResultToString(parseResult);

        fprintf(stderr, "JSON parse error: %s\n", static_cast<const char*>(errorDescription));

        return 1;
    }

    RootObject rootObject;

    if (jSd.DeserializeTo(rootObject) != Json::Status::kOk)
    {
        fprintf(stderr, "Failed to deserialize.");
        return 1;
    }

    if (rootObject.d != 2.45e-2)
    {
        fprintf(stderr, "trootObject.d not parsed correclty");
        return 1;
    }

    if (rootObject.doubleArray[5] != 1.9643e-11)
    {
        fprintf(stderr, "trootObject.d not parsed correclty");
        return 1;
    }

    if (rootObject.object.objectInObjectArrayArray[2].string != "str7")
    {
        fprintf(stderr, "trootObject.d not parsed correclty");
        return 1;
    }

    rootObject.object.objectInObjectArrayArray[2].d = 3.14159e-17;

    jSd.SerializeFrom(rootObject);

    jSd.Write("Data/Test/test_file02b.json");

    Json::StructuredData jSd2;

    parseResult = jSd2.Parse("Data/Test/test_file02b.json");

    if (parseResult.status != Json::Status::kOk)
    {
        Terathon::String<256> errorDescription = Json::ParseResultToString(parseResult);

        fprintf(stderr, "JSON parse error: %s\n", static_cast<const char*>(errorDescription));

        return 1;
    }

    RootObject rootObject2;

    if (jSd.DeserializeTo(rootObject2) != Json::Status::kOk)
    {
        fprintf(stderr, "Failed to deserialize.");
        return 1;
    }

    bool ok = (rootObject2.d == rootObject.d) && (rootObject2.doubleArray[5] == rootObject2.doubleArray[5]) &&
              (rootObject2.object.objectInObjectArrayArray[2].string == rootObject.object.objectInObjectArrayArray[2].string) &&
              (rootObject2.object.objectInObjectArrayArray[2].d == rootObject.object.objectInObjectArrayArray[2].d);

    if (!ok)
    {
        fprintf(stderr, "Roundtrip failed.");
        return 1;
    }

    return 0;
}