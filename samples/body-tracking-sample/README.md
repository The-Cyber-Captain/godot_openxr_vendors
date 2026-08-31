# Meta Body Tracking Sample

> Note: this project requires Godot 4.3 or later

This is a sample project demonstrating tracking features on Meta headsets. This includes body / hand tracking, which is supported on Quest 2, Quest 3, and Quest Pro;
as well as face tracking, which is only supported on Quest Pro. For more Meta-specific hand tracking features, check out the [Meta Hand Tracking Sample](https://github.com/GodotVR/godot_openxr_vendors/tree/master/samples/meta-hand-tracking-sample).

# Screenshots

![Screenshot](screenshots/meta_body_tracking_screenshot_01.png)

The in-headset UI also includes a developer-facing temporal-location smoke probe. It obtains the runtime's raw predicted-display `XrTime`, queries the view and Meta body pose at that exact value, and repeats the queries for an earlier raw timestamp retained by the sample. Timestamped body joints use the same `XRBodyTracker.Joint` layout and Godot transforms as `/user/body_tracker`; the readout compares selected current-time joints with that live tracker, including pose deltas and validity/tracked-flag agreement. It also exposes call failures separately from inactive or invalid locations and valid tracked locations.

The probe treats `XrTime` as an opaque OpenXR clock-domain value. It does not convert timestamps, match them to another sensor clock, or claim to validate historical-pose accuracy.
