#include <Json4C4/C4Json.h>

#include <iostream>

int main()
{
    C4::Json::StructuredData json;

    auto parseResult = json.Parse( "myfile.json" );

    if ( parseResult.status != C4::Json::Status::kOk )
    {
        std::cerr << "Parsing error: " <<  static_cast<const char*>( C4::Json::ParseResultToString( parseResult ) ) << endl;
    }

    return 0;
}
