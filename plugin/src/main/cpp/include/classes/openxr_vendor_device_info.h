/**************************************************************************/
/*  openxr_vendor_device_info.h                                           */
/**************************************************************************/

#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

/// Provides the hardware model and vendor OS identity for the active vendor.
class OpenXRVendorDeviceInfo : public Object {
	GDCLASS(OpenXRVendorDeviceInfo, Object);

	static OpenXRVendorDeviceInfo *singleton;

protected:
	static void _bind_methods();

public:
	static OpenXRVendorDeviceInfo *get_singleton();

	Dictionary get_device_info() const;

	OpenXRVendorDeviceInfo();
	virtual ~OpenXRVendorDeviceInfo();
};
