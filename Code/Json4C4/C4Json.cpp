// Json4C4
// A C++ JSON library
//
// MIT License
//
// Copyright(c) 2024 Athanasios Iliopoulos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// This software uses third party libraries. Their licenses can be found in the
// "Licenses" subfolder of the source code repository.

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ ) || defined( __NT__ )
#
#    define JSON4C4_WINDOWS
#
#elif ( defined __linux__ ) || ( defined __linux )
#
#    define JSON4C4_LINUX
#
#else
#
#    error "Unsupported Operating System"
#
#endif

#ifdef JSON4C4_WINDOWS

#    include "windows.h"

#elif defined( JSON4C4_LINUX )

#    include <cstdio>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>

#endif

#include "C4Json.h"

// Without system headers, Terathon declares its own SSE types, which clash with the <emmintrin.h> that NumberTextThree
// includes for its SIMD digits.
#if defined( TERATHON_NO_SYSTEM ) && !defined( NUMBERTEXTTHREE_NO_SIMD )

#    define NUMBERTEXTTHREE_NO_SIMD

#endif

#include "NumberTextThree.h"

#if defined( _MSC_VER )

#    pragma warning( push )

#endif

#include "TSText.h"

#if defined( _MSC_VER )

#    pragma warning( pop )
#    pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#endif

#ifdef C4_ENGINE_MODULE

#    include "C4Files.h"

#endif

#define JSON4C4VERSIONMAJOR 2
#define JSON4C4VERSIONMINOR 2
#define JSON4C4VERSIONPATCH 1

#define STRN( s ) #s
#define STR( s ) STRN( s )

#define JSON4C4VERSION STR( JSON4C4VERSIONMAJOR ) "." STR( JSON4C4VERSIONMINOR ) "." STR( JSON4C4VERSIONPATCH )

#ifndef JSON4C4_MAX_FILE_SIZE

#    define JSON4C4_MAX_FILE_SIZE 0x7FFFFFFE

#endif

namespace C4
{

#ifndef C4_ENGINE_MODULE

#    if defined JSON4C4_WINDOWS

    class WCharBuffer
    {
    private:
        wchar_t* buffer = nullptr;

    public:
        WCharBuffer( Terathon::uint32 maxLength )
        {
            wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
            wchar_t* end        = buffer + maxLength;

            while ( c != end )
            {
                *( c++ ) = 0;
            }

            *end = 0;
        }

        WCharBuffer( const wchar_t* str, Terathon::uint32 maxLength )
        {
            wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
            wchar_t* end        = buffer + maxLength;

            while ( *str != L'\0' && c != end )
            {
                *( c++ ) = *( str++ );
            }

            *c   = 0;
            *end = 0;
        }

        WCharBuffer( const Terathon::String<>& str )
        {
            using namespace Terathon;

            int32 length = Text::GetWideTextCharCount( str );
            buffer       = new wchar_t[ length + 1 ];

            Text::ConvertStringToWideText( str, reinterpret_cast<uint16*>( buffer ), length );
        }

        ~WCharBuffer()
        {
            if ( buffer != nullptr )
            {
                delete[] buffer;
            }
        }

        operator wchar_t*()
        {
            return buffer;
        }

        operator const wchar_t*() const
        {
            return buffer;
        }

        operator Terathon::String<>()
        {
            using namespace Terathon;

            int32 charArrayLength = Text::GetUnicodeStringLength( reinterpret_cast<uint16*>( buffer ) );

            String<> string;
            string.SetStringLength( charArrayLength );

            Text::ConvertWideTextToString( reinterpret_cast<uint16*>( buffer ), string, charArrayLength );

            return string;
        }
    };

#    endif

    enum FileOpenMode
    {
        kFileReadOnly,
        kFileCreate
    };

    enum FileStatus
    {
        kFileOkay,
        kFileOpenFailed,
        kFileNotOpen,
        kFileWriteError,
        kFileReadError
    };

    class File
    {
    private:
        bool IsReadOnly = false;

#    if defined( JSON4C4_WINDOWS )

        HANDLE fileHandle = nullptr;

#    elif defined( JSON4C4_LINUX )

        int fileDescriptor = -1;

#    endif

    public:
        ~File()
        {
            CloseFile();
        }

        FileStatus OpenFile( const char* fileName, FileOpenMode fileOpenMode )
        {
            CloseFile();

#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            if ( fileOpenMode == kFileReadOnly )
            {
                HANDLE handle =
                    CreateFileW( wideFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

                if ( handle == INVALID_HANDLE_VALUE )
                {
                    return kFileOpenFailed;
                }

                this->fileHandle = handle;
                this->IsReadOnly = true;

                return kFileOkay;
            }

            HANDLE handle = CreateFileW( wideFileName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );

            if ( handle == INVALID_HANDLE_VALUE )
            {
                return kFileOpenFailed;
            }

            this->fileHandle = handle;
            this->IsReadOnly = false;

            return kFileOkay;

#    elif defined( JSON4C4_LINUX )

            if ( fileOpenMode == kFileReadOnly )
            {
                int fd = open( fileName, O_RDONLY );

                if ( fd == -1 )
                {
                    return kFileOpenFailed;
                }

                this->fileDescriptor = fd;
                this->IsReadOnly     = true;

                return kFileOkay;
            }

            int fd = open( fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644 );

            if ( fd == -1 )
            {
                return kFileOpenFailed;
            }

            this->fileDescriptor = fd;
            this->IsReadOnly     = false;

            return kFileOkay;

#    endif
        }

        void CloseFile()
        {
#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle != nullptr )
            {
                CloseHandle( fileHandle );

                fileHandle = nullptr;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor != -1 )
            {
                close( this->fileDescriptor );

                fileDescriptor = -1;
            }

#    endif
        }

        Terathon::uint64 GetFileSize()
        {

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return 0;
            }

            FILE_STANDARD_INFO fileInfo { 0 };

            bool result = GetFileInformationByHandleEx( fileHandle, FileStandardInfo, &fileInfo, sizeof( FILE_STANDARD_INFO ) );

            if ( !result )
            {
                return 0;
            }

            return fileInfo.EndOfFile.QuadPart;

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return 0;
            }

            struct stat statBuffer;

            int result = fstat( this->fileDescriptor, &statBuffer );

            if ( result != 0 )
            {
                return 0;
            }

            return statBuffer.st_size;

#    endif
        }

        FileStatus ReadFile( char* buffer, Terathon::uint64 size )
        {

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return kFileNotOpen;
            }

