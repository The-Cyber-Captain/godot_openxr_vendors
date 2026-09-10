Monotonic time conversion
=========================

The ``OpenXRMonotonicTimeConversionExtension`` singleton exposes a native,
vendor-neutral Android clock bridge:

.. code-block:: gdscript

    var uptime_nanos := OpenXRMonotonicTimeConversionExtension.get_android_uptime_nanos()
    if OpenXRMonotonicTimeConversionExtension.is_monotonic_time_conversion_supported():
        var xr_time := OpenXRMonotonicTimeConversionExtension.monotonic_nanos_to_xr_time(uptime_nanos)

On Android the source is ``CLOCK_MONOTONIC``, exactly matching the
suspend-excluding semantics of ``SystemClock.uptimeNanos()``. OpenXR conversion
uses ``XR_KHR_convert_timespec_time`` on the active instance. The API returns
``-1`` when unsupported, before initialization, for non-positive input, on
overflow, or when the runtime conversion fails. Other platforms keep the API
registered but report it as unsupported and return ``-1`` for timestamps.

The extension does not use predicted display times, wall-clock time,
engine-relative time, calibration, or background polling. Both timelines pause
across deep sleep and resume with the same monotonic relationship.
