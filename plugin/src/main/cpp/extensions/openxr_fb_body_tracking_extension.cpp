/**************************************************************************/
/*  openxr_fb_body_tracking_extension_wrapper.cpp                         */
/**************************************************************************/
/*                       This file is part of:                            */
/*                              GODOT XR                                  */
/*                      https://godotengine.org                           */
/**************************************************************************/
/* Copyright (c) 2022-present Godot XR contributors (see CONTRIBUTORS.md) */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "extensions/openxr_fb_body_tracking_extension.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/open_xrapi_extension.hpp>
#include <godot_cpp/classes/xr_server.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

/// Joint mapping table entry
struct JointMapEntry {
	/// Joint in Godot XRBodyTracker
	XRBodyTracker::Joint xr_joint;

	/// Joint in OpenXR XrBodyJointFB or XrFullBodyJointMETA
	int fb_joint;

	/// Joint rotation
	Quaternion rotation;
};

/// Joint mapping table
static const JointMapEntry joint_table[] = {
	// Root joint
	{ XRBodyTracker::JOINT_ROOT, XR_BODY_JOINT_ROOT_FB, Quaternion(0.0, 0.0, 0.0, 1.0) },

	// Upper body joints
	{ XRBodyTracker::JOINT_HIPS, XR_BODY_JOINT_HIPS_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_SPINE, XR_BODY_JOINT_SPINE_LOWER_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_CHEST, XR_BODY_JOINT_SPINE_UPPER_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_UPPER_CHEST, XR_BODY_JOINT_CHEST_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_NECK, XR_BODY_JOINT_NECK_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_HEAD, XR_BODY_JOINT_HEAD_FB, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_LEFT_SHOULDER, XR_BODY_JOINT_LEFT_SHOULDER_FB, Quaternion(0.0, 0.0, 0.7071067811865475244, 0.7071067811865475244) },
	{ XRBodyTracker::JOINT_LEFT_UPPER_ARM, XR_BODY_JOINT_LEFT_ARM_UPPER_FB, Quaternion(-0.7071067811865475244, 0.7071067811865475244, 0.0, 0.0) },
	{ XRBodyTracker::JOINT_LEFT_LOWER_ARM, XR_BODY_JOINT_LEFT_ARM_LOWER_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_RIGHT_SHOULDER, XR_BODY_JOINT_RIGHT_SHOULDER_FB, Quaternion(0.7071067811865475244, 0.7071067811865475244, 0.0, 0.0) },
	{ XRBodyTracker::JOINT_RIGHT_UPPER_ARM, XR_BODY_JOINT_RIGHT_ARM_UPPER_FB, Quaternion(0.0, 0.0, -0.7071067811865475244, 0.7071067811865475244) },
	{ XRBodyTracker::JOINT_RIGHT_LOWER_ARM, XR_BODY_JOINT_RIGHT_ARM_LOWER_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },

	// Left hand joints
	{ XRBodyTracker::JOINT_LEFT_HAND, XR_BODY_JOINT_LEFT_HAND_WRIST_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PALM, XR_BODY_JOINT_LEFT_HAND_PALM_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_WRIST, XR_BODY_JOINT_LEFT_HAND_WRIST_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_THUMB_METACARPAL, XR_BODY_JOINT_LEFT_HAND_THUMB_METACARPAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_THUMB_PHALANX_PROXIMAL, XR_BODY_JOINT_LEFT_HAND_THUMB_PROXIMAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_THUMB_PHALANX_DISTAL, XR_BODY_JOINT_LEFT_HAND_THUMB_DISTAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_THUMB_TIP, XR_BODY_JOINT_LEFT_HAND_THUMB_TIP_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_INDEX_FINGER_METACARPAL, XR_BODY_JOINT_LEFT_HAND_INDEX_METACARPAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_INDEX_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_LEFT_HAND_INDEX_PROXIMAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_INDEX_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_LEFT_HAND_INDEX_INTERMEDIATE_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_INDEX_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_LEFT_HAND_INDEX_DISTAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_INDEX_FINGER_TIP, XR_BODY_JOINT_LEFT_HAND_INDEX_TIP_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_MIDDLE_FINGER_METACARPAL, XR_BODY_JOINT_LEFT_HAND_MIDDLE_METACARPAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_MIDDLE_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_LEFT_HAND_MIDDLE_PROXIMAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_MIDDLE_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_LEFT_HAND_MIDDLE_INTERMEDIATE_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_MIDDLE_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_LEFT_HAND_MIDDLE_DISTAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_MIDDLE_FINGER_TIP, XR_BODY_JOINT_LEFT_HAND_MIDDLE_TIP_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_RING_FINGER_METACARPAL, XR_BODY_JOINT_LEFT_HAND_RING_METACARPAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_RING_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_LEFT_HAND_RING_PROXIMAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_RING_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_LEFT_HAND_RING_INTERMEDIATE_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_RING_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_LEFT_HAND_RING_DISTAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_RING_FINGER_TIP, XR_BODY_JOINT_LEFT_HAND_RING_TIP_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PINKY_FINGER_METACARPAL, XR_BODY_JOINT_LEFT_HAND_LITTLE_METACARPAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PINKY_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_LEFT_HAND_LITTLE_PROXIMAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PINKY_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_LEFT_HAND_LITTLE_INTERMEDIATE_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PINKY_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_LEFT_HAND_LITTLE_DISTAL_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },
	{ XRBodyTracker::JOINT_LEFT_PINKY_FINGER_TIP, XR_BODY_JOINT_LEFT_HAND_LITTLE_TIP_FB, Quaternion(0.5, -0.5, -0.5, -0.5) },

	// Right hand joints
	{ XRBodyTracker::JOINT_RIGHT_HAND, XR_BODY_JOINT_RIGHT_HAND_WRIST_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PALM, XR_BODY_JOINT_RIGHT_HAND_PALM_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_WRIST, XR_BODY_JOINT_RIGHT_HAND_WRIST_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_THUMB_METACARPAL, XR_BODY_JOINT_RIGHT_HAND_THUMB_METACARPAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_THUMB_PHALANX_PROXIMAL, XR_BODY_JOINT_RIGHT_HAND_THUMB_PROXIMAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_THUMB_PHALANX_DISTAL, XR_BODY_JOINT_RIGHT_HAND_THUMB_DISTAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_THUMB_TIP, XR_BODY_JOINT_RIGHT_HAND_THUMB_TIP_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_INDEX_FINGER_METACARPAL, XR_BODY_JOINT_RIGHT_HAND_INDEX_METACARPAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_INDEX_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_RIGHT_HAND_INDEX_PROXIMAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_INDEX_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_RIGHT_HAND_INDEX_INTERMEDIATE_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_INDEX_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_RIGHT_HAND_INDEX_DISTAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_INDEX_FINGER_TIP, XR_BODY_JOINT_RIGHT_HAND_INDEX_TIP_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_MIDDLE_FINGER_METACARPAL, XR_BODY_JOINT_RIGHT_HAND_MIDDLE_METACARPAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_MIDDLE_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_RIGHT_HAND_MIDDLE_PROXIMAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_MIDDLE_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_RIGHT_HAND_MIDDLE_INTERMEDIATE_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_MIDDLE_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_RIGHT_HAND_MIDDLE_DISTAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_MIDDLE_FINGER_TIP, XR_BODY_JOINT_RIGHT_HAND_MIDDLE_TIP_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_RING_FINGER_METACARPAL, XR_BODY_JOINT_RIGHT_HAND_RING_METACARPAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_RING_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_RIGHT_HAND_RING_PROXIMAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_RING_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_RIGHT_HAND_RING_INTERMEDIATE_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_RING_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_RIGHT_HAND_RING_DISTAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_RING_FINGER_TIP, XR_BODY_JOINT_RIGHT_HAND_RING_TIP_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_METACARPAL, XR_BODY_JOINT_RIGHT_HAND_LITTLE_METACARPAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_PHALANX_PROXIMAL, XR_BODY_JOINT_RIGHT_HAND_LITTLE_PROXIMAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_PHALANX_INTERMEDIATE, XR_BODY_JOINT_RIGHT_HAND_LITTLE_INTERMEDIATE_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_PHALANX_DISTAL, XR_BODY_JOINT_RIGHT_HAND_LITTLE_DISTAL_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_TIP, XR_BODY_JOINT_RIGHT_HAND_LITTLE_TIP_FB, Quaternion(0.5, 0.5, -0.5, 0.5) },

	// Lower body joints
	{ XRBodyTracker::JOINT_LEFT_UPPER_LEG, XR_FULL_BODY_JOINT_LEFT_UPPER_LEG_META, Quaternion(0.5, -0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_LEFT_LOWER_LEG, XR_FULL_BODY_JOINT_LEFT_LOWER_LEG_META, Quaternion(-0.5, 0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_LEFT_FOOT, XR_FULL_BODY_JOINT_LEFT_FOOT_ANKLE_META, Quaternion(-0.5, -0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_LEFT_TOES, XR_FULL_BODY_JOINT_LEFT_FOOT_BALL_META, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_UPPER_LEG, XR_FULL_BODY_JOINT_RIGHT_UPPER_LEG_META, Quaternion(-0.5, -0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_LOWER_LEG, XR_FULL_BODY_JOINT_RIGHT_LOWER_LEG_META, Quaternion(0.5, 0.5, -0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_FOOT, XR_FULL_BODY_JOINT_RIGHT_FOOT_ANKLE_META, Quaternion(0.5, -0.5, 0.5, 0.5) },
	{ XRBodyTracker::JOINT_RIGHT_TOES, XR_FULL_BODY_JOINT_RIGHT_FOOT_BALL_META, Quaternion(-0.5, 0.5, 0.5, 0.5) },
};

struct CanonicalBodyJoint {
	Transform3D transform;
	BitField<XRBodyTracker::JointFlags> flags;
	bool populated = false;

	CanonicalBodyJoint() :
			flags(0) {}
};

static int convert_body_joint_locations(const XrBodyJointLocationFB *p_locations, bool p_is_active, bool p_full_body_available, LocalVector<CanonicalBodyJoint> &r_joints) {
	r_joints.resize(XRBodyTracker::JOINT_MAX);
	for (int joint_index = 0; joint_index < XRBodyTracker::JOINT_MAX; joint_index++) {
		r_joints[joint_index] = CanonicalBodyJoint();
	}

	int populated_joint_count = 0;
	for (const JointMapEntry &entry : joint_table) {
		if (!p_full_body_available && entry.fb_joint >= XR_BODY_JOINT_COUNT_FB) {
			continue;
		}

		const XrBodyJointLocationFB &location = p_locations[entry.fb_joint];
		const XrPosef &pose = location.pose;
		CanonicalBodyJoint &joint = r_joints[entry.xr_joint];
		joint.populated = true;
		populated_joint_count++;

		if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
			joint.flags.set_flag(XRBodyTracker::JOINT_FLAG_ORIENTATION_VALID);
			joint.transform.basis = Basis(Quaternion(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w) * entry.rotation);
		}
		if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) {
			joint.flags.set_flag(XRBodyTracker::JOINT_FLAG_ORIENTATION_TRACKED);
		}
		if (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
			joint.flags.set_flag(XRBodyTracker::JOINT_FLAG_POSITION_VALID);
			joint.transform.origin = Vector3(pose.position.x, pose.position.y, pose.position.z);
		}
		if (location.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) {
			joint.flags.set_flag(XRBodyTracker::JOINT_FLAG_POSITION_TRACKED);
		}
	}

	if (p_is_active) {
		Transform3D &hips = r_joints[XRBodyTracker::JOINT_HIPS].transform;
		Vector3 root_y = Vector3(0.0, 1.0, 0.0);
		Vector3 hips_left = hips.basis.get_column(Vector3::AXIS_X);
		Vector3 root_x = (hips_left.slide(Vector3(0.0, 1.0, 0.0))).normalized();
		Vector3 root_z = root_x.cross(root_y);
		Vector3 root_o = r_joints[XRBodyTracker::JOINT_ROOT].transform.origin;
		r_joints[XRBodyTracker::JOINT_ROOT].transform = Transform3D(root_x, root_y, root_z, root_o).orthonormalized();

		constexpr float shoulder_z_offset = -0.07;
		Transform3D &upper_chest = r_joints[XRBodyTracker::JOINT_UPPER_CHEST].transform;
		Vector3 shoulder_offset = upper_chest.basis.get_column(Vector3::AXIS_Z) * shoulder_z_offset;
		r_joints[XRBodyTracker::JOINT_LEFT_SHOULDER].transform.origin += shoulder_offset;
		r_joints[XRBodyTracker::JOINT_RIGHT_SHOULDER].transform.origin += shoulder_offset;
	}

	return populated_joint_count;
}

OpenXRFbBodyTrackingExtension *OpenXRFbBodyTrackingExtension::singleton = nullptr;

OpenXRFbBodyTrackingExtension *OpenXRFbBodyTrackingExtension::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(OpenXRFbBodyTrackingExtension());
	}
	return singleton;
}

OpenXRFbBodyTrackingExtension::OpenXRFbBodyTrackingExtension() :
		OpenXRExtensionWrapper() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "An OpenXRFbBodyTrackingExtension singleton already exists.");

	request_extensions[XR_FB_BODY_TRACKING_EXTENSION_NAME] = &fb_body_tracking_ext;
	request_extensions[XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME] = &meta_body_tracking_full_body_ext;
	request_extensions[XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME] = &meta_body_tracking_fidelity_ext;
	request_extensions[XR_META_BODY_TRACKING_CALIBRATION_EXTENSION_NAME] = &meta_body_tracking_calibration_ext;

	singleton = this;
}

