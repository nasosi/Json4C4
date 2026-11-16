// VortexXR's manifest structs and calls (VortexXrOpenXrInstanceManager.cpp), kept as a consumer test that each API
// change updates on purpose.

#include "TestCheck.h"

using Terathon::Array;
using Terathon::String;

struct RuntimeDescriptor
{
    String<> libraryPath;
    String<> name;
};

#define OPENXR_RUNTIME_DESCRIPTOR_PROTO "library_path", object.libraryPath, "name", object.name
DEFINE_JSON4C4_FUNCTIONS( RuntimeDescriptor, OPENXR_RUNTIME_DESCRIPTOR_PROTO )

struct OpenXrRuntimeManifest
{
    String<>          fileFormatVersion;
    RuntimeDescriptor runtimeDescriptor;
};

#define OPENXR_MANIFEST_DESCRIPTOR_PROTO "file_format_version", object.fileFormatVersion, "runtime", object.runtimeDescriptor
DEFINE_JSON4C4_FUNCTIONS( OpenXrRuntimeManifest, OPENXR_MANIFEST_DESCRIPTOR_PROTO )

struct ApiLayerInstanceExtensionDescriptor
{
    String<> name;
    String<> extensionVersion;
};

#define OPENXR_API_LATER_INSTANCE_EXTENSION_DESCRIPTOR_PROTO "name", object.name, "extension_version", object.extensionVersion
DEFINE_JSON4C4_FUNCTIONS( ApiLayerInstanceExtensionDescriptor, OPENXR_API_LATER_INSTANCE_EXTENSION_DESCRIPTOR_PROTO )

struct ApiLayerDescriptor
{
    String<>                                   libraryPath;
    String<>                                   name;
    String<>                                   apiVersion;
    String<>                                   implementationVersion;
    String<>                                   description;
    Json::ObjectMap<String<>>                  functionMap;
    Array<ApiLayerInstanceExtensionDescriptor> instanceExtensionArray;
    String<>                                   enableEnvironment;
    String<>                                   disableEnvironment;
};

#define OPENXR_API_LAYER_DESCRIPTOR_PROTO                                                                                                                      \
    "library_path", object.libraryPath, "name", object.name, "api_version", object.apiVersion, "implementation_version", object.implementationVersion,         \
        "description", object.description, "functions", object.functionMap, Json::optional, "instance_extensions", object.instanceExtensionArray,              \
        Json::optional, "enable_environment", object.enableEnvironment, Json::optional, "disable_environment", object.disableEnvironment
DEFINE_JSON4C4_FUNCTIONS( ApiLayerDescriptor, OPENXR_API_LAYER_DESCRIPTOR_PROTO )

struct OpenXrApiLayerManifest
{
    String<>           fileFormatVersion;
    ApiLayerDescriptor apiLayerDescriptor;
};

#define OPENXR_API_LAYER_MANIFEST_PROTO "file_format_version", object.fileFormatVersion, "api_layer", object.apiLayerDescriptor
DEFINE_JSON4C4_FUNCTIONS( OpenXrApiLayerManifest, OPENXR_API_LAYER_MANIFEST_PROTO )

