#include "TestCheck.h"

#ifdef __linux__

#    include <sys/stat.h>
#    include <unistd.h>

#endif

int main()
{
    {
        Json::StructuredData sd;

        CheckStatus( sd.Parse( "Data/Test/does_not_exist.json" ).status, Json::Status::kFileOpenError, "parse a missing file" );
        Json::Status status = sd.Parse( "Data/Test" ).status;
        Check( status == Json::Status::kFileOpenError || status == Json::Status::kFileReadError || status == Json::Status::kTextBufferContentsInvalid,
               "parse a directory" );
        CheckStatus( sd.Parse( "Data/Test/no_such_directory/file.json" ).status, Json::Status::kFileOpenError, "parse in a missing directory" );
    }

    {
        Check( WriteTextFile( "Data/Test/out_test16_empty.json", "" ), "write an empty file" );

        Json::StructuredData sd;
        CheckStatus( sd.Parse( "Data/Test/out_test16_empty.json" ).status, Json::Status::kTextBufferContentsInvalid, "parse an empty file" );
    }

    {
        Check( WriteTextFile( "Data/Test/out_test16_digit.json", "7" ), "write a one byte file" );

        Json::StructuredData sd;
        CheckStatus( sd.Parse( "Data/Test/out_test16_digit.json" ).status, Json::Status::kOk, "parse a one byte file" );
        Check( sd.GetRootJsonValue() && sd.GetRootJsonValue()->GetDataAsPointerTo<double>() && *sd.GetRootJsonValue()->GetDataAsPointerTo<double>() == 7,
               "one byte file value" );
    }

    {
        Check( WriteTextFile( "Data/Test/out_test16_crlf.json", "{\r\n  \"a\" : 1,\r\n  \"b\" : ]\r\n}" ), "write a CRLF file" );

        Json::StructuredData sd;
        Json::ParseResult    parseResult = sd.Parse( "Data/Test/out_test16_crlf.json" );
        CheckStatus( parseResult.status, Json::Status::kExpectedJsonValue, "parse a CRLF file with an error" );
        Check( parseResult.errorLine == 3 && parseResult.errorColumn == 9, "error position in a CRLF file" );
    }

    {
        Json::StructuredData sd;
        CheckStatus( ParseText( sd, "{\"a\":1}" ).status, Json::Status::kOk, "parse a document to write" );
        CheckStatus( sd.Write( "Data/Test/no_such_directory/file.json" ), Json::Status::kFileOpenError, "write into a missing directory" );
        CheckStatus( sd.Write( "Data/Test" ), Json::Status::kFileOpenError, "write onto a directory" );
    }

#ifdef __linux__

    {
        Json::StructuredData sd;
        CheckStatus( ParseText( sd, "{\"a\":2}" ).status, Json::Status::kOk, "parse a document to save" );

        Check( WriteTextFile( "Data/Test/out_test16_private.json", "{\"a\":1}" ), "write a private file" );
        chmod( "Data/Test/out_test16_private.json", 0600 );

        CheckStatus( sd.Write( "Data/Test/out_test16_private.json" ), Json::Status::kOk, "save over a private file" );

        struct stat statBuffer;
        Check( stat( "Data/Test/out_test16_private.json", &statBuffer ) == 0 && ( statBuffer.st_mode & 0777 ) == 0600, "saving keeps the file's permissions" );

        Check( WriteTextFile( "Data/Test/out_test16_target.json", "{\"a\":1}" ), "write a link target" );
        unlink( "Data/Test/out_test16_link.json" );
        Check( symlink( "out_test16_target.json", "Data/Test/out_test16_link.json" ) == 0, "create a symbolic link" );

        CheckStatus( sd.Write( "Data/Test/out_test16_link.json" ), Json::Status::kOk, "save through a symbolic link" );

        Terathon::String<> text;
        Check( lstat( "Data/Test/out_test16_link.json", &statBuffer ) == 0 && S_ISLNK( statBuffer.st_mode ), "saving keeps the symbolic link" );
        Check( ReadTextFile( "Data/Test/out_test16_target.json", text ) && strstr( text, "2" ) != nullptr, "saving through a link updates its target" );
    }

#endif

    return TestResult();
}