OpenXRFbBodyTrackingExtension::~OpenXRFbBodyTrackingExtension() {
	cleanup();
	singleton = nullptr;
}

void OpenXRFbBodyTrackingExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_predicted_display_time_raw"), &OpenXRFbBodyTrackingExtension::get_predicted_display_time_raw);
	ClassDB::bind_method(D_METHOD("locate_head_at_time_raw", "xr_time"), &OpenXRFbBodyTrackingExtension::locate_head_at_time_raw);
	ClassDB::bind_method(D_METHOD("locate_body_at_time_raw", "xr_time"), &OpenXRFbBodyTrackingExtension::locate_body_at_time_raw);

	ClassDB::bind_method(D_METHOD("is_full_body_tracking_supported"), &OpenXRFbBodyTrackingExtension::is_full_body_tracking_supported);

	ClassDB::bind_method(D_METHOD("is_body_tracking_fidelity_supported"), &OpenXRFbBodyTrackingExtension::is_body_tracking_fidelity_supported);
	ClassDB::bind_method(D_METHOD("request_body_tracking_fidelity", "fidelity"), &OpenXRFbBodyTrackingExtension::request_body_tracking_fidelity);
	ClassDB::bind_method(D_METHOD("get_body_tracking_fidelity_status"), &OpenXRFbBodyTrackingExtension::get_body_tracking_fidelity_status);

	ClassDB::bind_method(D_METHOD("is_body_tracking_height_override_supported"), &OpenXRFbBodyTrackingExtension::is_body_tracking_height_override_supported);
	ClassDB::bind_method(D_METHOD("suggest_body_tracking_height_override", "body_height"), &OpenXRFbBodyTrackingExtension::suggest_body_tracking_height_override);
	ClassDB::bind_method(D_METHOD("get_body_tracking_calibration_state"), &OpenXRFbBodyTrackingExtension::get_body_tracking_calibration_state);
	ClassDB::bind_method(D_METHOD("reset_body_tracking_calibration"), &OpenXRFbBodyTrackingExtension::reset_body_tracking_calibration);

	BIND_ENUM_CONSTANT(BODY_TRACKING_FIDELITY_UNKNOWN);
	BIND_ENUM_CONSTANT(BODY_TRACKING_FIDELITY_LOW);
	BIND_ENUM_CONSTANT(BODY_TRACKING_FIDELITY_HIGH);

	BIND_ENUM_CONSTANT(BODY_TRACKING_CALIBRATION_STATE_VALID);
	BIND_ENUM_CONSTANT(BODY_TRACKING_CALIBRATION_STATE_CALIBRATING);
	BIND_ENUM_CONSTANT(BODY_TRACKING_CALIBRATION_STATE_INVALID);
}

