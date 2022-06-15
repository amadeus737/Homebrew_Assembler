.arch homebrew
.include test1.asm

[start]:
	nop
	
[code]:
    ; mov $42 into b
	lda X
	nop
	@Y $21

	.insert test2.asm

	mov b, a
	tba
	mov Y, a

	sta $0a
