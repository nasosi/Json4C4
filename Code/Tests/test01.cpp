#include <Json4C4/C4Json.h>

namespace Json = C4::Json;

struct TestFile01
{
    Terathon::String<> str1;
    std::string        str2;
    double             d = 0;
    bool               b = false;
    Json::Null         null;
};

#define PROTO "str1", object.str1, "str2", object.str2, "double", object.d, "bool", object.b, "null", object.null

DEFINE_JSON4C4_FUNCTIONS(TestFile01, PROTO)

int main()
{
    Json::StructuredData jSd;

    auto parseResult = jSd.Parse("Data/Test/test_file01.json");

    if (parseResult.status != Json::Status::kOk)
    {
        Terathon::String<256> errorDescription = Json::ParseResultToString(parseResult);

        fprintf(stderr, "JSON parse error: %s\n", static_cast<const char*>(errorDescription));

        return 1;
    }

    TestFile01 testFile01;

    if (jSd.DeserializeTo(testFile01) != Json::Status::kOk)
    {
        fprintf(stderr, "Failed to deserialize.");
        return 1;
    }

    if (testFile01.str1 != "str1Value")
    {
        fprintf(stderr, "testFile01.str1 not parsed correctly");
        return 1;
    }

    if (testFile01.str2 != "str2Value")
    {
        fprintf(stderr, "testFile01.str2 not parsed correctly");
        return 1;
    }

    if (testFile01.d != 2.36531e-7)
    {
        fprintf(stderr, "testFile01.d not parsed correctly");
        return 1;
    }

    if (testFile01.b != true)
    {
        fprintf(stderr, "testFile01.b not parsed correctly");
        return 1;
    }

    return 0;
}