void OpenXRFbBodyTrackingExtension::cleanup() {
	fb_body_tracking_ext = false;
	meta_body_tracking_full_body_ext = false;
	time_location_functions_initialized = false;

	meta_body_tracking_fidelity_ext = false;
	meta_body_tracking_calibration_ext = false;
}

uint64_t OpenXRFbBodyTrackingExtension::_set_system_properties_and_get_next_pointer(void *p_next_pointer) {
	if (fb_body_tracking_ext) {
		system_body_tracking_properties.next = p_next_pointer;
		p_next_pointer = &system_body_tracking_properties;
	}
	if (meta_body_tracking_full_body_ext) {
		system_body_tracking_full_body_properties.next = p_next_pointer;
		p_next_pointer = &system_body_tracking_full_body_properties;
	}

	if (meta_body_tracking_fidelity_ext) {
		system_body_tracking_fidelity_properties.next = p_next_pointer;
		p_next_pointer = &system_body_tracking_fidelity_properties;
	}
	if (meta_body_tracking_calibration_ext) {
		system_body_tracking_calibration_properties.next = p_next_pointer;
		p_next_pointer = &system_body_tracking_calibration_properties;
	}

	return reinterpret_cast<uint64_t>(p_next_pointer);
}

godot::Dictionary OpenXRFbBodyTrackingExtension::_get_requested_extensions(uint64_t p_xr_version) {
	godot::Dictionary result;
	for (auto ext : request_extensions) {
		godot::String key = ext.first;
		uint64_t value = reinterpret_cast<uint64_t>(ext.second);
		result[key] = (godot::Variant)value;
	}
	return result;
}

