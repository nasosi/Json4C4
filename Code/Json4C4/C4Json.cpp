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

#ifndef TERATHON_NO_SYSTEM

#    ifndef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#        define JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#    endif

#endif

#ifdef JSON4C4_WINDOWS

#    include "windows.h"

#elif defined( JSON4C4_LINUX )

#    include <cstdio>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>

#endif

#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#    include <clocale>
#    include <cstdio>
#    include <cstdlib>

#endif

#include "C4Json.h"

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
#define JSON4C4VERSIONMINOR 0
#define JSON4C4VERSIONPATCH 0

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
                                                    "Could not write file" };

        constexpr unsigned int statusStringCount = sizeof( StatusString ) / sizeof( StatusString[ 0 ] );

        static_assert( statusStringCount == (unsigned int)( Status::kFileWriteError ) + 1, "StatusString must have one entry per Status" );

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
#ifndef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS
        class BigInteger
        {
        private:
            static constexpr int32 maxWordCount = 136;

            uint32 word[ maxWordCount ];
            int32  wordCount = 0;

            void Multiply( uint32 factor ) noexcept
            {
                uint64 carry = 0;

                for ( int32 a = 0; a < wordCount; a++ )
                {
                    uint64 product = uint64( word[ a ] ) * factor + carry;
                    word[ a ]      = uint32( product );
                    carry          = product >> 32;
                }

                if ( carry != 0 )
                {
                    word[ wordCount++ ] = uint32( carry );
                }
            }

            uint32 Divide( uint32 divisor ) noexcept
            {
                uint64 remainder = 0;

                for ( int32 a = wordCount - 1; a >= 0; a-- )
                {
                    uint64 current = ( remainder << 32 ) | word[ a ];
                    word[ a ]      = uint32( current / divisor );
                    remainder      = current % divisor;
                }

                Trim();

                return uint32( remainder );
            }

            void Trim() noexcept
            {
                while ( wordCount > 0 && word[ wordCount - 1 ] == 0 )
                {
                    wordCount--;
                }
            }

        public:
            explicit BigInteger( uint64 value = 0 ) noexcept
            {
                while ( value != 0 )
                {
                    word[ wordCount++ ] = uint32( value );
                    value >>= 32;
                }
            }

            bool IsZero() const noexcept
            {
                return wordCount == 0;
            }

            int32 GetBitCount() const noexcept
            {
                if ( wordCount == 0 )
                {
                    return 0;
                }

                int32  count = ( wordCount - 1 ) * 32;
                uint32 top   = word[ wordCount - 1 ];

                while ( top != 0 )
                {
                    count++;
                    top >>= 1;
                }

                return count;
            }

            bool GetBit( int32 index ) const noexcept
            {
                int32 wordIndex = index >> 5;

                return ( wordIndex < wordCount ) && ( ( word[ wordIndex ] >> ( index & 31 ) ) & 1 );
            }

            bool AnyBitBelow( int32 index ) const noexcept
            {
                int32 wordIndex = index >> 5;

                for ( int32 a = 0; a < wordIndex && a < wordCount; a++ )
                {
                    if ( word[ a ] != 0 )
                    {
                        return true;
                    }
                }

                return ( wordIndex < wordCount ) && ( index & 31 ) && ( word[ wordIndex ] & ( ( 1U << ( index & 31 ) ) - 1 ) );
            }

            uint64 GetBits( int32 index, int32 count ) const noexcept
            {
                uint64 result = 0;

                for ( int32 a = count - 1; a >= 0; a-- )
                {
                    result = ( result << 1 ) | uint64( GetBit( index + a ) );
                }

                return result;
            }

            void AddSmall( uint32 value ) noexcept
            {
                uint64 carry = value;

                for ( int32 a = 0; a < wordCount && carry != 0; a++ )
                {
                    uint64 sum = uint64( word[ a ] ) + carry;
                    word[ a ]  = uint32( sum );
                    carry      = sum >> 32;
                }

                if ( carry != 0 )
                {
                    word[ wordCount++ ] = uint32( carry );
                }
            }

            void MultiplyAdd( uint32 factor, uint32 value ) noexcept
            {
                Multiply( factor );
                AddSmall( value );
            }

            void MultiplyPower2( int32 exponent ) noexcept
            {
                int32 wordShift = exponent >> 5;
                int32 bitShift  = exponent & 31;

                if ( wordCount == 0 )
                {
                    return;
                }

                if ( wordShift != 0 )
                {
                    for ( int32 a = wordCount - 1; a >= 0; a-- )
                    {
                        word[ a + wordShift ] = word[ a ];
                    }

                    for ( int32 a = 0; a < wordShift; a++ )
                    {
                        word[ a ] = 0;
                    }

                    wordCount += wordShift;
                }

                if ( bitShift != 0 )
                {
                    Multiply( 1U << bitShift );
                }
            }

            void ShiftRightOne() noexcept
            {
                for ( int32 a = 0; a < wordCount; a++ )
                {
                    word[ a ] = ( word[ a ] >> 1 ) | ( ( a + 1 < wordCount ) ? ( word[ a + 1 ] << 31 ) : 0 );
                }

                Trim();
            }

            void MultiplyPower5( int32 exponent ) noexcept
            {
                for ( ; exponent >= 13; exponent -= 13 )
                {
                    Multiply( 1220703125U );
                }

                uint32 factor = 1;

                for ( ; exponent > 0; exponent-- )
                {
                    factor *= 5;
                }

                Multiply( factor );
            }

            void MultiplyPower10( int32 exponent ) noexcept
            {
                MultiplyPower5( exponent );
                MultiplyPower2( exponent );
            }

            int32 Compare( const BigInteger& other ) const noexcept
            {
                if ( wordCount != other.wordCount )
                {
                    return ( wordCount < other.wordCount ) ? -1 : 1;
                }

                for ( int32 a = wordCount - 1; a >= 0; a-- )
                {
                    if ( word[ a ] != other.word[ a ] )
                    {
                        return ( word[ a ] < other.word[ a ] ) ? -1 : 1;
                    }
                }

                return 0;
            }

            void Subtract( const BigInteger& other ) noexcept
            {
                int64 borrow = 0;

                for ( int32 a = 0; a < wordCount; a++ )
                {
                    int64 difference = int64( word[ a ] ) - ( ( a < other.wordCount ) ? int64( other.word[ a ] ) : 0 ) - borrow;
                    borrow           = ( difference < 0 ) ? 1 : 0;
                    word[ a ]        = uint32( difference + ( borrow << 32 ) );
                }

                Trim();
            }

            int32 ConvertToDecimal( char* digits ) noexcept
            {
                constexpr int32 maxDigitCount = 1200;

                char  reversed[ maxDigitCount ];
                int32 count = 0;

                while ( wordCount != 0 )
                {
                    uint32 chunk = Divide( 1000000000U );

                    for ( int32 a = 0; a < 9; a++ )
                    {
                        reversed[ count++ ] = char( '0' + chunk % 10 );
                        chunk /= 10;
                    }
                }

                while ( count > 1 && reversed[ count - 1 ] == '0' )
                {
                    count--;
                }

                for ( int32 a = 0; a < count; a++ )
                {
                    digits[ a ] = reversed[ count - 1 - a ];
                }

                return count;
            }
        };

        int32 CompareDecimal( const char* a, int32 aLength, const char* b, int32 bLength ) noexcept
        {
            if ( aLength != bLength )
            {
                return ( aLength < bLength ) ? -1 : 1;
            }

            for ( int32 i = 0; i < aLength; i++ )
            {
                if ( a[ i ] != b[ i ] )
                {
                    return ( a[ i ] < b[ i ] ) ? -1 : 1;
                }
            }

            return 0;
        }

        void FormatDouble( double value, char* output ) noexcept
        {
            uint64      bits;
            const char* source      = reinterpret_cast<const char*>( &value );
            char*       destination = reinterpret_cast<char*>( &bits );

            for ( machine a = 0; a < 8; a++ )
            {
                destination[ a ] = source[ a ];
            }

            if ( bits >> 63 )
            {
                *output++ = '-';
            }

            int32  biasedExponent = int32( ( bits >> 52 ) & 0x07FF );
            uint64 mantissa       = bits & 0x000FFFFFFFFFFFFFULL;

            if ( biasedExponent == 0 && mantissa == 0 )
            {
                output[ 0 ] = '0';
                output[ 1 ] = 0;
                return;
            }

            int32 exponent = 1 - 1075;

            if ( biasedExponent != 0 )
            {
                mantissa |= 0x0010000000000000ULL;
                exponent = biasedExponent - 1075;
            }

            bool narrowLowerGap = ( mantissa == 0x0010000000000000ULL ) && ( biasedExponent > 1 );
            bool includeBounds  = ( mantissa & 1 ) == 0;

            uint64 bound[ 3 ] = { 4 * mantissa - ( narrowLowerGap ? 1 : 2 ), 4 * mantissa, 4 * mantissa + 2 };

            constexpr int32 maxDigitCount = 800;

            char  digits[ 3 ][ maxDigitCount ];
            int32 digitCount[ 3 ];

            int32 scaleExponent = exponent - 2;
            int32 decimalScale  = ( scaleExponent < 0 ) ? -scaleExponent : 0;

            for ( machine a = 0; a < 3; a++ )
            {
                BigInteger integer( bound[ a ] );

                if ( scaleExponent >= 0 )
                {
                    integer.MultiplyPower2( scaleExponent );
                }
                else
                {
                    integer.MultiplyPower5( -scaleExponent );
                }

                digitCount[ a ] = integer.ConvertToDecimal( digits[ a ] );
            }

            const char* valueDigits = digits[ 1 ];
            int32       valueLength = digitCount[ 1 ];

            char  candidate[ maxDigitCount + 1 ];
            int32 candidateLength  = 0;
            int32 significantCount = 0;

            for ( int32 precision = 1; precision <= 17; precision++ )
            {
                int32 kept = ( precision < valueLength ) ? precision : valueLength;

                for ( int32 a = 0; a < kept; a++ )
                {
                    candidate[ a + 1 ] = valueDigits[ a ];
                }

                bool roundUp = false;

                if ( kept < valueLength )
                {
                    char next = valueDigits[ kept ];

                    if ( next > '5' )
                    {
                        roundUp = true;
                    }
                    else if ( next == '5' )
                    {
                        bool tail = false;

                        for ( int32 a = kept + 1; a < valueLength; a++ )
                        {
                            tail |= ( valueDigits[ a ] != '0' );
                        }

                        roundUp = tail || ( ( valueDigits[ kept - 1 ] - '0' ) & 1 );
                    }
                }

                candidate[ 0 ] = '0';

                if ( roundUp )
                {
                    int32 a = kept;

                    while ( candidate[ a ] == '9' )
                    {
                        candidate[ a-- ] = '0';
                    }

                    candidate[ a ]++;
                }

                char* first     = ( candidate[ 0 ] == '0' ) ? candidate + 1 : candidate;
                int32 firstKept = ( candidate[ 0 ] == '0' ) ? kept : kept + 1;

                candidateLength = valueLength + ( first == candidate ? 1 : 0 );

                for ( int32 a = firstKept; a < candidateLength; a++ )
                {
                    first[ a ] = '0';
                }

                int32 lowerLimit = includeBounds ? 0 : 1;
                int32 upperLimit = includeBounds ? 0 : -1;

                if ( precision == 17 || ( CompareDecimal( first, candidateLength, digits[ 0 ], digitCount[ 0 ] ) >= lowerLimit &&
                                          CompareDecimal( first, candidateLength, digits[ 2 ], digitCount[ 2 ] ) <= upperLimit ) )
                {
                    if ( first != candidate )
                    {
                        for ( int32 a = 0; a < candidateLength; a++ )
                        {
                            candidate[ a ] = first[ a ];
                        }
                    }

                    significantCount = firstKept;
                    break;
                }
            }

            while ( significantCount > 1 && candidate[ significantCount - 1 ] == '0' )
            {
                significantCount--;
            }

            int32 pointPosition = candidateLength - decimalScale;
            int32 exponent10    = pointPosition - 1;

            if ( exponent10 >= -6 && exponent10 < 21 )
            {
                if ( pointPosition <= 0 )
                {
                    *output++ = '0';
                    *output++ = '.';

                    for ( int32 a = 0; a < -pointPosition; a++ )
                    {
                        *output++ = '0';
                    }

                    for ( int32 a = 0; a < significantCount; a++ )
                    {
                        *output++ = candidate[ a ];
                    }
                }
                else
                {
                    for ( int32 a = 0; a < significantCount || a < pointPosition; a++ )
                    {
                        if ( a == pointPosition )
                        {
                            *output++ = '.';
                        }

                        *output++ = ( a < significantCount ) ? candidate[ a ] : '0';
                    }
                }
            }
            else
            {
                *output++ = candidate[ 0 ];

                if ( significantCount > 1 )
                {
                    *output++ = '.';

                    for ( int32 a = 1; a < significantCount; a++ )
                    {
                        *output++ = candidate[ a ];
                    }
                }

                *output++ = 'e';
                *output++ = ( exponent10 < 0 ) ? '-' : '+';

                int32 magnitude = ( exponent10 < 0 ) ? -exponent10 : exponent10;
                char  exponentDigits[ 4 ];
                int32 exponentLength = 0;

                do
                {
                    exponentDigits[ exponentLength++ ] = char( '0' + magnitude % 10 );
                    magnitude /= 10;
                } while ( magnitude != 0 );

                while ( exponentLength > 0 )
                {
                    *output++ = exponentDigits[ --exponentLength ];
                }
            }

            *output = 0;
        }

