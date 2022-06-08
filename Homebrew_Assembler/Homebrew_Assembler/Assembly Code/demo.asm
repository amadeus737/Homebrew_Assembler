.org 0
.export 0 256

@X $42

[start]:
	nop
	
[code]:
    ; mov $42 into b
	mov a, X
	lda X
	nop
	@Y $21
	mov b, a
	tba
	mov Y, c
	stc Y
