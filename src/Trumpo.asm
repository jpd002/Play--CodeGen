_TEXT SEGMENT

;	CX86Assembler::rBX,
;	CX86Assembler::rSI,
;	CX86Assembler::rDI,
;	CX86Assembler::r12,
;	CX86Assembler::r13,
;	CX86Assembler::r14,
;	CX86Assembler::r15,

trumpoline PROC
	push rbx
	push rsi
	push rdi
	push r12
	push r13
	push r14
	push r15
	sub rsp, 16

	call rdx

	add rsp, 16
	pop r15
	pop r14
	pop r13
	pop r12
	pop rdi
	pop rsi
	pop rbx
	ret

trumpoline ENDP

_TEXT ENDS

END
