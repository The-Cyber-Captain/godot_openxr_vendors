/**************************************************************************/
/*  openxr_vendor_device_info.cpp                                         */
/**************************************************************************/

#include "classes/openxr_vendor_device_info.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

OpenXRVendorDeviceInfo *OpenXRVendorDeviceInfo::singleton = nullptr;

void OpenXRVendorDeviceInfo::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_device_info"), &OpenXRVendorDeviceInfo::get_device_info);
}

OpenXRVendorDeviceInfo *OpenXRVendorDeviceInfo::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(OpenXRVendorDeviceInfo());
	}
	return singleton;
}

Dictionary OpenXRVendorDeviceInfo::get_device_info() const {
	Dictionary info;
	info["model"] = String();
	info["os"] = String();
	info["os_version"] = String();

#ifdef ANDROID_ENABLED
	Object *godot_openxr = Engine::get_singleton()->get_singleton("GodotOpenXR");
	if (godot_openxr == nullptr) {
		return info;
	}

	info["model"] = godot_openxr->call("getDeviceModel");
	info["os"] = godot_openxr->call("getDeviceOs");
	info["os_version"] = godot_openxr->call("getDeviceOsVersion");
#endif

	return info;
}

OpenXRVendorDeviceInfo::OpenXRVendorDeviceInfo() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "An OpenXRVendorDeviceInfo singleton already exists.");
	singleton = this;
}

OpenXRVendorDeviceInfo::~OpenXRVendorDeviceInfo() {
	singleton = nullptr;
}
