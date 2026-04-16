; Test 3: heat, prime, and print a square on the bed surface
; This is a simple first-layer style print check.

;@m33 status: Hello from the bed-print square test
; Use millimeters
G21
; Use absolute XYZ positioning
G90
; Use absolute extrusion mode
M82

; Turn part cooling fan off
M107
; Start heating the nozzle
M104 S200
; Wait for the nozzle to reach target temperature
M109 S200

; Home / auto-center X and Y
; G28
; Reset extrusion position counter
G92 E0

; Lift before moving to the prime line start
G0 Z5 F30
; Move to the prime line start
G0 X10 Y20 F600
; Move down to first-layer height
G0 Z0.15 F30

; Prime the nozzle with a straight line
G1 X10 Y70 E8.0 F700
; Reset extrusion after the prime line
G92 E0

; Move to the square start point
G0 X20 Y20 Z0.15 F600

; Print a 60 mm square on the bed
G1 X80 Y20 E4.0 F700
G1 X80 Y80 E7.0 F700
G1 X20 Y80 E10.0 F700
G1 X20 Y20 E13.0 F700

;@m33 status: Goodbye from the bed-print square test
; Cool the nozzle, stop the fan, and release motors
M104 S0
M107
M18