#endif

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

        Status AppendText( Array<char>& output, const char* text, int32 length )
        {
            int32 count = output.GetArrayElementCount();

            if ( length > JSON4C4_MAX_FILE_SIZE - count )
            {
                return Status::kFileTooLarge;
            }

            output.SetArrayElementCount( count + length );

            for ( int32 a = 0; a < length; a++ )
            {
                output[ count + a ] = text[ a ];
            }

            return Status::kOk;
        }

        Status AppendText( Array<char>& output, const char* text )
        {
            return AppendText( output, text, Text::GetTextLength( text ) );
        }

        Status AppendEscapedString( Array<char>& output, const char* text )
        {
            static const char hexadecimalDigit[] = "0123456789abcdef";

            Status status = AppendText( output, "\"", 1 );

            for ( const uint8* byte = reinterpret_cast<const uint8*>( text ); status == Status::kOk && byte[ 0 ] != 0; )
            {
                uint8 c = byte[ 0 ];

                char        escape[ 6 ] = { '\\', char( c ), 0, 0, 0, 0 };
                const char* piece       = escape;
                int32       pieceLength = 2;

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
                    escape[ 1 ] = 'u';
                    escape[ 2 ] = '0';
                    escape[ 3 ] = '0';
                    escape[ 4 ] = hexadecimalDigit[ c >> 4 ];
                    escape[ 5 ] = hexadecimalDigit[ c & 0x0F ];
                    pieceLength = 6;
                }
                else if ( ( c != '"' ) && ( c != '\\' ) )
                {
                    piece       = reinterpret_cast<const char*>( byte );
                    pieceLength = Text::ValidateUnicodeChar( piece );

                    if ( ( pieceLength == 0 ) || ( ( c == 0xEDU ) && ( byte[ 1 ] >= 0xA0U ) ) )
                    {
                        return Status::kStringIllegalChar;
                    }
                }

                status = AppendText( output, piece, pieceLength );
                byte += ( piece == escape ) ? 1 : pieceLength;
            }

            return ( status == Status::kOk ) ? AppendText( output, "\"", 1 ) : status;
        }

        int32 ComputeWhitespaceLength( const char* text ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            while ( byte[ 0 ] != 0 && ( byte[ 0 ] == space || byte[ 0 ] == tab || byte[ 0 ] == newLine || byte[ 0 ] == carriageReturn ) )
            {
                byte++;
            }

            return int32( reinterpret_cast<const char*>( byte ) - text );
        }

        inline bool IsDigit( uint32 c ) noexcept
        {
            return ( c - '0' ) < 10U;
        }

        Status ScanNumberLiteral( const char* text, int32* textLength ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            if ( byte[ 0 ] == '-' )
            {
                byte++;
            }

            if ( byte[ 0 ] == '0' )
            {
                byte++;

                if ( IsDigit( byte[ 0 ] ) )
                {
                    return ( Status::kNumbersCannotHaveLeadingZeros );
                }
            }
            else if ( IsDigit( byte[ 0 ] ) )
            {
                while ( IsDigit( byte[ 0 ] ) )
                {
                    byte++;
                }
            }
            else
            {
                return ( Status::kFloatInvalid );
            }

            if ( byte[ 0 ] == '.' )
            {
                byte++;

                if ( !IsDigit( byte[ 0 ] ) )
                {
                    return ( Status::kFloatInvalid );
                }

                while ( IsDigit( byte[ 0 ] ) )
                {
                    byte++;
                }
            }

            if ( ( byte[ 0 ] == 'e' ) || ( byte[ 0 ] == 'E' ) )
            {
                byte++;

                if ( ( byte[ 0 ] == '+' ) || ( byte[ 0 ] == '-' ) )
                {
                    byte++;
                }

                if ( !IsDigit( byte[ 0 ] ) )
                {
                    return ( Status::kFloatInvalid );
                }

                while ( IsDigit( byte[ 0 ] ) )
                {
                    byte++;
                }
            }

            *textLength = int32( reinterpret_cast<const char*>( byte ) - text );

            return ( Status::kOk );
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

        Status ReadStringLiteral( const char* text, int32* textLength, int32* stringLength, char* restrict string = nullptr ) noexcept
        {
            const uint8* byte   = reinterpret_cast<const uint8*>( text );
            int32        count  = 0;
            Status       status = Status::kOk;

            for ( ;; )
            {
                uint32 c = byte[ 0 ];
                if ( c == 0 )
                {
                    status = Status::kPrematureNullTerminator;
                    break;
                }

                if ( c == '\"' )
                {
                    break;
                }

                if ( c < 32U )
                {
                    status = Status::kStringIllegalChar;
                    break;
                }

                if ( c != '\\' )
                {
                    int32 len = Text::ValidateUnicodeChar( reinterpret_cast<const char*>( byte ) );
                    if ( ( len == 0 ) || ( ( c == 0xEDU ) && ( byte[ 1 ] >= 0xA0U ) ) )
                    {
                        status = Status::kStringIllegalChar;
                        break;
                    }

                    if ( string )
                    {
                        for ( machine a = 0; a < len; a++ )
                        {
                            string[ a ] = char( byte[ a ] );
                        }

                        string += len;
                    }

                    byte += len;
                    count += len;
                }
                else
                {
                    int32 stringLen;

                    int32 textLen = ReadStringEscapeChar( reinterpret_cast<const char*>( byte + 1 ), &stringLen, string );
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
            }

            *textLength   = int32( reinterpret_cast<const char*>( byte ) - text );
            *stringLength = count;

            return ( status );
        }

#ifndef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

        double ConvertDecimalToDouble( const char* text ) noexcept
        {
            constexpr int32 maxSignificantDigitCount = 800;

            const uint8* byte     = reinterpret_cast<const uint8*>( text );
            bool         negative = ( byte[ 0 ] == '-' );

            if ( negative )
            {
                byte++;
            }

            BigInteger significand;
            int32      digitCount      = 0;
            int64      decimalExponent = 0;
            bool       truncated       = false;
            bool       fraction        = false;

            for ( ;; byte++ )
            {
                if ( byte[ 0 ] == '.' )
                {
                    fraction = true;
                    continue;
                }

                if ( !IsDigit( byte[ 0 ] ) )
                {
                    break;
                }

                uint32 digit = byte[ 0 ] - '0';

                if ( digitCount == 0 && digit == 0 )
                {
                    decimalExponent -= fraction ? 1 : 0;
                    continue;
                }

                if ( digitCount < maxSignificantDigitCount )
                {
                    significand.MultiplyAdd( 10, digit );
                    digitCount++;
                    decimalExponent -= fraction ? 1 : 0;
                }
                else
                {
                    truncated |= ( digit != 0 );
                    decimalExponent += fraction ? 0 : 1;
                }
            }

            if ( ( byte[ 0 ] == 'e' ) || ( byte[ 0 ] == 'E' ) )
            {
                byte++;

                bool negativeExponent = ( byte[ 0 ] == '-' );
                if ( ( byte[ 0 ] == '-' ) || ( byte[ 0 ] == '+' ) )
                {
                    byte++;
                }

                int64 exponent = 0;

                for ( ; IsDigit( byte[ 0 ] ); byte++ )
                {
                    if ( exponent < 1000000000000 )
                    {
                        exponent = exponent * 10 + int64( byte[ 0 ] - '0' );
                    }
                }

                decimalExponent += negativeExponent ? -exponent : exponent;
            }

            uint64 bits = negative ? 0x8000000000000000ULL : 0;

            if ( digitCount != 0 )
            {
                if ( truncated )
                {
                    significand.MultiplyAdd( 10, 1 );
                    decimalExponent--;
                }

                int64 magnitude = digitCount + ( truncated ? 1 : 0 ) + decimalExponent - 1;

                if ( magnitude > 309 )
                {
                    bits |= 0x7FF0000000000000ULL;
                }
                else if ( magnitude >= -330 )
                {
                    BigInteger value          = significand;
                    int32      binaryExponent = 0;
                    bool       sticky         = false;

                    if ( decimalExponent >= 0 )
                    {
                        value.MultiplyPower10( int32( decimalExponent ) );
                    }
                    else
                    {
                        BigInteger divisor( 1 );
                        divisor.MultiplyPower10( int32( -decimalExponent ) );

                        int32 shift = divisor.GetBitCount() - value.GetBitCount() + 56;
                        if ( shift > 0 )
                        {
                            value.MultiplyPower2( shift );
                            binaryExponent = -shift;
                        }

                        int32 position = value.GetBitCount() - divisor.GetBitCount();

                        BigInteger quotient;

                        if ( position >= 0 )
                        {
                            divisor.MultiplyPower2( position );

                            for ( ; position >= 0; position-- )
                            {
                                quotient.MultiplyPower2( 1 );

                                if ( value.Compare( divisor ) >= 0 )
                                {
                                    value.Subtract( divisor );
                                    quotient.AddSmall( 1 );
                                }

                                divisor.ShiftRightOne();
                            }
                        }

                        sticky = !value.IsZero();
                        value  = quotient;
                    }

                    int32 bitCount = value.GetBitCount();
                    int32 shift    = Max( bitCount - 53, -1074 - binaryExponent );

                    uint64 mantissa;

                    if ( shift > 0 )
                    {
                        mantissa = value.GetBits( shift, Min( bitCount - shift, 53 ) );

                        bool half = value.GetBit( shift - 1 );
                        bool rest = sticky || value.AnyBitBelow( shift - 1 );

                        if ( half && ( rest || ( mantissa & 1 ) ) )
                        {
                            mantissa++;

                            if ( mantissa == 0x0020000000000000ULL )
                            {
                                mantissa >>= 1;
                                shift++;
                            }
                        }
                    }
                    else
                    {
                        mantissa = value.GetBits( 0, bitCount ) << -shift;
                    }

                    int32 exponent2 = binaryExponent + shift;

                    if ( mantissa >= 0x0010000000000000ULL )
                    {
                        int32 biasedExponent = exponent2 + 1075;

                        if ( biasedExponent >= 2047 )
                        {
                            bits |= 0x7FF0000000000000ULL;
                        }
                        else
                        {
                            bits |= ( uint64( biasedExponent ) << 52 ) | ( mantissa & 0x000FFFFFFFFFFFFFULL );
                        }
                    }
                    else
                    {
                        bits |= mantissa;
                    }
                }
            }

            double result;
            char*  destination = reinterpret_cast<char*>( &result );
            char*  source      = reinterpret_cast<char*>( &bits );

            for ( machine a = 0; a < 8; a++ )
            {
                destination[ a ] = source[ a ];
            }

            return result;
        }

#endif

        Status ReadDoubleLiteral( const char* text, int32* textLength, double* value )
        {
            Status status = ScanNumberLiteral( text, textLength );

            if ( status != Status::kOk )
            {
                return ( status );
            }

#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

            constexpr int32 localBufferSize = 64;
            char            localBuffer[ localBufferSize ];

            const char* decimalPoint       = localeconv()->decimal_point;
            int32       decimalPointLength = Text::GetTextLength( decimalPoint );

            if ( decimalPointLength == 0 )
            {
                decimalPoint       = ".";
                decimalPointLength = 1;
            }

            int32 bufferLength = *textLength + decimalPointLength;
            char* buffer       = ( bufferLength < localBufferSize ) ? localBuffer : new char[ bufferLength ];
            char* c            = buffer;

            for ( int32 a = 0; a < *textLength; a++ )
            {
                if ( text[ a ] == '.' )
                {
                    for ( int32 b = 0; b < decimalPointLength; b++ )
                    {
                        *c++ = decimalPoint[ b ];
                    }
                }
                else
                {
                    *c++ = text[ a ];
                }
            }

            *c = 0;

            *value = strtod( buffer, nullptr );

            if ( buffer != localBuffer )
            {
                delete[] buffer;
            }

#else

            *value = ConvertDecimalToDouble( text );

#endif

            return ( Status::kOk );
        }

        template <class T>
        Status ParseValue( Value*& value, const char*& text, const ParseOptions& options, int32 depth ) noexcept
        {
            T* jsonValue = nullptr;

            Status status = MayThrow(
                [ & ]()
                {
                    jsonValue = new T;
                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                return status;
            }

            if constexpr ( Detail::IsSame<T, ArrayValue>::Value || Detail::IsSame<T, ObjectValue>::Value )
            {
                status = T::Parse( jsonValue, text, options, depth );
            }
            else
            {
                status = T::Parse( jsonValue, text );
            }

            if ( status == Status::kOk )
            {
                value = jsonValue;

                return status;
            }

            delete jsonValue;

            return status;
        };

        Status ParseAnyValue( Value*& value, const char*& text, const ParseOptions& options, int32 depth ) noexcept
        {
            switch ( text[ 0 ] )
            {
                case '"':
                    return ParseValue<StringValue>( value, text, options, depth );

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
                    return ParseValue<NumberValue>( value, text, options, depth );

                case 't':
                case 'f':
                    return ParseValue<BoolValue>( value, text, options, depth );

                case 'n':
                    return ParseValue<NullValue>( value, text, options, depth );

                case '[':
                    if ( depth >= options.maxNestingDepth )
                    {
                        return Status::kMaximumNestingDepthExceeded;
                    }

                    return ParseValue<ArrayValue>( value, text, options, depth + 1 );

                case '{':
                    if ( depth >= options.maxNestingDepth )
                    {
                        return Status::kMaximumNestingDepthExceeded;
                    }

                    return ParseValue<ObjectValue>( value, text, options, depth + 1 );

                default:
                    return Status::kExpectedJsonValue;
            }
        }

        class StringValue final : public Value
        {
        public:
            static Status Parse( StringValue* jsonStringValue, const char*& text ) noexcept
            {
                if ( text[ 0 ] != '"' )
                {
                    return ( Status::kExpectedBeginingDoubleQuotes );
                }

                int32 textLength;
                int32 stringLength;

                text++;

                Status status = ReadStringLiteral( text, &textLength, &stringLength );
                if ( status != Status::kOk )
                {
                    text += textLength;
                    return ( status );
                }

                if ( jsonStringValue )
                {
                    status = MayThrow(
                        [ & ]()
                        {
                            jsonStringValue->data.SetStringLength( stringLength );
                            return Status::kOk;
                        } );

                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    ReadStringLiteral( text, &textLength, &stringLength, jsonStringValue->data );
                }

                text += textLength + 1;

                return ( Status::kOk );
            }

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
            static Status Parse( NumberValue* value, const char*& text ) noexcept
            {
                int32  length;
                double doubleData;

                Status status = MayThrow( [ & ]() { return ReadDoubleLiteral( text, &length, &doubleData ); } );

                if ( status != Status::kOk )
                {
                    return ( status );
                }

                if ( value )
                {
                    value->data = doubleData;
                }

                text += length;

                return ( Status::kOk );
            }

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

#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

                constexpr int maxDoubleDigits = 32;
                char          text[ maxDoubleDigits ];

                for ( int precision = 15; precision <= 17; precision++ )
                {
                    snprintf( text, maxDoubleDigits, "%.*g", precision, data );

                    if ( strtod( text, nullptr ) == data )
                    {
                        break;
                    }
                }

                const char* decimalPoint       = localeconv()->decimal_point;
                int32       decimalPointLength = Text::GetTextLength( decimalPoint );

                if ( decimalPointLength != 0 && ( decimalPointLength != 1 || decimalPoint[ 0 ] != '.' ) )
                {
                    if ( char* found = strstr( text, decimalPoint ) )
                    {
                        found[ 0 ] = '.';

                        for ( char* c = found + 1;; c++ )
                        {
                            c[ 0 ] = c[ decimalPointLength - 1 ];

                            if ( c[ 0 ] == 0 )
                            {
                                break;
                            }
                        }
                    }
                }

                return AppendText( output, text );

#else

                constexpr int maxDoubleLength = 32;
                char          text[ maxDoubleLength ];

                FormatDouble( data, text );

                return AppendText( output, text );

#endif
            }
        };

        class BoolValue final : public Value
        {
        public:
            static Status Parse( BoolValue* value, const char*& text ) noexcept
            {
                if ( text[ 0 ] == 't' && text[ 1 ] == 'r' && text[ 2 ] == 'u' && text[ 3 ] == 'e' )
                {
                    value->data = true;
                    text += 4;

                    return Status::kOk;
                }

                if ( text[ 0 ] == 'f' && text[ 1 ] == 'a' && text[ 2 ] == 'l' && text[ 3 ] == 's' && text[ 4 ] == 'e' )
                {
                    value->data = false;
                    text += 5;

                    return Status::kOk;
                }

                return Status::kExpectedTrueOrFalse;
            }

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
            static Status Parse( NullValue* /*JsonNullValue*/, const char*& text ) noexcept
            {
                if ( text[ 0 ] == 'n' && text[ 1 ] == 'u' && text[ 2 ] == 'l' && text[ 3 ] == 'l' )
                {
                    text += 4;

                    return Status::kOk;
                }

                return Status::kExpectedNull;
            }

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

        Status ArrayValue::Parse( ArrayValue* jsonArray, const char*& text, const ParseOptions& options, int32 depth ) noexcept
        {
            if ( text[ 0 ] != '[' )
            {
                return Status::kExpectedOpeningSquareBrackets;
            }

            bool emptyArray = true;

            for ( ;; )
            {
                text++;
                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] == ']' )
                {
                    if ( !emptyArray )
                    {
                        return Status::kExpectedJsonValue;
                    }

                    text++;

                    return Status::kOk;
                }

                Value* jsonValue = nullptr;
                Status status    = ParseAnyValue( jsonValue, text, options, depth );
                if ( status != Status::kOk )
                {
                    return status;
                }

                status = MayThrow(
                    [ & ]()
                    {
                        jsonArray->data.AppendArrayElement( jsonValue );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    delete jsonValue;
                    return status;
                }

                emptyArray = false;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ',' )
                {
                    break;
                }
            }

            if ( text[ 0 ] != ']' )
            {
                return Status::kExpectedClosingSquareBracketsOrComma;
            }

            text++;

            return Status::kOk;
        }

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
        }

        bool ObjectValue::InsertAccountedMapElement( Value* element ) noexcept
        {
            if ( FindMapElement( element->name ) )
            {
                return false;
            }

            element->insertionSequence = nextInsertionSequence++;

            return this->InsertMapElement( element );
        }

        Value* ObjectValue::RemoveAccountedMapElement( const char* key ) noexcept
        {
            Value* element = FindValue( key );

            if ( element )
            {
                element->Detach();
            }

            return element;
        }

        void ObjectValue::PurgeAccountedMap() noexcept
        {
            PurgeMap();
        }

        Status ObjectValue::Parse( ObjectValue* jsonObject, const char*& text, const ParseOptions& options, int32 depth ) noexcept
        {
            if ( text[ 0 ] != '{' )
            {
                return Status::kExpectedOpeningCurlyBraces;
            }

            bool emptyObject = true;

            for ( ;; )
            {
                text++;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] == '}' )
                {
                    if ( !emptyObject )
                    {
                        return Status::kExpectedJsonValue;
                    }
                    text++;

                    return Status::kOk;
                }

                const char* nameStart = text;

                StringValue name;
                Status      status = StringValue::Parse( &name, text );
                if ( status != Status::kOk )
                {
                    return status;
                }

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ':' )
                {
                    return Status::kExpectedColon;
                }

                text++;
                text += ComputeWhitespaceLength( text );

                Value* jsonValue = nullptr;

                status = ParseAnyValue( jsonValue, text, options, depth );
                if ( status != Status::kOk )
                {
                    return status;
                }

                jsonValue->name = static_cast<String<>&&>( name.data );

                if ( Value* existing = jsonObject->FindMapElement( jsonValue->name ) )
                {
                    if ( options.duplicateNames == DuplicateNames::kReject )
                    {
                        delete jsonValue;
                        text = nameStart;
                        return Status::kDuplicateName;
                    }

                    if ( options.duplicateNames == DuplicateNames::kKeepLast )
                    {
                        jsonValue->insertionSequence = existing->insertionSequence;

                        delete existing;
                        jsonObject->InsertMapElement( jsonValue );
                    }
                    else
                    {
                        delete jsonValue;
                    }
                }
                else
                {
                    status = MayThrow(
                        [ & ]()
                        {
                            jsonObject->InsertAccountedMapElement( jsonValue );
                            return Status::kOk;
                        } );

                    if ( status != Status::kOk )
                    {
                        delete jsonValue;
                        return status;
                    }
                }

                emptyObject = false;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ',' )
                {
                    break;
                }
            }

            if ( text[ 0 ] != '}' )
            {
                return Status::kExpectedClosingCurlyBracesOrComma;
            }

            text++;

            return Status::kOk;
        }

        ObjectValue::~ObjectValue() noexcept
        {
        }

        ObjectValue* ObjectValue::AsJsonObjectValue() noexcept
        {
            return this;
        }

        const ObjectValue* ObjectValue::AsJsonObjectValue() const noexcept
        {
            return this;
        }

        Value* ObjectValue::FindValue( const char* key ) noexcept
        {
            Value* value = nullptr;

            MayThrow(
                [ & ]()
                {
                    value = FindMapElement( key );
                    return Status::kOk;
                } );

            return value;
        }

        const Value* ObjectValue::FindValue( const char* key ) const noexcept
        {
            const Value* value = nullptr;

            MayThrow(
                [ & ]()
                {
                    value = FindMapElement( key );
                    return Status::kOk;
                } );

            return value;
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
            int32 count = GetMapElementCount();

            Array<const Value*> members;
            Array<const Value*> scratch;

            members.SetArrayElementCount( count );
            scratch.SetArrayElementCount( count );

            int32 index = 0;

            for ( const Value* member : *this )
            {
                members[ index++ ] = member;
            }

            for ( int64 width = 1; width < count; width *= 2 )
            {
                for ( int64 low = 0; low < count; low += 2 * width )
                {
                    int32 middle = int32( ( low + width < count ) ? low + width : count );
                    int32 high   = int32( ( low + 2 * width < count ) ? low + 2 * width : count );
                    int32 i      = int32( low );
                    int32 j      = middle;

                    for ( int32 k = int32( low ); k < high; k++ )
                    {
                        bool takeRight = ( j < high ) && ( ( i >= middle ) || ( members[ j ]->insertionSequence < members[ i ]->insertionSequence ) );
                        scratch[ k ]   = takeRight ? members[ j++ ] : members[ i++ ];
                    }
                }

                for ( int32 a = 0; a < count; a++ )
                {
                    members[ a ] = scratch[ a ];
                }
            }

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

        Status ParseJsonRoot(
            Value*& jsonRoot, const Array<char>& nullTerminatedCharArray, const ParseOptions& options, int32* errorLine, int32* errorColumn ) noexcept
        {
            if ( nullTerminatedCharArray.GetArrayElementCount() < 2 )
            {
                return Status::kTextBufferContentsInvalid;
            }

            if ( *( nullTerminatedCharArray.end() - 1 ) != 0 )
            {
                return Status::kUnterminatedTextBuffer;
            }

            const char* begin = &nullTerminatedCharArray[ 0 ];
            const char* end   = nullTerminatedCharArray.end() - 1;
            const char* text  = begin + ComputeWhitespaceLength( begin );

            Status status = ParseAnyValue( jsonRoot, text, options, 0 );

            if ( status == Status::kOk )
            {
                text += ComputeWhitespaceLength( text );

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
                    if ( ( c[ 0 ] == '\n' ) || ( ( c[ 0 ] == '\r' ) && ( c[ 1 ] != '\n' ) ) )
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
            delete rootJsonValue;
            rootJsonValue = nullptr;

            ParseResult parseResult;
            parseResult.status = ParseJsonRoot( rootJsonValue, nullTerminatedTextBuffer, options, &parseResult.errorLine, &parseResult.errorColumn );

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
