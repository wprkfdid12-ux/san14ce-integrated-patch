
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001406be850 <.text+0x6bd850>:
   1406be850:	48 8b c1             	mov    rax,rcx
   1406be853:	48 8b 89 d8 01 00 00 	mov    rcx,QWORD PTR [rcx+0x1d8]
   1406be85a:	83 b9 78 01 00 00 00 	cmp    DWORD PTR [rcx+0x178],0x0
   1406be861:	75 14                	jne    0x1406be877
   1406be863:	83 b9 7c 01 00 00 00 	cmp    DWORD PTR [rcx+0x17c],0x0
   1406be86a:	75 0b                	jne    0x1406be877
   1406be86c:	89 90 78 01 00 00    	mov    DWORD PTR [rax+0x178],edx
   1406be872:	e9 19 78 19 00       	jmp    0x140856090
   1406be877:	c3                   	ret
   1406be878:	cc                   	int3
   1406be879:	cc                   	int3
   1406be87a:	cc                   	int3
   1406be87b:	cc                   	int3
   1406be87c:	cc                   	int3
   1406be87d:	cc                   	int3
   1406be87e:	cc                   	int3
   1406be87f:	cc                   	int3
   1406be880:	48 8b c1             	mov    rax,rcx
   1406be883:	48 8b 89 78 02 00 00 	mov    rcx,QWORD PTR [rcx+0x278]
   1406be88a:	83 b9 78 01 00 00 00 	cmp    DWORD PTR [rcx+0x178],0x0
   1406be891:	75 14                	jne    0x1406be8a7
   1406be893:	83 b9 7c 01 00 00 00 	cmp    DWORD PTR [rcx+0x17c],0x0
   1406be89a:	75 0b                	jne    0x1406be8a7
   1406be89c:	89 90 78 01 00 00    	mov    DWORD PTR [rax+0x178],edx
   1406be8a2:	e9 e9 77 19 00       	jmp    0x140856090
   1406be8a7:	c3                   	ret
   1406be8a8:	cc                   	int3
   1406be8a9:	cc                   	int3
   1406be8aa:	cc                   	int3
   1406be8ab:	cc                   	int3
   1406be8ac:	cc                   	int3
   1406be8ad:	cc                   	int3
   1406be8ae:	cc                   	int3
   1406be8af:	cc                   	int3
   1406be8b0:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   1406be8b5:	57                   	push   rdi
   1406be8b6:	48 83 ec 20          	sub    rsp,0x20
   1406be8ba:	48 8b d9             	mov    rbx,rcx
   1406be8bd:	8b fa                	mov    edi,edx
   1406be8bf:	48 8b 89 b8 01 00 00 	mov    rcx,QWORD PTR [rcx+0x1b8]
   1406be8c6:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406be8c9:	ff 90 18 01 00 00    	call   QWORD PTR [rax+0x118]
   1406be8cf:	85                   	.byte 0x85