void OpenXRFbBodyTrackingExtension::_on_instance_created(uint64_t p_instance) {
	time_location_functions_initialized = initialize_time_location_functions();
	if (!time_location_functions_initialized) {
		ERR_PRINT("Failed to initialize OpenXR temporal location functions");
	}

	if (fb_body_tracking_ext) {
		bool result = initialize_fb_body_tracking_extension((XrInstance)p_instance);
		if (!result) {
			ERR_PRINT("Failed to initialize fb_body_tracking extension");
			fb_body_tracking_ext = false;
		}
	}

	if (meta_body_tracking_fidelity_ext) {
		bool result = initialize_meta_body_tracking_fidelity_extension((XrInstance)p_instance);
		if (!result) {
			ERR_PRINT("Failed to initialize meta_body_tracking_fidelity extension");
			meta_body_tracking_fidelity_ext = false;
		}
	}

	if (meta_body_tracking_calibration_ext) {
		bool result = initialize_meta_body_tracking_calibration_extension((XrInstance)p_instance);
		if (!result) {
			ERR_PRINT("Failed to initialize meta_body_tracking_calibration extension");
			meta_body_tracking_calibration_ext = false;
		}
	}
}

void OpenXRFbBodyTrackingExtension::_on_instance_destroyed() {
	cleanup();
}

