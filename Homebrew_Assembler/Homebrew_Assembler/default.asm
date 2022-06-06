.org $fa
.export 0 256

@X $42

[start]:
	nop
	
[code]:
    ; mov $42 into b
	mov a, X
	mov b, a