            if ( size > JSON4C4_MAX_FILE_SIZE )
            {
                return kFileReadError;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            char*       p     = buffer;

            while ( p != end )
            {
                DWORD numberOfBytesRead = 0;

                BOOL ok = ::ReadFile( fileHandle, p, DWORD( end - p ), &numberOfBytesRead, nullptr );

                if ( !ok || numberOfBytesRead == 0 )
                {
                    return kFileReadError;
                }

                p += numberOfBytesRead;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return kFileNotOpen;
            }

            if ( size > JSON4C4_MAX_FILE_SIZE )
            {
                return kFileReadError;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            char*       p     = buffer;

            while ( p != end )
            {
                ssize_t numberOfBytesRead = read( this->fileDescriptor, p, size_t( end - p ) );

                if ( numberOfBytesRead <= 0 )
                {
                    return kFileReadError;
                }

                p += numberOfBytesRead;
            }

#    endif

            return kFileOkay;
        }

        static bool CanReplaceFile( const char* fileName )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            DWORD attributes = GetFileAttributesW( wideFileName );

            return ( attributes == INVALID_FILE_ATTRIBUTES ) || ( ( attributes & ( FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT ) ) == 0 );

#    elif defined( JSON4C4_LINUX )

            struct stat statBuffer;

            if ( lstat( fileName, &statBuffer ) != 0 )
            {
                return true;
            }

            return S_ISREG( statBuffer.st_mode ) && ( statBuffer.st_nlink == 1 );

#    endif
        }

        void CopyPermissionsFrom( const char* fileName )
        {
#    if defined( JSON4C4_LINUX )

            struct stat statBuffer;

            if ( this->fileDescriptor != -1 && stat( fileName, &statBuffer ) == 0 )
            {
                fchmod( this->fileDescriptor, statBuffer.st_mode & 07777 );
            }

#    else

            static_cast<void>( fileName );

#    endif
        }

        static bool MoveOverFile( const char* source, const char* destination )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideSource( Terathon::String<> { source } );
            const WCharBuffer wideDestination( Terathon::String<> { destination } );

            if ( GetFileAttributesW( wideDestination ) != INVALID_FILE_ATTRIBUTES )
            {
                return ReplaceFileW(
                           wideDestination, wideSource, nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr ) != 0;
            }

            return MoveFileExW( wideSource, wideDestination, MOVEFILE_WRITE_THROUGH ) != 0;

#    elif defined( JSON4C4_LINUX )

            return rename( source, destination ) == 0;

#    endif
        }

        static void RemoveFile( const char* fileName )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            DeleteFileW( wideFileName );

#    elif defined( JSON4C4_LINUX )

            unlink( fileName );

#    endif
        }

        FileStatus WriteFile( const char* buffer, Terathon::uint64 size )
        {
            if ( IsReadOnly )
            {
                return kFileWriteError;
            }

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return kFileNotOpen;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            const char* p     = begin;

            while ( p != end )
            {
                constexpr Terathon::uint64 maxChunkSize = 0x40000000;

                Terathon::uint64 remaining            = Terathon::uint64( end - p );
                DWORD            numberOfBytesWritten = 0;

                BOOL ok = ::WriteFile( fileHandle, p, DWORD( remaining < maxChunkSize ? remaining : maxChunkSize ), &numberOfBytesWritten, nullptr );

                if ( !ok || numberOfBytesWritten == 0 )
                {
                    return kFileWriteError;
                }

                p += numberOfBytesWritten;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return kFileNotOpen;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            const char* p     = begin;

            while ( p != end )
            {
                ssize_t numberOfBytesWritten = write( this->fileDescriptor, p, size_t( end - p ) );

                if ( numberOfBytesWritten <= 0 )
                {
                    return kFileWriteError;
                }

                p += numberOfBytesWritten;
            }

#    endif

            return kFileOkay;
        }
    };