void OpenXRFbBodyTrackingExtension::_on_session_created(uint64_t instance) {
	// Meta Body Tracking is the deliberate prerequisite for this extension's
	// timestamped body and companion view-pose observation surface.
	if (!is_enabled()) {
		return;
	}

	if (time_location_functions_initialized) {
		const XrReferenceSpaceCreateInfo create_info = {
			XR_TYPE_REFERENCE_SPACE_CREATE_INFO, // type
			nullptr, // next
			XR_REFERENCE_SPACE_TYPE_VIEW, // referenceSpaceType
			{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }, // poseInReferenceSpace
		};
		const XrResult result = xrCreateReferenceSpace(SESSION, &create_info, &view_space);
		if (XR_FAILED(result)) {
			ERR_PRINT(vformat("Failed to create OpenXR view reference space: %s", get_openxr_api()->get_error_string(result)));
			view_space = XR_NULL_HANDLE;
		}
	}

	// Create the body-tracker handle
	XrBodyJointSetFB body_joint_set = XR_BODY_JOINT_SET_DEFAULT_FB;
	if (meta_body_tracking_full_body_ext && is_full_body_tracking_supported()) {
		body_joint_set = XR_BODY_JOINT_SET_FULL_BODY_META;
	}

	XrBodyTrackerCreateInfoFB createInfo = {
		XR_TYPE_BODY_TRACKER_CREATE_INFO_FB, // type
		nullptr, // next
		body_joint_set, // bodyJointSet
	};
	XrResult result = xrCreateBodyTrackerFB(SESSION, &createInfo, &body_tracker);
	ERR_FAIL_COND_MSG(XR_FAILED(result), vformat("Failed to create body-tracker handle: %s", get_openxr_api()->get_error_string(result)));

	// Construct the XRBodyTracker if necessary
	if (xr_body_tracker.is_null()) {
		xr_body_tracker.instantiate();
		xr_body_tracker->set_tracker_name("/user/body_tracker");

		BitField<XRBodyTracker::BodyFlags> body_flags = XRBodyTracker::BODY_FLAG_UPPER_BODY_SUPPORTED | XRBodyTracker::BODY_FLAG_HANDS_SUPPORTED;
		if (meta_body_tracking_full_body_ext && is_full_body_tracking_supported()) {
			body_flags.set_flag(XRBodyTracker::BODY_FLAG_LOWER_BODY_SUPPORTED);
		}
		xr_body_tracker->set_body_flags(body_flags);
	}
}

void OpenXRFbBodyTrackingExtension::_on_session_destroyed() {
	if (body_tracker) {
		const XrResult result = xrDestroyBodyTrackerFB(body_tracker);
		if (XR_FAILED(result)) {
			ERR_PRINT(vformat("Failed to destroy body-tracker handle: %s", get_openxr_api()->get_error_string(result)));
		}
		body_tracker = XR_NULL_HANDLE;
	}

	// Unregister the body tracker.
	if (xr_body_tracker_registered) {
		XRServer *xr_server = XRServer::get_singleton();
		if (xr_server && xr_body_tracker.is_valid()) {
			xr_server->remove_tracker(xr_body_tracker);
		}
	}
	xr_body_tracker_registered = false;

	if (view_space) {
		const XrResult result = xrDestroySpace(view_space);
		if (XR_FAILED(result)) {
			ERR_PRINT(vformat("Failed to destroy OpenXR view reference space: %s", get_openxr_api()->get_error_string(result)));
		}
		view_space = XR_NULL_HANDLE;
	}
}

void OpenXRFbBodyTrackingExtension::_on_process() {
	// Skip if not enabled, or no body-tracker handle
	if (!is_enabled() || !body_tracker) {
		return;
	}

	// Get the next frame time
	const XrTime display_time = get_openxr_api()->get_predicted_display_time();
	if (display_time == 0) {
		return;
	}

	// Construct the expression info struct.
	XrBodyJointsLocateInfoFB locate_info = {
		XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB, // type
		nullptr, // next
		(XrSpace)get_openxr_api()->get_play_space(), // baseSpace
		display_time // time
	};

	// Construct locations struct next chain.
	void *next_pointer = nullptr;
	if (meta_body_tracking_fidelity_ext && is_body_tracking_fidelity_supported()) {
		body_tracking_fidelity_status.next = next_pointer;
		next_pointer = &body_tracking_fidelity_status;
	}

	if (meta_body_tracking_calibration_ext) {
		body_tracking_calibration_status.next = next_pointer;
		next_pointer = &body_tracking_calibration_status;
	}

	// Construct the locations struct.
	uint32_t fb_joint_count = XR_BODY_JOINT_COUNT_FB;
	bool is_full_body_supported = is_full_body_tracking_supported();
	if (meta_body_tracking_full_body_ext && is_full_body_supported) {
		fb_joint_count = XR_FULL_BODY_JOINT_COUNT_META;
	}

	XrBodyJointLocationFB fb_locations[XR_FULL_BODY_JOINT_COUNT_META];
	XrBodyJointLocationsFB locations = {
		XR_TYPE_BODY_JOINT_LOCATIONS_FB, // type
		next_pointer, // next
		XR_FALSE, // isActive
		0.0f, // confidence
		fb_joint_count, // jointCount
		fb_locations // jointLocations
	};

	// Read the weights
	XrResult result = xrLocateBodyJointsFB(body_tracker, &locate_info, &locations);
	ERR_FAIL_COND_MSG(XR_FAILED(result), vformat("Failed to get body joint locations: %s", get_openxr_api()->get_error_string(result)));

	// Set the tracking active flag
	xr_body_tracker->set_has_tracking_data(locations.isActive);

	const bool full_body_available = meta_body_tracking_full_body_ext && is_full_body_supported;
	LocalVector<CanonicalBodyJoint> canonical_joints;
	convert_body_joint_locations(fb_locations, locations.isActive, full_body_available, canonical_joints);

	for (const JointMapEntry &entry : joint_table) {
		const CanonicalBodyJoint &joint = canonical_joints[entry.xr_joint];
		if (!joint.populated) {
			continue;
		}
		xr_body_tracker->set_joint_flags(entry.xr_joint, joint.flags);
		xr_body_tracker->set_joint_transform(entry.xr_joint, joint.transform);
	}

	if (locations.isActive) {
		const Transform3D &root = canonical_joints[XRBodyTracker::JOINT_ROOT].transform;
		xr_body_tracker->set_pose("default", root, Vector3(), Vector3(), XRPose::XR_TRACKING_CONFIDENCE_HIGH);
	}

	// Register the XRBodyTracker if necessary
	if (!xr_body_tracker_registered) {
		XRServer *xr_server = XRServer::get_singleton();
		if (xr_server) {
			xr_server->add_tracker(xr_body_tracker);
			xr_body_tracker_registered = true;
		}
	}
}

