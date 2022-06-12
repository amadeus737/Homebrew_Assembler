.arch "..\\Homebrew_Assembler\\Architecture_Config\\homebrew.arch"
.org 0
.export 0 256

@X $42

[start]:
	nop
	
[code]:
    ; mov $42 into b
	mov $33, X

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
