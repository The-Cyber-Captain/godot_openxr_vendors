/**************************************************************************/
/*  openxr_monotonic_time_conversion_extension.h                          */
/**************************************************************************/

#ifndef OPENXR_MONOTONIC_TIME_CONVERSION_EXTENSION_H
#define OPENXR_MONOTONIC_TIME_CONVERSION_EXTENSION_H

// XR_KHR_convert_timespec_time is guarded by XR_USE_TIMESPEC in the
// Khronos headers. Android is the only platform on which this bridge is
// implemented; keeping the guard here also keeps desktop builds portable.
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
#ifndef XR_USE_TIMESPEC
#define XR_USE_TIMESPEC
#endif
#endif

#include <openxr/openxr.h>
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
#include <openxr/openxr_platform.h>
#endif
#include <godot_cpp/classes/open_xr_extension_wrapper.hpp>
#include <godot_cpp/classes/open_xrapi_extension.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "util.h"

class OpenXRMonotonicTimeConversionExtension : public godot::OpenXRExtensionWrapper {
	GDCLASS(OpenXRMonotonicTimeConversionExtension, godot::OpenXRExtensionWrapper);

public:
	static OpenXRMonotonicTimeConversionExtension *get_singleton();

	OpenXRMonotonicTimeConversionExtension();
	~OpenXRMonotonicTimeConversionExtension() override;

	godot::Dictionary _get_requested_extensions(uint64_t p_xr_version) override;
	void _on_instance_created(uint64_t p_instance) override;
	void _on_instance_destroyed() override;

	bool is_monotonic_time_conversion_supported() const;
	int64_t get_android_uptime_nanos() const;
	int64_t monotonic_nanos_to_xr_time(int64_t p_monotonic_nanos) const;

protected:
	static void _bind_methods();

private:
	static OpenXRMonotonicTimeConversionExtension *singleton;

	void cleanup();

#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	bool convert_timespec_time_enabled = false;
	XrInstance instance = XR_NULL_HANDLE;
	PFN_xrConvertTimespecTimeToTimeKHR xrConvertTimespecTimeToTimeKHR_ptr = nullptr;
#endif
};

#endif // OPENXR_MONOTONIC_TIME_CONVERSION_EXTENSION_H
