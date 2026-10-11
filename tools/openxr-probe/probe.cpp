// Standalone diagnostics only: no game injection or runtime-registry changes.
#include <openxr/openxr.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

std::string quote(const char* text) {
    std::string result = "\"";
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (*p == '"' || *p == '\\') result += '\\';
        if (*p >= 32) result += static_cast<char>(*p);
    }
    return result + '"';
}
std::string result_name(XrInstance instance, XrResult result) {
    char text[XR_MAX_RESULT_STRING_SIZE]{};
    if (instance && XR_SUCCEEDED(xrResultToString(instance, result, text))) return quote(text);
    return std::to_string(result);
}
int main() {
    uint32_t count = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr);
    printf("{\n\"enumerate_result\":%d", result);
    if (XR_FAILED(result)) { puts("\n}"); return 1; }
    std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
    result = xrEnumerateInstanceExtensionProperties(nullptr, count, &count, extensions.data());
    if (XR_FAILED(result)) { printf(",\"extension_read_result\":%d\n}\n", result); return 1; }
    printf(",\n\"extensions\":[");
    for (uint32_t i=0; i<count; ++i)
        printf("%s%s", i ? "," : "", quote(extensions[i].extensionName).c_str());
    puts("],");
    XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(create.applicationInfo.applicationName, "GearsVR Standalone Probe");
    strcpy_s(create.applicationInfo.engineName, "GearsVR Research");
    create.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    XrInstance instance = XR_NULL_HANDLE;
    result = xrCreateInstance(&create, &instance);
    printf("\"create_instance_result\":%d", result);
    if (XR_FAILED(result)) { puts("\n}"); return 1; }
    XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};
    result = xrGetInstanceProperties(instance, &props);
    if (XR_SUCCEEDED(result))
        printf(",\n\"runtime_name\":%s,\"runtime_version\":\"%u.%u.%u\"",
               quote(props.runtimeName).c_str(), XR_VERSION_MAJOR(props.runtimeVersion),
               XR_VERSION_MINOR(props.runtimeVersion), XR_VERSION_PATCH(props.runtimeVersion));
    XrSystemGetInfo get_system{XR_TYPE_SYSTEM_GET_INFO}; get_system.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system = XR_NULL_SYSTEM_ID;
    result = xrGetSystem(instance, &get_system, &system);
    printf(",\n\"get_system_result\":%s", result_name(instance, result).c_str());
    int exit_code = 1;
    if (XR_SUCCEEDED(result)) {
        exit_code = 0;
        XrSystemProperties system_props{XR_TYPE_SYSTEM_PROPERTIES};
        result = xrGetSystemProperties(instance, system, &system_props);
        if (XR_SUCCEEDED(result))
            printf(",\n\"system_name\":%s,\"orientation_tracking\":%s,\"position_tracking\":%s",
                   quote(system_props.systemName).c_str(), system_props.trackingProperties.orientationTracking ? "true" : "false",
                   system_props.trackingProperties.positionTracking ? "true" : "false");
        uint32_t views = 0;
        result = xrEnumerateViewConfigurationViews(instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &views, nullptr);
        printf(",\n\"stereo_view_enumeration_result\":%s", result_name(instance, result).c_str());
        if (XR_SUCCEEDED(result)) {
            std::vector<XrViewConfigurationView> configurations(views, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
            result = xrEnumerateViewConfigurationViews(instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                      views, &views, configurations.data());
            printf(",\n\"views\":[");
            if (XR_SUCCEEDED(result))
                for (uint32_t i=0; i<views; ++i)
                    printf("%s{\"width\":%u,\"height\":%u,\"samples\":%u}", i ? "," : "",
                           configurations[i].recommendedImageRectWidth, configurations[i].recommendedImageRectHeight,
                           configurations[i].recommendedSwapchainSampleCount);
            printf("]");
        }
    }
    result = xrDestroyInstance(instance);
    printf(",\n\"destroy_instance_result\":%d\n}\n", result);
    return exit_code;
}
