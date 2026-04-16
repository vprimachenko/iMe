; Test 1: move in a square above the bed without extrusion
; This is useful for checking XY motion and path shape safely in the air.

;@m33 status: Hello from the square-in-the-air test
; Use millimeters
G21
; Use absolute XYZ positioning
G90
; Use absolute extrusion mode even though this file does not extrude
M82

; Home / auto-center X and Y
G28
; Reset the extruder position counter
G92 E0

; Lift to a safe height above the bed
G0 Z10 F30
; Move to the square start point in the air
G0 X20 Y20 F600

; Trace a 60 mm square without extruding
G1 X80 Y20 F600
G1 X80 Y80 F600
G1 X20 Y80 F600
G1 X20 Y20 F600

;@m33 status: Goodbye from the square-in-the-air test
; Turn motors off at the end
M18