int64_t OpenXRFbBodyTrackingExtension::get_predicted_display_time_raw() {
	if (!is_enabled() || !get_openxr_api().is_valid() || !get_openxr_api()->is_running()) {
		return 0;
	}

	return get_openxr_api()->get_predicted_display_time();
}

Dictionary OpenXRFbBodyTrackingExtension::create_query_result(int64_t p_xr_time) const {
	Dictionary result;
	result["requested_time"] = p_xr_time;
	result["success"] = false;
	result["error_code"] = int64_t(XR_ERROR_HANDLE_INVALID);
	result["error_string"] = "OpenXR temporal location is unavailable";
	return result;
}

void OpenXRFbBodyTrackingExtension::set_query_error(Dictionary &r_result, XrResult p_error, const String &p_context) {
	r_result["success"] = XR_SUCCEEDED(p_error);
	r_result["error_code"] = int64_t(p_error);

	String error_string = get_openxr_api().is_valid() ? get_openxr_api()->get_error_string(p_error) : String("Unknown OpenXR result");
	if (!p_context.is_empty()) {
		error_string = p_context + String(": ") + error_string;
	}
	r_result["error_string"] = error_string;
}

Dictionary OpenXRFbBodyTrackingExtension::locate_head_at_time_raw(int64_t p_xr_time) {
	Dictionary result = create_query_result(p_xr_time);
	result["transform"] = Transform3D();
	result["location_flags"] = int64_t(0);
	result["position_valid"] = false;
	result["orientation_valid"] = false;
	result["position_tracked"] = false;
	result["orientation_tracked"] = false;
	result["linear_velocity"] = Vector3();
	result["angular_velocity"] = Vector3();
	result["velocity_flags"] = int64_t(0);
	result["linear_velocity_valid"] = false;
	result["angular_velocity_valid"] = false;

	if (!is_enabled() || !time_location_functions_initialized || !view_space || !get_openxr_api().is_valid() || !get_openxr_api()->get_play_space()) {
		return result;
	}

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr, // next
		0, // velocityFlags
		{ 0.0f, 0.0f, 0.0f }, // linearVelocity
		{ 0.0f, 0.0f, 0.0f }, // angularVelocity
	};
	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity, // next
		0, // locationFlags
		{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }, // pose
	};

	const XrResult xr_result = xrLocateSpace(view_space, (XrSpace)get_openxr_api()->get_play_space(), (XrTime)p_xr_time, &location);
	set_query_error(result, xr_result, "xrLocateSpace");
	if (XR_FAILED(xr_result)) {
		return result;
	}

	const bool orientation_valid = location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
	const bool position_valid = location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT;
	Transform3D transform;
	if (orientation_valid) {
		transform.basis = Basis(Quaternion(location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z, location.pose.orientation.w));
	}
	if (position_valid) {
		transform.origin = OpenXRUtilities::XrVector3f_to_godot_vector3(location.pose.position);
	}

	result["transform"] = transform;
	result["location_flags"] = int64_t(location.locationFlags);
	result["position_valid"] = position_valid;
	result["orientation_valid"] = orientation_valid;
	result["position_tracked"] = bool(location.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
	result["orientation_tracked"] = bool(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT);
	result["velocity_flags"] = int64_t(velocity.velocityFlags);
	result["linear_velocity_valid"] = bool(velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT);
	result["angular_velocity_valid"] = bool(velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT);
	if (velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
		result["linear_velocity"] = OpenXRUtilities::XrVector3f_to_godot_vector3(velocity.linearVelocity);
	}
	if (velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) {
		result["angular_velocity"] = OpenXRUtilities::XrVector3f_to_godot_vector3(velocity.angularVelocity);
	}

	return result;
}

