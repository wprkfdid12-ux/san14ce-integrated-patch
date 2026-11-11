
/home/claude/r40/mt_test/source/build/SAN14_MeritTraits.dll:     file format pei-x86-64


Disassembly of section .text:

0000000180018920 <.text+0x17920>:
   180018920:	56                   	push   rsi
   180018921:	57                   	push   rdi
   180018922:	b8 28 20 00 00       	mov    eax,0x2028
   180018927:	e8 c0 4f 01 00       	call   0x18002d8ec
   18001892c:	48 29 c4             	sub    rsp,rax
   18001892f:	4c 89 ce             	mov    rsi,r9
   180018932:	83 f9 01             	cmp    ecx,0x1
   180018935:	75 2c                	jne    0x180018963
   180018937:	48 8d 7c 24 24       	lea    rdi,[rsp+0x24]
   18001893c:	48 89 d1             	mov    rcx,rdx
   18001893f:	48 89 fa             	mov    rdx,rdi
   180018942:	e8 69 bd 00 00       	call   0x1800246b0
   180018947:	44 8b 84 24 24 20 00 	mov    r8d,DWORD PTR [rsp+0x2024]
   18001894e:	00 
   18001894f:	48 89 f1             	mov    rcx,rsi
   180018952:	48 89 fa             	mov    rdx,rdi
   180018955:	e8 a6 86 fe ff       	call   0x180001000
   18001895a:	8b 84 24 24 20 00 00 	mov    eax,DWORD PTR [rsp+0x2024]
   180018961:	eb 1e                	jmp    0x180018981
   180018963:	31 c0                	xor    eax,eax
   180018965:	41 81 f8 00 20 00 00 	cmp    r8d,0x2000
   18001896c:	77 13                	ja     0x180018981
   18001896e:	44 89 c0             	mov    eax,r8d
   180018971:	48 89 f1             	mov    rcx,rsi
   180018974:	44 89 c6             	mov    esi,r8d
   180018977:	49 89 c0             	mov    r8,rax
   18001897a:	e8 81 86 fe ff       	call   0x180001000
   18001897f:	89 f0                	mov    eax,esi
   180018981:	48 81 c4 28 20 00 00 	add    rsp,0x2028
   180018988:	5f                   	pop    rdi
   180018989:	5e                   	pop    rsi
   18001898a:	c3                   	ret
