extends StartXR

const TEMPORAL_QUERY_INTERVAL := 0.25
const RETAINED_TIMESTAMP_COUNT := 8

@onready var viewport_2d_in_3d = %Viewport2Din3D
@onready var eye_gaze: XRController3D = %EyeGaze
@onready var android_xr_left_eye: XRController3D = %AndroidXRLeftEye
@onready var android_xr_right_eye: XRController3D = %AndroidXRRightEye

var eye_mode: String = "XR_EXT_eye_gaze_interaction"
var retained_timestamps: Array[int] = []
var temporal_query_elapsed := TEMPORAL_QUERY_INTERVAL


func _ready() -> void:
	super._ready()

	viewport_2d_in_3d.get_scene_root().eye_mode_changed.connect(_on_eye_mode_changed)


func _process(p_delta: float) -> void:
	if eye_mode == "XR_EXT_eye_gaze_interaction":
		var text: String = "Has tracking data!" if eye_gaze.get_has_tracking_data() else "No tracking data."
		viewport_2d_in_3d.get_scene_root().set_info_label_text(text)

	elif eye_mode == "XR_ANDROID_eye_tracking":
		var lines: PackedStringArray
		lines.push_back("Left eye: " + _get_android_xr_eye_info(android_xr_left_eye))
		lines.push_back("Right eye: " + _get_android_xr_eye_info(android_xr_right_eye))
		var text: String = "\n\n".join(lines)
		viewport_2d_in_3d.get_scene_root().set_info_label_text(text)

	_update_temporal_location_probe(p_delta)


func _update_temporal_location_probe(p_delta: float) -> void:
	temporal_query_elapsed += p_delta
	if temporal_query_elapsed < TEMPORAL_QUERY_INTERVAL:
		return
	temporal_query_elapsed = 0.0

	var current_time: int = OpenXRFbBodyTrackingExtension.get_predicted_display_time_raw()
	if current_time == 0:
		var unavailable_text := "Temporal raw XrTime probe: OpenXR session is not running"
		viewport_2d_in_3d.get_scene_root().set_temporal_location_text(unavailable_text)
		return

	if retained_timestamps.is_empty() or retained_timestamps.back() != current_time:
		retained_timestamps.push_back(current_time)
		if retained_timestamps.size() > RETAINED_TIMESTAMP_COUNT:
			retained_timestamps.pop_front()

	var earlier_time: int = retained_timestamps.front()
	var current_head: Dictionary = OpenXRFbBodyTrackingExtension.locate_head_at_time_raw(current_time)
	var current_body: Dictionary = OpenXRFbBodyTrackingExtension.locate_body_at_time_raw(current_time)
	var earlier_head: Dictionary = OpenXRFbBodyTrackingExtension.locate_head_at_time_raw(earlier_time)
	var earlier_body: Dictionary = OpenXRFbBodyTrackingExtension.locate_body_at_time_raw(earlier_time)

	var lines: PackedStringArray = ["Temporal raw XrTime smoke probe (no clock conversion)"]
	lines.push_back(_format_temporal_time("NOW", current_time, current_body))
	lines.push_back(_format_head_result("  head", current_head))
	lines.push_back(_format_body_result("  body", current_body))
	lines.push_back(_format_temporal_time("PAST", earlier_time, earlier_body))
	lines.push_back(_format_head_result("  head", earlier_head))
	lines.push_back(_format_body_result("  body", earlier_body))
	lines.push_back("Δraw NOW-PAST=%d" % (current_time - earlier_time))
	lines.push_back(_format_pose_delta("  head delta", current_head, earlier_head))
	lines.push_back(_format_pose_delta("  root delta", _get_body_root(current_body), _get_body_root(earlier_body)))
	lines.push_back(_format_live_joint_comparison("  NOW root/live", current_body, XRBodyTracker.JOINT_ROOT))
	lines.push_back(_format_live_joint_comparison("  NOW left shoulder/live", current_body, XRBodyTracker.JOINT_LEFT_SHOULDER))
	viewport_2d_in_3d.get_scene_root().set_temporal_location_text("\n".join(lines))


func _format_temporal_time(p_label: String, p_requested_time: int, p_body: Dictionary) -> String:
	return "%s req=%d body_eff=%d" % [p_label, p_requested_time, p_body.get("effective_time", 0)]


func _format_head_result(p_label: String, p_head: Dictionary) -> String:
	var state := _get_location_state(p_head)
	if not p_head.get("success", false):
		return "%s %s" % [p_label, state]

	return (
		"%s %s P:%s O:%s LV:%s AV:%s %s"
		% [
			p_label,
			state,
			_format_valid_tracked(p_head, "position"),
			_format_valid_tracked(p_head, "orientation"),
			_flag(p_head.get("linear_velocity_valid", false)),
			_flag(p_head.get("angular_velocity_valid", false)),
			_format_pose(p_head.get("transform", Transform3D())),
		]
	)


func _format_body_result(p_label: String, p_body: Dictionary) -> String:
	if not p_body.get("success", false):
		return "%s FAIL(%d)" % [p_label, p_body.get("error_code", 0)]
	if not p_body.get("is_active", false):
		return "%s INACTIVE conf=%.2f" % [p_label, p_body.get("confidence", 0.0)]

	var root := _get_body_root(p_body)
	if root.is_empty():
		return "%s ACTIVE/INVALID conf=%.2f root=missing" % [p_label, p_body.get("confidence", 0.0)]

	return (
		"%s ACTIVE/%s conf=%.2f root P:%s O:%s %s"
		% [
			p_label,
			_get_location_state(root),
			p_body.get("confidence", 0.0),
			_format_valid_tracked(root, "position"),
			_format_valid_tracked(root, "orientation"),
			_format_pose(root.get("transform", Transform3D())),
		]
	)