#endif

    namespace Json
    {
        const char* GetVersion()
        {
            return JSON4C4VERSION;
        }

        static const char* const StatusString[] = { "Ok",
                                                    "Could not open file",
                                                    "Could not read file",
                                                    "File is too large",
                                                    "Invalid JSON text",
                                                    "Unterminated text buffer",
                                                    "Expected opening curly braces",
                                                    "Expected closing curly braces or comma",
                                                    "Expected opening square braces",
                                                    "Expected closing square braces or comma",
                                                    "Illegal character",
                                                    "Illegal escape",
                                                    "Expected colon",
                                                    "Expected begining double quotes",
                                                    "Expected ending double quotes",
                                                    "Invalid number",
                                                    "Expected 'true' or 'false'",
                                                    "Expected 'null'",
                                                    "Expected a JSON value (object, array, string, number, 'true', 'false', or 'null')",
                                                    "Expected end of file",
                                                    "File ends prematurely",
                                                    "Numbers cannot have leading zeros",
                                                    "JSON structure is invalid",
                                                    "An expected object value pair was not found",
                                                    "An exception was caught",
                                                    "JSON structured data contains invalid value type",
                                                    "Could not find requested name of name/value pair",
                                                    "Maximum nesting depth exceeded",
                                                    "Duplicate name in object",
                                                    "Could not write file",
                                                    "Maximum value count exceeded",
                                                    "Maximum string byte count exceeded" };

        constexpr unsigned int statusStringCount = sizeof( StatusString ) / sizeof( StatusString[ 0 ] );

        static_assert( statusStringCount == (unsigned int)( Status::kMaximumStringByteCountExceeded ) + 1, "StatusString must have one entry per Status" );

        TERATHON_API String<256> StatusToString( const Status& status ) noexcept
        {
            if ( (unsigned int)( status ) >= statusStringCount )
            {
                return "Unknown status";
            }

            return StatusString[ (unsigned int)( status ) ];
        }

        alignas( 64 ) const int8 hexadecimalCharValue[ 55 ] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11,
                                                                12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15 };

        constexpr uint8 space          = 0x20;
        constexpr uint8 tab            = 0x09;
        constexpr uint8 newLine        = 0x0A;
        constexpr uint8 carriageReturn = 0x0D;

        constexpr double maxDouble = 1.7976931348623157e308;

        class StringValue;
        class NumberValue;
        class BoolValue;
        class NullValue;
        class ArrayValue;

        Status ExpandArray( Array<char>& array, const char c, const uint32 n )
        {
            int32 count = array.GetArrayElementCount();

            if ( n > uint32( JSON4C4_MAX_FILE_SIZE - count ) )
            {
                return Status::kFileTooLarge;
            }

            array.SetArrayElementCount( count + int32( n ), c );

            return Status::kOk;
        }

        inline void CopyText( char* restrict destination, const char* restrict source, int32 length ) noexcept
        {
            for ( int32 a = 0; a < length; a++ )
            {
                destination[ a ] = source[ a ];
            }
        }

        Status AppendText( Array<char>& output, const char* text, int32 length )
        {
            int32 count = output.GetArrayElementCount();

            if ( length > JSON4C4_MAX_FILE_SIZE - count )
            {
                return Status::kFileTooLarge;
            }

            output.SetArrayElementCount( count + length );
            CopyText( output.begin() + count, text, length );

            return Status::kOk;
        }

        Status AppendText( Array<char>& output, const char* text )
        {
            return AppendText( output, text, Text::GetTextLength( text ) );
        }

        // The length of the character at byte when a string literal holds it unescaped, or 0 when it has to be escaped or is
        // not valid UTF-8.
        inline int32 GetPlainCharLength( const uint8* byte ) noexcept
        {
            uint32 c = byte[ 0 ];

            if ( c < 0x80U )
            {
                return ( ( c >= 0x20U ) && ( c != '"' ) && ( c != '\\' ) );
            }

            int32 length = Text::ValidateUnicodeChar( reinterpret_cast<const char*>( byte ) );

            return ( ( c == 0xEDU ) && ( byte[ 1 ] >= 0xA0U ) ) ? 0 : length;
        }

        // A character that may run into the end of the text is read from a copy followed by zeros.
        inline int32 GetPlainCharLength( const uint8* byte, const uint8* end ) noexcept
        {
            if ( end - byte >= 4 )
            {
                return GetPlainCharLength( byte );
            }

            char tail[ 4 ] = {};
            CopyText( tail, reinterpret_cast<const char*>( byte ), int32( end - byte ) );

            return GetPlainCharLength( reinterpret_cast<const uint8*>( tail ) );
        }

        Status AppendEscapedString( Array<char>& output, const char* text )
        {
            static const char hexadecimalDigit[] = "0123456789abcdef";

            Status status = AppendText( output, "\"", 1 );

            for ( const uint8* byte = reinterpret_cast<const uint8*>( text ); status == Status::kOk; )
            {
                const uint8* run = byte;

                while ( int32 length = GetPlainCharLength( byte ) )
                {
                    byte += length;
                }

                status = AppendText( output, reinterpret_cast<const char*>( run ), int32( byte - run ) );

                uint8 c = byte[ 0 ];

                if ( ( status != Status::kOk ) || ( c == 0 ) )
                {
                    break;
                }

                if ( c >= 0x80U )
                {
                    return Status::kStringIllegalChar;
                }

                char  escape[ 6 ]  = { '\\', char( c ), 0, 0, 0, 0 };
                int32 escapeLength = 2;

                if ( c == '\b' )
                {
                    escape[ 1 ] = 'b';
                }
                else if ( c == '\f' )
                {
                    escape[ 1 ] = 'f';
                }
                else if ( c == '\n' )
                {
                    escape[ 1 ] = 'n';
                }
                else if ( c == '\r' )
                {
                    escape[ 1 ] = 'r';
                }
                else if ( c == '\t' )
                {
                    escape[ 1 ] = 't';
                }
                else if ( c < 0x20 )
                {
                    escape[ 1 ]  = 'u';
                    escape[ 2 ]  = '0';
                    escape[ 3 ]  = '0';
                    escape[ 4 ]  = hexadecimalDigit[ c >> 4 ];
                    escape[ 5 ]  = hexadecimalDigit[ c & 0x0F ];
                    escapeLength = 6;
                }

                status = AppendText( output, escape, escapeLength );
                byte++;
            }

            return ( status == Status::kOk ) ? AppendText( output, "\"", 1 ) : status;
        }

        inline bool IsWhitespace( uint32 c ) noexcept
        {
            return ( c == space ) || ( c == tab ) || ( c == newLine ) || ( c == carriageReturn );
        }

        inline bool IsDigit( uint32 c ) noexcept
        {
            return ( c - '0' ) < 10U;
        }

        int32 ReadEscapeChar( const char* text, uint32* value ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );
            uint32       c    = byte[ 0 ];

            if ( ( c == '\"' ) || ( c == '\\' ) || ( c == '/' ) )
            {
                *value = c;
                return ( 1 );
            }
            else if ( c == 'b' )
            {
                *value = '\b';
                return ( 1 );
            }
            else if ( c == 'f' )
            {
                *value = '\f';
                return ( 1 );
            }
            else if ( c == 'n' )
            {
                *value = '\n';
                return ( 1 );
            }
            else if ( c == 'r' )
            {
                *value = '\r';
                return ( 1 );
            }
            else if ( c == 't' )
            {
                *value = '\t';
                return ( 1 );
            }

            return ( 0 );
        }

        bool ReadHexadecimalCode( const uint8* byte, uint32* code ) noexcept
        {
            uint32 value = 0;

            for ( machine a = 0; a < 4; a++ )
            {
                uint32 c = byte[ a ] - '0';
                if ( c >= 55U )
                {
                    return ( false );
                }

                int32 x = hexadecimalCharValue[ c ];
                if ( x < 0 )
                {
                    return ( false );
                }

                value = ( value << 4 ) | uint32( x );
            }

            *code = value;

            return ( true );
        }

        int32 ReadStringEscapeChar( const char* text, int32* stringLength, char* restrict string ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            if ( byte[ 0 ] == 'u' )
            {
                uint32 code = 0;

                if ( !ReadHexadecimalCode( byte + 1, &code ) || ( code == 0 ) || ( code - 0xDC00U < 0x0400U ) )
                {
                    return ( 0 );
                }

                int32 textLength = 5;

                if ( code - 0xD800U < 0x0400U )
                {
                    uint32 lowCode = 0;

                    if ( ( byte[ 5 ] != '\\' ) || ( byte[ 6 ] != 'u' ) || !ReadHexadecimalCode( byte + 7, &lowCode ) || ( lowCode - 0xDC00U >= 0x0400U ) )
                    {
                        return ( 0 );
                    }

                    code       = 0x010000 + ( ( code - 0xD800 ) << 10 ) + ( lowCode - 0xDC00 );
                    textLength = 11;
                }

                if ( string )
                {
                    *stringLength = Text::WriteUnicodeChar( string, code );
                }
                else
                {
                    *stringLength = 1 + ( code >= 0x000080 ) + ( code >= 0x000800 ) + ( code >= 0x010000 );
                }

                return ( textLength );
            }

            uint32 value;

            int32 textLength = ReadEscapeChar( text, &value );
            if ( textLength != 0 )
            {
                if ( string )
                {
                    *string = char( value );
                }

                *stringLength = 1;
            }

            return ( textLength );
        }

        class StringValue final : public Value
        {
        public:
            using DataType = String<>;

            DataType data;
            ~StringValue() noexcept override { };

            String<>* GetStringData() noexcept override
            {
                return &data;
            }

            const String<>* GetStringData() const noexcept override
            {
                return &data;
            }

            Status
            Write( Array<char>& output, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {
                return AppendEscapedString( output, data );
            }
        };

        class NumberValue final : public Value
        {
        public:
            using DataType = double;
            DataType data  = 0;

            ~NumberValue() override { };

            double* GetDoubleData() noexcept override
            {
                return &data;
            }

            const double* GetDoubleData() const noexcept override
            {
                return &data;
            }

            Status
            Write( Array<char>& output, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {
                if ( !( data >= -maxDouble && data <= maxDouble ) )
                {
                    return Status::kFloatInvalid;
                }

                char text[ NUMBERTEXTTHREE_F64_MAX_CHARS ];

                return AppendText( output, text, ntt_write_f64( data, text ) );
            }
        };

        class BoolValue final : public Value
        {
        public:
            using DataType = bool;
            DataType data  = true;

            ~BoolValue() override { };

            bool* GetBoolData() noexcept override
            {
                return &data;
            }

            const bool* GetBoolData() const noexcept override
            {
                return &data;
            }

            Status Write( Array<char>& output, uint32, const char, Array<char>& ) const override
            {
                return AppendText( output, data ? "true" : "false" );
            }
        };

        class NullValue final : public Value
        {
        public:
            using DataType = Null;
            DataType data;

            ~NullValue() override { };

            Null* GetJsonNullData() noexcept override
            {
                return &data;
            }

            const Null* GetJsonNullData() const noexcept override
            {
                return &data;
            }

            Status
            Write( Array<char>& output, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {
                return AppendText( output, "null" );
            }
        };

        namespace Detail
        {
            template <>
            Value* CreateValueFor( const String<>& ) noexcept( false )
            {
                return new StringValue;
            }

            template <>
            Value* CreateValueFor( const double& ) noexcept( false )
            {
                return new NumberValue;
            }

            template <>
            Value* CreateValueFor( const bool& ) noexcept( false )
            {
                return new BoolValue;
            }

            template <>
            Value* CreateValueFor( const Null& ) noexcept( false )
            {
                return new NullValue;
            }

        } // namespace Detail

        // Objects with more members than this find a name through an index of member positions sorted by name, smaller
        // ones by comparing every name.
        constexpr int32 indexedMemberCount = 16;

        // Orders names by their bytes, read as unsigned values.
        int32 CompareNames( const char* first, const char* second ) noexcept
        {
            const uint8* a = reinterpret_cast<const uint8*>( first );
            const uint8* b = reinterpret_cast<const uint8*>( second );

            while ( ( a[ 0 ] == b[ 0 ] ) && ( a[ 0 ] != 0 ) )
            {
                a++;
                b++;
            }

            return int32( a[ 0 ] ) - int32( b[ 0 ] );
        }

        // How far back from the end the parser looks for a character that ends every string run.
        constexpr int32 stringStopSearchLength = 4096;

        class Parser
        {
        public:
            // Runs of whitespace and of string characters test the end of the text only once. Everything from spaceStop
            // on is whitespace, so a whitespace run that starts before it stops there at the latest. stringStop follows a
            // quote, a backslash or a control character, none of which a string run passes or a UTF-8 character
            // spans, so a string run that starts before it stops there at the latest.
            Parser( const char* begin, const char* textEnd, const ParseOptions& parseOptions ) noexcept :
                end( textEnd ),
                spaceStop( textEnd ),
                stringStop( begin ),
                options( parseOptions ),
                valuesLeft( parseOptions.maxValueCount ),
                stringBytesLeft( parseOptions.maxStringByteCount )
            {
                while ( ( spaceStop != begin ) && IsWhitespace( uint8( spaceStop[ -1 ] ) ) )
                {
                    spaceStop--;
                }

                const char* searchStart = ( end - begin > stringStopSearchLength ) ? end - stringStopSearchLength : begin;

                for ( const char* c = end; c != searchStart; c-- )
                {
                    uint32 byte = uint8( c[ -1 ] );

                    if ( ( byte == '"' ) || ( byte == '\\' ) || ( byte < 0x20U ) )
                    {
                        stringStop = c;
                        break;
                    }
                }
            }

            const char* SkipWhitespace( const char* text ) const noexcept
            {
                if ( text >= spaceStop )
                {
                    return end;
                }

                while ( IsWhitespace( uint8( text[ 0 ] ) ) )
                {
                    text++;
                }

                return text;
            }

            Status ParseValue( Value*& value, const char*& text, int32 depth ) noexcept
            {
                switch ( Peek( text ) )
                {
                    case '"':
                        return ParseNew<StringValue>( value, text, depth );

                    case '-':
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        return ParseNew<NumberValue>( value, text, depth );

                    case 't':
                    case 'f':
                        return ParseNew<BoolValue>( value, text, depth );

                    case 'n':
                        return ParseNew<NullValue>( value, text, depth );

                    case '[':
                        return ( depth < options.maxNestingDepth ) ? ParseNew<ArrayValue>( value, text, depth + 1 ) : Status::kMaximumNestingDepthExceeded;

                    case '{':
                        return ( depth < options.maxNestingDepth ) ? ParseNew<ObjectValue>( value, text, depth + 1 ) : Status::kMaximumNestingDepthExceeded;

                    default:
                        return Status::kExpectedJsonValue;
                }
            }

        private:
            const char*         end;
            const char*         spaceStop;
            const char*         stringStop;
            const ParseOptions& options;
            int64               valuesLeft;
            int64               stringBytesLeft;

            // Where the names of the objects being parsed begin, for each member past their first indexedMemberCount.
            Array<const char*> nameStarts;

            Array<ObjectValue::NamedPosition> sortedNames;
            Array<ObjectValue::NamedPosition> sortScratch;

            uint32 Peek( const char* text ) const noexcept
            {
                return ( text != end ) ? uint8( text[ 0 ] ) : 0;
            }

            Status ReadStringLiteral( const char* text, int32* textLength, int32* stringLength, char* restrict string = nullptr ) const noexcept
            {
                const uint8* byte    = reinterpret_cast<const uint8*>( text );
                const uint8* byteEnd = reinterpret_cast<const uint8*>( end );
                const uint8* stop    = reinterpret_cast<const uint8*>( stringStop );
                int32        count   = 0;
                Status       status  = Status::kOk;

                for ( ;; )
                {
                    const uint8* run = byte;

                    if ( byte < stop )
                    {
                        while ( int32 length = GetPlainCharLength( byte ) )
                        {
                            byte += length;
                        }
                    }
                    else
                    {
                        while ( int32 length = GetPlainCharLength( byte, byteEnd ) )
                        {
                            byte += length;
                        }
                    }

                    int32 runLength = int32( byte - run );

                    if ( string )
                    {
                        CopyText( string, reinterpret_cast<const char*>( run ), runLength );
                        string += runLength;
                    }

                    count += runLength;

                    uint32 c = ( byte != byteEnd ) ? byte[ 0 ] : 0;

                    if ( c != '\\' )
                    {
                        if ( c != '\"' )
                        {
                            status = ( c == 0 ) ? Status::kPrematureNullTerminator : Status::kStringIllegalChar;
                        }

                        break;
                    }

                    // The longest escape, a surrogate pair, is 11 characters after the backslash.
                    const char* escape = reinterpret_cast<const char*>( byte + 1 );
                    char        tail[ 11 ];

                    if ( byteEnd - byte < 12 )
                    {
                        int32 left = int32( byteEnd - byte - 1 );

                        for ( int32 a = 0; a < 11; a++ )
                        {
                            tail[ a ] = ( a < left ) ? escape[ a ] : 0;
                        }

                        escape = tail;
                    }

                    int32 stringLen;

                    int32 textLen = ReadStringEscapeChar( escape, &stringLen, string );
                    if ( textLen == 0 )
                    {
                        status = Status::kStringIllegalEscape;
                        break;
                    }

                    if ( string )
                    {
                        string += stringLen;
                    }

                    byte += textLen + 1;
                    count += stringLen;
                }

                *textLength   = int32( reinterpret_cast<const char*>( byte ) - text );
                *stringLength = count;

                return ( status );
            }

            Status ParseString( String<>& string, const char*& text ) noexcept
            {
                if ( Peek( text ) != '"' )
                {
                    return ( Status::kExpectedBeginingDoubleQuotes );
                }

                int32 textLength;
                int32 stringLength;

                Status status = ReadStringLiteral( text + 1, &textLength, &stringLength );
                if ( status != Status::kOk )
                {
                    text += textLength + 1;
                    return ( status );
                }

                if ( stringLength > stringBytesLeft )
                {
                    return Status::kMaximumStringByteCountExceeded;
                }

                stringBytesLeft -= stringLength;
                text++;

                status = MayThrow(
                    [ & ]()
                    {
                        string.SetStringLength( stringLength );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    return status;
                }

                if ( stringLength == textLength )
                {
                    CopyText( string, text, stringLength );
                }
                else
                {
                    ReadStringLiteral( text, &textLength, &stringLength, string );
                }

                text += textLength + 1;

                return ( Status::kOk );
            }

            template <class T>
            Status ParseNew( Value*& value, const char*& text, int32 depth ) noexcept
            {
                if ( valuesLeft == 0 )
                {
                    return Status::kMaximumValueCountExceeded;
                }

                valuesLeft--;

                T* newValue = nullptr;

                Status status = MayThrow(
                    [ & ]()
                    {
                        newValue = new T;
                        return Status::kOk;
                    } );

                if ( status == Status::kOk )
                {
                    status = Parse( newValue, text, depth );

                    if ( status == Status::kOk )
                    {
                        value = newValue;
                        return status;
                    }

                    delete newValue;
                }

                return status;
            }

            Status Parse( StringValue* value, const char*& text, int32 ) noexcept
            {
                return ParseString( value->data, text );
            }

            Status Parse( NumberValue* value, const char*& text, int32 ) const noexcept
            {
                const char* stop;

                // A number out of range keeps the value NumberTextThree gives it, and Write refuses an infinity. The only
                // invalid number NumberTextThree stops at a digit is one with a leading zero.
                if ( ntt_parse_json_f64( text, end, &value->data, &stop ) == NUMBERTEXTTHREE_INVALID )
                {
                    return IsDigit( Peek( stop ) ) ? Status::kNumbersCannotHaveLeadingZeros : Status::kFloatInvalid;
                }

                text = stop;

                return Status::kOk;
            }

            Status Parse( BoolValue* value, const char*& text, int32 ) const noexcept
            {
                if ( end - text >= 4 && text[ 0 ] == 't' && text[ 1 ] == 'r' && text[ 2 ] == 'u' && text[ 3 ] == 'e' )
                {
                    value->data = true;
                    text += 4;

                    return Status::kOk;
                }

                if ( end - text >= 5 && text[ 0 ] == 'f' && text[ 1 ] == 'a' && text[ 2 ] == 'l' && text[ 3 ] == 's' && text[ 4 ] == 'e' )
                {
                    value->data = false;
                    text += 5;

                    return Status::kOk;
                }

                return Status::kExpectedTrueOrFalse;
            }

            Status Parse( NullValue*, const char*& text, int32 ) const noexcept
            {
                if ( end - text >= 4 && text[ 0 ] == 'n' && text[ 1 ] == 'u' && text[ 2 ] == 'l' && text[ 3 ] == 'l' )
                {
                    text += 4;

                    return Status::kOk;
                }

                return Status::kExpectedNull;
            }

            Status Parse( ArrayValue* array, const char*& text, int32 depth ) noexcept
            {
                bool emptyArray = true;

                for ( ;; )
                {
                    text++;
                    text = SkipWhitespace( text );

                    if ( Peek( text ) == ']' )
                    {
                        if ( !emptyArray )
                        {
                            return Status::kExpectedJsonValue;
                        }

                        text++;

                        return Status::kOk;
                    }

                    Value* element = nullptr;
                    Status status  = ParseValue( element, text, depth );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    status = MayThrow(
                        [ & ]()
                        {
                            array->data.AppendArrayElement( element );
                            return Status::kOk;
                        } );

                    if ( status != Status::kOk )
                    {
                        delete element;
                        return status;
                    }

                    emptyArray = false;

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ',' )
                    {
                        break;
                    }
                }

                if ( Peek( text ) != ']' )
                {
                    return Status::kExpectedClosingSquareBracketsOrComma;
                }

                text++;

                return Status::kOk;
            }

            Status Parse( ObjectValue* object, const char*& text, int32 depth ) noexcept
            {
                int32  nameBase = nameStarts.GetArrayElementCount();
                Status status   = ParseMembers( object, text, depth );

                if ( ( status == Status::kOk ) && ( object->members.GetArrayElementCount() > indexedMemberCount ) )
                {
                    status = IndexMembers( object, text, nameBase );
                }

                nameStarts.SetArrayElementCount( nameBase );

                return status;
            }

            Status ParseMembers( ObjectValue* object, const char*& text, int32 depth ) noexcept
            {
                bool emptyObject = true;

                for ( ;; )
                {
                    text++;
                    text = SkipWhitespace( text );

                    if ( Peek( text ) == '}' )
                    {
                        if ( !emptyObject )
                        {
                            return Status::kExpectedJsonValue;
                        }

                        text++;

                        return Status::kOk;
                    }

                    const char* nameStart = text;

                    String<> name;
                    Status   status = ParseString( name, text );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ':' )
                    {
                        return Status::kExpectedColon;
                    }

                    text++;
                    text = SkipWhitespace( text );

                    Value* member = nullptr;

                    status = ParseValue( member, text, depth );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    member->name = static_cast<String<>&&>( name );

                    // Past the first members, IndexMembers finds the duplicate names once the object ends.
                    bool  indexed  = object->members.GetArrayElementCount() >= indexedMemberCount;
                    int32 position = indexed ? -1 : object->FindMemberPosition( member->name );

                    if ( position < 0 )
                    {
                        status = MayThrow(
                            [ & ]()
                            {
                                if ( indexed )
                                {
                                    nameStarts.AppendArrayElement( nameStart );
                                }

                                object->members.AppendArrayElement( member );
                                return Status::kOk;
                            } );

                        if ( status != Status::kOk )
                        {
                            delete member;
                            return status;
                        }

                        member->owner = object;
                    }
                    else if ( options.duplicateNames == DuplicateNames::kKeepLast )
                    {
                        object->ReplaceMember( position, member );
                    }
                    else
                    {
                        delete member;

                        if ( options.duplicateNames == DuplicateNames::kReject )
                        {
                            text = nameStart;
                            return Status::kDuplicateName;
                        }
                    }

                    emptyObject = false;

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ',' )
                    {
                        break;
                    }
                }

                if ( Peek( text ) != '}' )
                {
                    return Status::kExpectedClosingCurlyBracesOrComma;
                }

                text++;

                return Status::kOk;
            }

            // Sorts the members of a large object by name and settles the duplicate names among them. With kReject, the
            // duplicate reported is the earliest member whose name came before, as if each member were checked when read.
            Status IndexMembers( ObjectValue* object, const char*& text, int32 nameBase ) noexcept
            {
                Status status = MayThrow(
                    [ & ]()
                    {
                        object->BuildMemberIndex( sortedNames, sortScratch );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    return status;
                }

                Array<Value*, 8>&                 members = object->members;
                const ObjectValue::NamedPosition* sorted  = sortedNames.begin();
                int32                             count   = members.GetArrayElementCount();
                int32                             repeat  = count;
                bool                              removed = false;

                for ( int32 first = 0, next = 1; first < count; first = next++ )
                {
                    while ( ( next < count ) && ( CompareNames( sorted[ next ].name, sorted[ first ].name ) == 0 ) )
                    {
                        next++;
                    }

                    if ( next - first == 1 )
                    {
                        continue;
                    }

                    if ( options.duplicateNames == DuplicateNames::kReject )
                    {
                        repeat = ( sorted[ first + 1 ].position < repeat ) ? sorted[ first + 1 ].position : repeat;
                        continue;
                    }

                    Value* kept = members[ sorted[ ( options.duplicateNames == DuplicateNames::kKeepLast ) ? next - 1 : first ].position ];

                    for ( int32 a = first; a < next; a++ )
                    {
                        Value*& member = members[ sorted[ a ].position ];

                        if ( member != kept )
                        {
                            member->owner = nullptr;
                            delete member;
                        }

                        member = nullptr;
                    }

                    members[ sorted[ first ].position ] = kept;
                    removed                             = true;
                }

                if ( repeat < count )
                {
                    text = nameStarts[ nameBase + repeat - indexedMemberCount ];
                    return Status::kDuplicateName;
                }

                if ( removed )
                {
                    int32 keptCount = 0;

                    for ( int32 a = 0; a < members.GetArrayElementCount(); a++ )
                    {
                        if ( members[ a ] )
                        {
                            members[ keptCount++ ] = members[ a ];
                        }
                    }

                    members.SetArrayElementCount( keptCount );

                    status = MayThrow(
                        [ & ]()
                        {
                            object->BuildMemberIndex( sortedNames, sortScratch );
                            return Status::kOk;
                        } );
                }

                return status;
            }
        };

        ArrayValue::~ArrayValue()
        {
            for ( auto pointer : data )
            {
                delete pointer;
            }
        };

        Array<Value*>* ArrayValue::GetJsonValueArrayData() noexcept
        {
            return &data;
        }

        const Array<Value*>* ArrayValue::GetJsonValueArrayData() const noexcept
        {
            return &data;
        }

        Status ArrayValue::Write( Array<char>& output, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const
        {
            Status status = AppendText( output, "[\n" );

            if ( status == Status::kOk )
            {
                status = ExpandArray( indentationCharArray, indentationChar, indentationLength );
            }

            for ( int32 a = 0; status == Status::kOk && a != data.GetArrayElementCount(); a++ )
            {
                status = AppendText( output, indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

                if ( status == Status::kOk )
                {
                    status = data[ a ]->Write( output, indentationLength, indentationChar, indentationCharArray );
                }

                if ( status == Status::kOk )
                {
                    status = AppendText( output, ( a < data.GetArrayElementCount() - 1 ) ? ",\n" : "\n" );
                }
            }

            if ( status != Status::kOk )
            {
                return status;
            }

            indentationCharArray.SetArrayElementCount( indentationCharArray.GetArrayElementCount() - int32( indentationLength ) );

            status = AppendText( output, indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

            return ( status == Status::kOk ) ? AppendText( output, "]", 1 ) : status;
        }

        Value::~Value() noexcept
        {
            Detach();
        }

        void Value::Detach() noexcept
        {
            if ( owner )
            {
                owner->RemoveMember( this );
            }
        }

        int32 ObjectValue::FindIndexSlot( const char* key ) const noexcept
        {
            int32 low  = 0;
            int32 high = memberIndex.GetArrayElementCount();

            while ( low < high )
            {
                int32 middle = low + ( high - low ) / 2;

                if ( CompareNames( members[ memberIndex[ middle ] ]->name, key ) < 0 )
                {
                    low = middle + 1;
                }
                else
                {
                    high = middle;
                }
            }

            return low;
        }

        int32 ObjectValue::FindMemberPosition( const char* key ) const noexcept
        {
            if ( memberIndex.GetArrayElementCount() == 0 )
            {
                for ( int32 a = 0; a < members.GetArrayElementCount(); a++ )
                {
                    const String<>& memberName = members[ a ]->name;

                    if ( ( memberName[ 0 ] == key[ 0 ] ) && ( memberName == key ) )
                    {
                        return a;
                    }
                }

                return -1;
            }

            int32 slot = FindIndexSlot( key );

            if ( ( slot < memberIndex.GetArrayElementCount() ) && ( members[ memberIndex[ slot ] ]->name == key ) )
            {
                return memberIndex[ slot ];
            }

            return -1;
        }

        // A merge sort, so that members with the same name stay in document order. The names are sorted with their
        // positions, so that each comparison reads only the two names.
        void ObjectValue::BuildMemberIndex( Array<NamedPosition>& sorted, Array<NamedPosition>& scratch )
        {
            int32 count = members.GetArrayElementCount();

            sorted.SetArrayElementCount( count );
            scratch.SetArrayElementCount( count );
            memberIndex.SetArrayElementCount( count );

            for ( int32 a = 0; a < count; a++ )
            {
                sorted[ a ] = NamedPosition { members[ a ]->name, a };
            }

            NamedPosition* from = sorted.begin();
            NamedPosition* to   = scratch.begin();

            for ( int32 width = 1; width < count; width *= 2 )
            {
                for ( int32 left = 0; left < count; left += 2 * width )
                {
                    int32 middle = ( count - left > width ) ? left + width : count;
                    int32 right  = ( count - middle > width ) ? middle + width : count;
                    int32 i      = left;
                    int32 j      = middle;

                    for ( int32 k = left; k < right; k++ )
                    {
                        bool takeLeft = ( j == right ) || ( ( i < middle ) && ( CompareNames( from[ i ].name, from[ j ].name ) <= 0 ) );
                        to[ k ]       = takeLeft ? from[ i++ ] : from[ j++ ];
                    }
                }

                NamedPosition* swap = from;
                from                = to;
                to                  = swap;
            }

            for ( int32 a = 0; a < count; a++ )
            {
                sorted[ a ]      = from[ a ];
                memberIndex[ a ] = from[ a ].position;
            }
        }

        void ObjectValue::AppendMember( Value* member )
        {
            int32 count = members.GetArrayElementCount() + 1;

            members.ReserveArrayElementCount( count );

            if ( ( count > indexedMemberCount ) && ( memberIndex.GetArrayElementCount() == 0 ) )
            {
                Array<NamedPosition> sorted;
                Array<NamedPosition> scratch;

                BuildMemberIndex( sorted, scratch );
            }

            if ( memberIndex.GetArrayElementCount() != 0 )
            {
                memberIndex.ReserveArrayElementCount( count );
                memberIndex.InsertArrayElement( FindIndexSlot( member->name ), count - 1 );
            }

            members.AppendArrayElement( member );
            member->owner = this;
        }

        void ObjectValue::ReplaceMember( int32 position, Value* member ) noexcept
        {
            Value* replaced = members[ position ];

            replaced->owner = nullptr;
            delete replaced;

            members[ position ] = member;
            member->owner       = this;
        }

        void ObjectValue::RemoveMember( Value* member ) noexcept
        {
            for ( int32 a = 0; a < members.GetArrayElementCount(); a++ )
            {
                if ( members[ a ] == member )
                {
                    members.RemoveArrayElement( a );
                    member->owner = nullptr;

                    int32 kept = 0;

                    for ( int32 position : memberIndex )
                    {
                        if ( position != a )
                        {
                            memberIndex[ kept++ ] = position - ( position > a );
                        }
                    }

                    memberIndex.SetArrayElementCount( kept );

                    return;
                }
            }
        }

        bool ObjectValue::InsertAccountedMapElement( Value* element )
        {
            if ( FindMemberPosition( element->name ) >= 0 )
            {
                return false;
            }

            element->Detach();
            AppendMember( element );

            return true;
        }

        Value* ObjectValue::RemoveAccountedMapElement( const char* key ) noexcept
        {
            Value* element = FindMapElement( key );

            if ( element )
            {
                element->Detach();
            }

            return element;
        }

        void ObjectValue::PurgeAccountedMap() noexcept
        {
            for ( Value* member : members )
            {
                member->owner = nullptr;
                delete member;
            }

            members.PurgeArray();
            memberIndex.PurgeArray();
        }

        ObjectValue::~ObjectValue() noexcept
        {
            PurgeAccountedMap();
        }

        ObjectValue* ObjectValue::AsJsonObjectValue() noexcept
        {
            return this;
        }

        const ObjectValue* ObjectValue::AsJsonObjectValue() const noexcept
        {
            return this;
        }

        Value* ObjectValue::FindMapElement( const char* key ) const noexcept
        {
            int32 position = FindMemberPosition( key );

            return ( position >= 0 ) ? members[ position ] : nullptr;
        }

        Value* ObjectValue::FindValue( const char* key ) noexcept
        {
            return FindMapElement( key );
        }

        const Value* ObjectValue::FindValue( const char* key ) const noexcept
        {
            return FindMapElement( key );
        }

        const String<>* ObjectValue::FindString( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->GetDataAsPointerTo<String<>>();
            }

            return nullptr;
        }

        const double* ObjectValue::FindNumber( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->GetDataAsPointerTo<double>();
            }

            return nullptr;
        }

        const bool* ObjectValue::FindBool( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->GetDataAsPointerTo<bool>();
            }

            return nullptr;
        }

        const Null* ObjectValue::FindJsonNull( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->GetDataAsPointerTo<Null>();
            }

            return nullptr;
        }

        const Array<Value*>* ObjectValue::FindJsonValueArray( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->GetDataAsPointerTo<Array<Value*>>();
            }

            return nullptr;
        }

        const ObjectValue* ObjectValue::FindJsonObjectValue( const char* key ) const noexcept
        {
            if ( const Value* jsonValue = FindValue( key ) )
            {
                return jsonValue->AsJsonObjectValue();
            }

            return nullptr;
        }

        Status ObjectValue::Write( Array<char>& output, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const
        {
            int32  count  = members.GetArrayElementCount();
            Status status = AppendText( output, "{\n" );

            if ( status == Status::kOk )
            {
                status = ExpandArray( indentationCharArray, indentationChar, indentationLength );
            }

            for ( int32 a = 0; status == Status::kOk && a != count; a++ )
            {
                const Value* value = members[ a ];

                status = AppendText( output, indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

                if ( status == Status::kOk )
                {
                    status = AppendEscapedString( output, value->name );
                }

                if ( status == Status::kOk )
                {
                    status = AppendText( output, " : " );
                }

                if ( status == Status::kOk )
                {
                    status = value->Write( output, indentationLength, indentationChar, indentationCharArray );
                }

                if ( status == Status::kOk )
                {
                    status = AppendText( output, ( a < count - 1 ) ? ",\n" : "\n" );
                }
            }

            if ( status != Status::kOk )
            {
                return status;
            }

            indentationCharArray.SetArrayElementCount( indentationCharArray.GetArrayElementCount() - int32( indentationLength ) );

            status = AppendText( output, indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

            return ( status == Status::kOk ) ? AppendText( output, "}", 1 ) : status;
        }

        Status ParseJsonRoot( Value*& jsonRoot, const char* begin, const char* end, const ParseOptions& options, int32* errorLine, int32* errorColumn ) noexcept
        {
            Parser      parser( begin, end, options );
            const char* text   = parser.SkipWhitespace( begin );
            Status      status = parser.ParseValue( jsonRoot, text, 0 );

            if ( status == Status::kOk )
            {
                text = parser.SkipWhitespace( text );

                if ( text != end )
                {
                    status = Status::kExpectedEndOfFile;
                }
            }
            else if ( status == Status::kPrematureNullTerminator && text != end )
            {
                status = Status::kStringIllegalChar;
            }

            if ( status != Status::kOk )
            {
                delete jsonRoot;
                jsonRoot = nullptr;

                int32       line      = 1;
                const char* lineStart = begin;

                for ( const char* c = begin; c != text; c++ )
                {
                    if ( ( c[ 0 ] == '\n' ) || ( ( c[ 0 ] == '\r' ) && ( ( c + 1 == end ) || ( c[ 1 ] != '\n' ) ) ) )
                    {
                        line++;
                        lineStart = c + 1;
                    }
                }

                *errorLine   = line;
                *errorColumn = int32( text - lineStart ) + 1;
            }

            return status;
        }

        StructuredData::~StructuredData() noexcept
        {
            delete rootJsonValue;
        }

        ParseResult StructuredData::Parse( const char* fileName, const ParseOptions& options ) noexcept
        {
            delete rootJsonValue;
            rootJsonValue = nullptr;

            File       file;
            FileStatus fileStatus = kFileOpenFailed;

            MayThrow(
                [ & ]()
                {
                    fileStatus = file.OpenFile( fileName, kFileReadOnly );
                    return Status::kOk;
                } );

            if ( fileStatus != kFileOkay )
            {
                return ParseResult { Status::kFileOpenError, 0, 0 };
            }

            Terathon::uint64 fileSize = file.GetFileSize();

            if ( fileSize > JSON4C4_MAX_FILE_SIZE )
            {
                return ParseResult { Status::kFileTooLarge, 0, 0 };
            }

            Array<char> nullTerminatedText;

            Status status = MayThrow(
                [ & ]()
                {
                    nullTerminatedText.SetArrayElementCount( int32( fileSize + 1 ), 0 );

                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                return ParseResult { status, 0, 0 };
            }

            if ( file.ReadFile( nullTerminatedText.begin(), fileSize ) != kFileOkay )
            {
                return ParseResult { Status::kFileReadError, 0, 0 };
            }

            file.CloseFile();

            return Parse( nullTerminatedText, options );
        }

        ParseResult StructuredData::Parse( const Array<char>& nullTerminatedTextBuffer, const ParseOptions& options ) noexcept
        {
            int32 count = nullTerminatedTextBuffer.GetArrayElementCount();

            if ( ( count >= 2 ) && ( nullTerminatedTextBuffer[ count - 1 ] != 0 ) )
            {
                delete rootJsonValue;
                rootJsonValue = nullptr;

                return ParseResult { Status::kUnterminatedTextBuffer, 0, 0 };
            }

            return Parse( nullTerminatedTextBuffer.begin(), uint64( ( count > 0 ) ? count - 1 : 0 ), options );
        }

        ParseResult StructuredData::Parse( const char* text, uint64 length, const ParseOptions& options ) noexcept
        {
            delete rootJsonValue;
            rootJsonValue = nullptr;

            if ( ( text == nullptr ) || ( length == 0 ) )
            {
                return ParseResult { Status::kTextBufferContentsInvalid, 0, 0 };
            }

            if ( length > JSON4C4_MAX_FILE_SIZE )
            {
                return ParseResult { Status::kFileTooLarge, 0, 0 };
            }

            ParseResult parseResult;
            parseResult.status = ParseJsonRoot( rootJsonValue, text, text + length, options, &parseResult.errorLine, &parseResult.errorColumn );

            return parseResult;
        }

        TERATHON_API Status StructuredData::Write( const char* fileName, const uint32 indentationLength, const char indentationChar ) noexcept
        {
            if ( !rootJsonValue )
            {
                return Status::kInvalidStructuredData;
            }

            Array<char> output;

            Status status = MayThrow(
                [ & ]()
                {
                    Array<char> indentCharArray;

                    return rootJsonValue->Write( output, indentationLength, indentationChar, indentCharArray );
                } );

            if ( status != Status::kOk )
            {
                return status;
            }

#ifdef C4_ENGINE_MODULE

            File file;
            if ( file.OpenFile( fileName, kFileCreate ) != kFileOkay )
            {
                return Status::kFileOpenError;
            }

            if ( file.WriteFile( output.begin(), output.GetArrayElementCount() ) != kFileOkay )
            {
                return Status::kFileWriteError;
            }

            file.CloseFile();

            return Status::kOk;

#else

            String<>   temporaryName;
            File       file;
            FileStatus fileStatus = kFileOpenFailed;

            status = MayThrow(
                [ & ]()
                {
                    if ( File::CanReplaceFile( fileName ) )
                    {
                        String<> name( fileName );
                        name += ".json4c4.tmp";

                        temporaryName = static_cast<String<>&&>( name );
                        fileStatus    = file.OpenFile( temporaryName, kFileCreate );
                    }

                    if ( fileStatus != kFileOkay )
                    {
                        temporaryName = "";
                        fileStatus    = file.OpenFile( fileName, kFileCreate );
                    }

                    return Status::kOk;
                } );

            if ( status != Status::kOk || fileStatus != kFileOkay )
            {
                return ( status != Status::kOk ) ? status : Status::kFileOpenError;
            }

            bool written = ( file.WriteFile( output.begin(), output.GetArrayElementCount() ) == kFileOkay );

            if ( temporaryName.GetStringLength() == 0 )
            {
                file.CloseFile();

                return written ? Status::kOk : Status::kFileWriteError;
            }

            file.CopyPermissionsFrom( fileName );
            file.CloseFile();

            bool replaced = false;

            MayThrow(
                [ & ]()
                {
                    replaced = written && File::MoveOverFile( temporaryName, fileName );

                    if ( !replaced )
                    {
                        File::RemoveFile( temporaryName );
                    }

                    return Status::kOk;
                } );

            return replaced ? Status::kOk : Status::kFileWriteError;

#endif
        }

        const Value* StructuredData::GetRootJsonValue() const noexcept
        {
            return rootJsonValue;
        }

        Value* StructuredData::GetRootJsonValue() noexcept
        {
            return rootJsonValue;
        }

        String<256> ParseResultToString( const ParseResult& parseResult ) noexcept
        {
            String<256> string = StatusToString( parseResult.status );

            if ( parseResult.errorLine != 0 && parseResult.status != Status::kException )
            {
                string += ". Line: ";
                string += String<16>( parseResult.errorLine );
                string += ", column: ";
                string += String<16>( parseResult.errorColumn );
            }

            return string;
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        namespace Detail
        {
            template <>
            Value* CreateValueFor( const std::string& )
            {
                return new StringValue;
            }
        } // namespace Detail

        template <>
        TERATHON_API Status Validate( const Value* value, const std::string& /*data*/ ) noexcept
        {
            const String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return Status::kOk;
        }

        template <>
        TERATHON_API Status Deserialize( const Value* value, std::string& data ) noexcept
        {
            const String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    data = static_cast<const char*>( *valueData );

                    return Status::kOk;
                } );
        }

        template <>
        TERATHON_API Status Serialize( Value* value, const std::string& data ) noexcept
        {
            String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    String<> copy( data.c_str() );
                    *valueData = static_cast<String<>&&>( copy );

                    return Status::kOk;
                } );
        }

#endif

    } // namespace Json
} // namespace C4

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::String<>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::String<>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::String<>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, double& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const double& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const double& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, bool& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const bool& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const bool& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::Null& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::Null& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::Null& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;

#endif