Dictionary OpenXRFbBodyTrackingExtension::locate_body_at_time_raw(int64_t p_xr_time) {
	Dictionary result = create_query_result(p_xr_time);
	result["effective_time"] = int64_t(0);
	result["is_active"] = false;
	result["confidence"] = 0.0f;
	result["skeleton_changed_count"] = int64_t(0);
	result["body_tracking_supported"] = is_enabled();
	result["full_body_supported"] = meta_body_tracking_full_body_ext && is_full_body_tracking_supported();
	result["joint_count"] = 0;
	result["joints"] = Array();

	if (!is_enabled() || !body_tracker || !get_openxr_api().is_valid() || !get_openxr_api()->get_play_space()) {
		return result;
	}

	const XrBodyJointsLocateInfoFB locate_info = {
		XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB, // type
		nullptr, // next
		(XrSpace)get_openxr_api()->get_play_space(), // baseSpace
		(XrTime)p_xr_time, // time
	};

	const bool full_body = meta_body_tracking_full_body_ext && system_body_tracking_full_body_properties.supportsFullBodyTracking;
	const uint32_t joint_count = full_body ? XR_FULL_BODY_JOINT_COUNT_META : XR_BODY_JOINT_COUNT_FB;
	XrBodyJointLocationFB joint_locations[XR_FULL_BODY_JOINT_COUNT_META] = {};
	XrBodyJointLocationsFB locations = {
		XR_TYPE_BODY_JOINT_LOCATIONS_FB, // type
		nullptr, // next
		XR_FALSE, // isActive
		0.0f, // confidence
		joint_count, // jointCount
		joint_locations, // jointLocations
		0, // skeletonChangedCount
		0, // time
	};

	const XrResult xr_result = xrLocateBodyJointsFB(body_tracker, &locate_info, &locations);
	set_query_error(result, xr_result, "xrLocateBodyJointsFB");
	if (XR_FAILED(xr_result)) {
		return result;
	}

	LocalVector<CanonicalBodyJoint> canonical_joints;
	const int canonical_joint_count = convert_body_joint_locations(joint_locations, locations.isActive, full_body, canonical_joints);
	Array joints;
	joints.resize(XRBodyTracker::JOINT_MAX);
	for (int joint_index = 0; joint_index < XRBodyTracker::JOINT_MAX; joint_index++) {
		const CanonicalBodyJoint &canonical_joint = canonical_joints[joint_index];
		if (!canonical_joint.populated) {
			continue;
		}

		Dictionary joint;
		joint["transform"] = canonical_joint.transform;
		joint["position_valid"] = canonical_joint.flags.has_flag(XRBodyTracker::JOINT_FLAG_POSITION_VALID);
		joint["orientation_valid"] = canonical_joint.flags.has_flag(XRBodyTracker::JOINT_FLAG_ORIENTATION_VALID);
		joint["position_tracked"] = canonical_joint.flags.has_flag(XRBodyTracker::JOINT_FLAG_POSITION_TRACKED);
		joint["orientation_tracked"] = canonical_joint.flags.has_flag(XRBodyTracker::JOINT_FLAG_ORIENTATION_TRACKED);
		joints[joint_index] = joint;
	}

	result["effective_time"] = int64_t(locations.time);
	result["is_active"] = bool(locations.isActive);
	result["confidence"] = locations.confidence;
	result["skeleton_changed_count"] = int64_t(locations.skeletonChangedCount);
	result["joint_count"] = canonical_joint_count;
	result["joints"] = joints;
	return result;
}

bool OpenXRFbBodyTrackingExtension::is_enabled() const {
	return fb_body_tracking_ext && system_body_tracking_properties.supportsBodyTracking;
}

bool OpenXRFbBodyTrackingExtension::initialize_time_location_functions() {
	GDEXTENSION_INIT_XR_FUNC_V(xrCreateReferenceSpace);
	GDEXTENSION_INIT_XR_FUNC_V(xrDestroySpace);
	GDEXTENSION_INIT_XR_FUNC_V(xrLocateSpace);

	return true;
}

bool OpenXRFbBodyTrackingExtension::initialize_fb_body_tracking_extension(const XrInstance p_instance) {
	GDEXTENSION_INIT_XR_FUNC_V(xrCreateBodyTrackerFB);
	GDEXTENSION_INIT_XR_FUNC_V(xrDestroyBodyTrackerFB);
	GDEXTENSION_INIT_XR_FUNC_V(xrLocateBodyJointsFB);

	return true;
}

// META_body_tracking_full_body extension.

bool OpenXRFbBodyTrackingExtension::is_full_body_tracking_supported() {
	return system_body_tracking_full_body_properties.supportsFullBodyTracking;
}

// META_body_tracking_fidelity extension.

bool OpenXRFbBodyTrackingExtension::is_body_tracking_fidelity_supported() {
	return system_body_tracking_fidelity_properties.supportsBodyTrackingFidelity;
}

void OpenXRFbBodyTrackingExtension::request_body_tracking_fidelity(BodyTrackingFidelity p_fidelity) {
	ERR_FAIL_COND_MSG(!fb_body_tracking_ext || !meta_body_tracking_fidelity_ext, "XR_META_body_tracking_fidelity extension is not enabled");
	ERR_FAIL_COND_MSG(p_fidelity == BODY_TRACKING_FIDELITY_UNKNOWN, "Cannot request body tracking fidelity update: invalid fidelity type");
	ERR_FAIL_COND_MSG(body_tracker == XR_NULL_HANDLE, "Cannot request body tracking fidelity update: body tracker handle is null");

	const XrBodyTrackingFidelityMETA fidelity = XrBodyTrackingFidelityMETA(p_fidelity);
	XrResult result = xrRequestBodyTrackingFidelityMETA(body_tracker, fidelity);
	ERR_FAIL_COND_MSG(XR_FAILED(result), vformat("Failed to request body tracking fidelity update: %s", get_openxr_api()->get_error_string(result)));
}