func _get_body_root(p_body: Dictionary) -> Dictionary:
	if not p_body.get("success", false) or not p_body.get("is_active", false):
		return {}

	var joints: Array = p_body.get("joints", [])
	if joints.size() <= XRBodyTracker.JOINT_ROOT:
		return {}
	return joints[XRBodyTracker.JOINT_ROOT]


func _format_live_joint_comparison(p_label: String, p_body: Dictionary, p_joint: int) -> String:
	var timestamped_joint := _get_body_joint(p_body, p_joint)
	var live_body_tracker := XRServer.get_tracker("/user/body_tracker") as XRBodyTracker
	if live_body_tracker == null or timestamped_joint.is_empty():
		return "%s unavailable" % p_label

	var live_flags: int = live_body_tracker.get_joint_flags(p_joint)
	var live_joint := {
		"transform": live_body_tracker.get_joint_transform(p_joint),
		"position_valid": bool(live_flags & XRBodyTracker.JOINT_FLAG_POSITION_VALID),
		"orientation_valid": bool(live_flags & XRBodyTracker.JOINT_FLAG_ORIENTATION_VALID),
		"position_tracked": bool(live_flags & XRBodyTracker.JOINT_FLAG_POSITION_TRACKED),
		"orientation_tracked": bool(live_flags & XRBodyTracker.JOINT_FLAG_ORIENTATION_TRACKED),
	}
	var agreement := "agree" if _location_state_agrees(timestamped_joint, live_joint) else "FLAGS-DIFFER"
	return "%s %s %s" % [p_label, agreement, _format_pose_delta("delta", timestamped_joint, live_joint)]


func _get_body_joint(p_body: Dictionary, p_joint: int) -> Dictionary:
	var joints: Array = p_body.get("joints", [])
	if joints.size() <= p_joint:
		return {}
	return joints[p_joint]


func _location_state_agrees(p_first: Dictionary, p_second: Dictionary) -> bool:
	return p_first.get("position_valid", false) == p_second.get("position_valid", false) and p_first.get("orientation_valid", false) == p_second.get("orientation_valid", false) and p_first.get("position_tracked", false) == p_second.get("position_tracked", false) and p_first.get("orientation_tracked", false) == p_second.get("orientation_tracked", false)


func _format_pose_delta(p_label: String, p_now: Dictionary, p_past: Dictionary) -> String:
	if not _has_valid_pose(p_now) or not _has_valid_pose(p_past):
		return "%s unavailable (requires two valid locations)" % p_label

	var now_transform: Transform3D = p_now.get("transform", Transform3D())
	var past_transform: Transform3D = p_past.get("transform", Transform3D())
	var position_delta := now_transform.origin.distance_to(past_transform.origin)
	var rotation_delta := rad_to_deg(now_transform.basis.get_rotation_quaternion().angle_to(past_transform.basis.get_rotation_quaternion()))
	return "%s dist=%.3fm rot=%.1fdeg" % [p_label, position_delta, rotation_delta]


func _has_valid_pose(p_location: Dictionary) -> bool:
	return p_location.get("success", true) and p_location.get("position_valid", false) and p_location.get("orientation_valid", false)


func _get_location_state(p_location: Dictionary) -> String:
	if not p_location.get("success", true):
		return "FAIL(%d)" % p_location.get("error_code", 0)
	if not p_location.get("position_valid", false) or not p_location.get("orientation_valid", false):
		return "INVALID"
	if p_location.get("position_tracked", false) and p_location.get("orientation_tracked", false):
		return "TRACKED"
	return "VALID/UNTRACKED"


func _format_valid_tracked(p_location: Dictionary, p_component: String) -> String:
	return (
		"%s/%s"
		% [
			_flag(p_location.get(p_component + "_valid", false)),
			_flag(p_location.get(p_component + "_tracked", false)),
		]
	)


func _flag(p_value: bool) -> String:
	return "Y" if p_value else "N"


func _format_pose(p_transform: Transform3D) -> String:
	var position := p_transform.origin
	var rotation := p_transform.basis.get_euler()
	return (
		"p(%.2f,%.2f,%.2f) rdeg(%.0f,%.0f,%.0f)"
		% [
			position.x,
			position.y,
			position.z,
			rad_to_deg(rotation.x),
			rad_to_deg(rotation.y),
			rad_to_deg(rotation.z),
		]
	)


func _get_android_xr_eye_info(p_eye: XRController3D) -> String:
	var text: String = "Has tracking data" if p_eye.get_has_tracking_data() else "No tracking data"
	text += "\nBlinking: "
	text += "yes" if p_eye.is_button_pressed("blink") else "no"
	return text


func _on_eye_mode_changed(p_mode: String) -> void:
	eye_mode = p_mode

	if eye_mode == "XR_EXT_eye_gaze_interaction":
		eye_gaze.show_when_tracked = true
		android_xr_left_eye.visible = false
		android_xr_left_eye.show_when_tracked = false
		android_xr_right_eye.visible = false
		android_xr_right_eye.show_when_tracked = false

	elif eye_mode == "XR_ANDROID_eye_tracking":
		eye_gaze.show_when_tracked = false
		eye_gaze.visible = false
		android_xr_left_eye.show_when_tracked = true
		android_xr_right_eye.show_when_tracked = true
