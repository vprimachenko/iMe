; Test 2: heat the nozzle and extrude filament in place
; This is useful for checking heating and extrusion without XY printing.

;@m33 status: Hello from the heat-and-extrude test
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
;@m33 status: heating the nozzle
M109 S200

; Reset extrusion position before the test
G92 E0
; Lift slightly above the bed to avoid scraping
G0 Z5 F30
; Move to a visible position for the extrusion test
G0 X40 Y40 F600

; Extrude a small amount of filament slowly in place
G1 E60.0 F120
; Extrude a little more to confirm sustained flow
G1 E100.0 F120

;@m33 status: Goodbye from the heat-and-extrude test
; Cool the nozzle and release motors
M104 S0
M18