OpenXRFbBodyTrackingExtension::BodyTrackingFidelity OpenXRFbBodyTrackingExtension::get_body_tracking_fidelity_status() {
	ERR_FAIL_COND_V_MSG(!fb_body_tracking_ext || !meta_body_tracking_fidelity_ext, BODY_TRACKING_FIDELITY_UNKNOWN, "XR_META_body_tracking_fidelity extension is not enabled");

	switch (body_tracking_fidelity_status.fidelity) {
		case XR_BODY_TRACKING_FIDELITY_LOW_META:
			return BODY_TRACKING_FIDELITY_LOW;
		case XR_BODY_TRACKING_FIDELITY_HIGH_META:
			return BODY_TRACKING_FIDELITY_HIGH;
		default:
			return BODY_TRACKING_FIDELITY_UNKNOWN;
	}
}

bool OpenXRFbBodyTrackingExtension::initialize_meta_body_tracking_fidelity_extension(XrInstance p_instance) {
	GDEXTENSION_INIT_XR_FUNC_V(xrRequestBodyTrackingFidelityMETA);

	return true;
}

// META_body_tracking_calibration extension.

bool OpenXRFbBodyTrackingExtension::is_body_tracking_height_override_supported() {
	return system_body_tracking_calibration_properties.supportsHeightOverride;
}

OpenXRFbBodyTrackingExtension::BodyTrackingCalibrationState OpenXRFbBodyTrackingExtension::get_body_tracking_calibration_state() {
	ERR_FAIL_COND_V_MSG(!fb_body_tracking_ext || !meta_body_tracking_calibration_ext, BODY_TRACKING_CALIBRATION_STATE_INVALID, "XR_META_body_tracking_calibration extension is not enabled");

	switch (body_tracking_calibration_status.status) {
		case XR_BODY_TRACKING_CALIBRATION_STATE_VALID_META:
			return BODY_TRACKING_CALIBRATION_STATE_VALID;
		case XR_BODY_TRACKING_CALIBRATION_STATE_CALIBRATING_META:
			return BODY_TRACKING_CALIBRATION_STATE_CALIBRATING;
		case XR_BODY_TRACKING_CALIBRATION_STATE_INVALID_META:
			return BODY_TRACKING_CALIBRATION_STATE_INVALID;
		default:
			return BODY_TRACKING_CALIBRATION_STATE_INVALID;
	}
}

void OpenXRFbBodyTrackingExtension::suggest_body_tracking_height_override(float p_body_height) {
	ERR_FAIL_COND_MSG(!fb_body_tracking_ext || !meta_body_tracking_calibration_ext, "XR_META_body_tracking_calibration extension is not enabled");
	ERR_FAIL_COND_MSG(p_body_height < 0.5f || p_body_height > 3.0f, "Cannot request body tracking height override: height must be within range of 0.5 and 3.0 meters");
	ERR_FAIL_COND_MSG(body_tracker == XR_NULL_HANDLE, "Cannot request body tracking height override: body tracker handle is null");

	const XrBodyTrackingCalibrationInfoMETA body_tracking_calibration_info = {
		XR_TYPE_BODY_TRACKING_CALIBRATION_INFO_META, // type
		nullptr, // next
		p_body_height, // bodyHeight
	};

	XrResult result = xrSuggestBodyTrackingCalibrationOverrideMETA(body_tracker, &body_tracking_calibration_info);
	ERR_FAIL_COND_MSG(XR_FAILED(result), vformat("Failed to suggest body tracking calibration override: %s", get_openxr_api()->get_error_string(result)));
}

void OpenXRFbBodyTrackingExtension::reset_body_tracking_calibration() {
	ERR_FAIL_COND_MSG(!fb_body_tracking_ext || !meta_body_tracking_calibration_ext, "XR_META_body_tracking_calibration extension is not enabled");
	ERR_FAIL_COND_MSG(body_tracker == XR_NULL_HANDLE, "Cannot reset body tracking calibration: body tracker handle is null");

	XrResult result = xrResetBodyTrackingCalibrationMETA(body_tracker);
	ERR_FAIL_COND_MSG(XR_FAILED(result), vformat("Failed to reset body tracking calibration: %s", get_openxr_api()->get_error_string(result)));
}

bool OpenXRFbBodyTrackingExtension::initialize_meta_body_tracking_calibration_extension(XrInstance p_instance) {
	GDEXTENSION_INIT_XR_FUNC_V(xrSuggestBodyTrackingCalibrationOverrideMETA);
	GDEXTENSION_INIT_XR_FUNC_V(xrResetBodyTrackingCalibrationMETA);

	return true;
}
