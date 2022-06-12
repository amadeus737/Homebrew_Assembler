.arch "homebrew"
.org 0
.export 0 255

@X $42

[start]:
	nop
	
[code]:
    ; mov $42 into b
	lda X
	nop
	@Y $21

	ldb Y
	lda $33

	.byte %1010, $53, 15
	.ascii "Hello/world!"

	mov b, a
	tba
	mov Y, a

	sta $0a
