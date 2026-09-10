/**************************************************************************/
/*  openxr_monotonic_time_conversion_extension.cpp                        */
/**************************************************************************/

#include "extensions/openxr_monotonic_time_conversion_extension.h"

#include <limits>

#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
#include <time.h>
#endif

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

OpenXRMonotonicTimeConversionExtension *OpenXRMonotonicTimeConversionExtension::singleton = nullptr;

OpenXRMonotonicTimeConversionExtension *OpenXRMonotonicTimeConversionExtension::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(OpenXRMonotonicTimeConversionExtension());
	}
	return singleton;
}

OpenXRMonotonicTimeConversionExtension::OpenXRMonotonicTimeConversionExtension() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "An OpenXRMonotonicTimeConversionExtension singleton already exists.");
	singleton = this;
}

OpenXRMonotonicTimeConversionExtension::~OpenXRMonotonicTimeConversionExtension() {
	cleanup();
	singleton = nullptr;
}

void OpenXRMonotonicTimeConversionExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_monotonic_time_conversion_supported"), &OpenXRMonotonicTimeConversionExtension::is_monotonic_time_conversion_supported);
	ClassDB::bind_method(D_METHOD("get_android_uptime_nanos"), &OpenXRMonotonicTimeConversionExtension::get_android_uptime_nanos);
	ClassDB::bind_method(D_METHOD("monotonic_nanos_to_xr_time", "monotonic_nanos"), &OpenXRMonotonicTimeConversionExtension::monotonic_nanos_to_xr_time);
}

Dictionary OpenXRMonotonicTimeConversionExtension::_get_requested_extensions(uint64_t p_xr_version) {
	Dictionary result;
	(void)p_xr_version;
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	// The bool pointer is filled by Godot's OpenXR extension negotiation before
	// instance creation. There is intentionally no project-setting opt-out:
	// this bridge is vendor-neutral and harmless when unsupported.
	result[XR_KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME] = (uint64_t)&convert_timespec_time_enabled;
#endif
	return result;
}

void OpenXRMonotonicTimeConversionExtension::_on_instance_created(uint64_t p_instance) {
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	instance = (XrInstance)p_instance;
	if (!convert_timespec_time_enabled || instance == XR_NULL_HANDLE) {
		cleanup();
		return;
	}

	xrConvertTimespecTimeToTimeKHR_ptr = reinterpret_cast<PFN_xrConvertTimespecTimeToTimeKHR>(get_openxr_api()->get_instance_proc_addr("xrConvertTimespecTimeToTimeKHR"));
	if (xrConvertTimespecTimeToTimeKHR_ptr == nullptr) {
		cleanup();
	}
#else
	(void)p_instance;
#endif
}

void OpenXRMonotonicTimeConversionExtension::_on_instance_destroyed() {
	cleanup();
}

void OpenXRMonotonicTimeConversionExtension::cleanup() {
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	convert_timespec_time_enabled = false;
	instance = XR_NULL_HANDLE;
	xrConvertTimespecTimeToTimeKHR_ptr = nullptr;
#endif
}

bool OpenXRMonotonicTimeConversionExtension::is_monotonic_time_conversion_supported() const {
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	return convert_timespec_time_enabled && instance != XR_NULL_HANDLE && xrConvertTimespecTimeToTimeKHR_ptr != nullptr;
#else
	return false;
#endif
}

int64_t OpenXRMonotonicTimeConversionExtension::get_android_uptime_nanos() const {
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= 1000000000L) {
		return -1;
	}
	const int64_t seconds = static_cast<int64_t>(now.tv_sec);
	if (seconds > (std::numeric_limits<int64_t>::max() - now.tv_nsec) / 1000000000LL) {
		return -1;
	}
	return seconds * 1000000000LL + static_cast<int64_t>(now.tv_nsec);
#else
	return -1;
#endif
}

int64_t OpenXRMonotonicTimeConversionExtension::monotonic_nanos_to_xr_time(int64_t p_monotonic_nanos) const {
#if defined(__ANDROID__) || defined(ANDROID_ENABLED)
	if (!is_monotonic_time_conversion_supported() || p_monotonic_nanos <= 0) {
		return -1;
	}

	const int64_t seconds = p_monotonic_nanos / 1000000000LL;
	const int64_t nanoseconds = p_monotonic_nanos % 1000000000LL;
	if (seconds > static_cast<int64_t>(std::numeric_limits<time_t>::max())) {
		return -1;
	}

	struct timespec source_time = {};
	source_time.tv_sec = static_cast<time_t>(seconds);
	source_time.tv_nsec = static_cast<long>(nanoseconds);
	XrTime xr_time = 0;
	if (XR_FAILED(xrConvertTimespecTimeToTimeKHR_ptr(instance, &source_time, &xr_time))) {
		return -1;
	}
	return static_cast<int64_t>(xr_time);
#else
	(void)p_monotonic_nanos;
	return -1;
#endif
}