int main()
{
    const char* runtime            = R"({ "file_format_version": "1.0.0", "runtime": { "name": "Vortex", "library_path": "./vortex_runtime.dll" } })";
    const char* layer              = R"({
        "file_format_version": "1.0.0",
        "api_layer": {
            "name": "XR_APILAYER_vortex_validation",
            "library_path": "./vortex_layer.dll",
            "api_version": "1.0",
            "implementation_version": "1",
            "description": "Vortex validation layer",
            "functions": { "xrNegotiateLoaderApiLayerInterface": "VortexNegotiate", "xrGetInstanceProcAddr": "VortexGetProc" },
            "instance_extensions": [ { "name": "XR_EXT_debug_utils", "extension_version": "4" }, { "name": "XR_EXT_hand_tracking", "extension_version": "1" } ],
            "disable_environment": "DISABLE_VORTEX_LAYER"
        } })";
    const char* missingRequired    = R"({ "file_format_version": "1.0.0", "runtime": { "name": "Vortex" } })";
    const char* incompleteOptional = R"({
        "file_format_version": "1.0.0",
        "api_layer": {
            "name": "XR_APILAYER_vortex_validation",
            "library_path": "./vortex_layer.dll",
            "api_version": "1.0",
            "implementation_version": "1",
            "description": "Vortex validation layer",
            "functions": {},
            "instance_extensions": [ { "name": "XR_EXT_debug_utils" } ]
        } })";
    const char* zeroInString =
        "{ \"file_format_version\": \"1.0.0\", \"runtime\": { \"name\": \"Vor\\u0000tex\", \"library_path\": \"./vortex_runtime.dll\" } }";

    Json::StructuredData  sd;
    OpenXrRuntimeManifest runtimeManifest;
    CheckStatus( ParseText( sd, runtime ).status, Json::Status::kOk, "parse a runtime manifest" );
    CheckStatus( Json::Validate( sd, runtimeManifest ), Json::Status::kOk, "validate a runtime manifest" );
    CheckStatus( sd.DeserializeTo( runtimeManifest ), Json::Status::kOk, "deserialize a runtime manifest" );
    Check( runtimeManifest.runtimeDescriptor.libraryPath == "./vortex_runtime.dll" && runtimeManifest.runtimeDescriptor.name == "Vortex", "runtime fields" );

    OpenXrApiLayerManifest* layerManifest = new OpenXrApiLayerManifest;
    CheckStatus( ParseText( sd, layer ).status, Json::Status::kOk, "parse a layer manifest" );
    CheckStatus( Json::Validate( sd, *layerManifest ), Json::Status::kOk, "validate a layer manifest" );
    CheckStatus( sd.DeserializeTo( *layerManifest ), Json::Status::kOk, "deserialize a layer manifest" );

    const ApiLayerDescriptor& d = layerManifest->apiLayerDescriptor;
    Check( d.name == "XR_APILAYER_vortex_validation" && d.disableEnvironment == "DISABLE_VORTEX_LAYER" && d.enableEnvironment.GetStringLength() == 0,
           "layer fields, optional field absent" );
    Check( d.functionMap.GetMapElementCount() == 2 && d.functionMap.FindMapElement( "xrGetInstanceProcAddr" ) &&
               d.functionMap.FindMapElement( "xrGetInstanceProcAddr" )->data == "VortexGetProc",
           "functions map" );
    Check( d.instanceExtensionArray.GetArrayElementCount() == 2 && d.instanceExtensionArray[ 1 ].name == "XR_EXT_hand_tracking" &&
               d.instanceExtensionArray[ 0 ].extensionVersion == "4",
           "instance extensions in document order" );
    delete layerManifest;

    OpenXrRuntimeManifest invalid;
    CheckStatus( ParseText( sd, missingRequired ).status, Json::Status::kOk, "parse a manifest that misses a required field" );
    Check( Json::Validate( sd, invalid ) != Json::Status::kOk, "a missing required field fails validation" );

    OpenXrApiLayerManifest incomplete;
    CheckStatus( ParseText( sd, incompleteOptional ).status, Json::Status::kOk, "parse a layer manifest with an incomplete instance extension" );
    CheckStatus( Json::Validate( sd, incomplete ), Json::Status::kNameNotPresent, "an optional member that is present must be complete to validate" );
    CheckStatus( sd.DeserializeTo( incomplete ), Json::Status::kNameNotPresent, "an optional member that is present must be complete to deserialize" );

    CheckStatus( ParseText( sd, zeroInString ).status, Json::Status::kOk, "parse a manifest with an escaped zero in a string" );
    CheckStatus( Json::Validate( sd, invalid ), Json::Status::kStringHoldsZero, "a String<> refuses an escaped zero" );

    return TestResult();
}
