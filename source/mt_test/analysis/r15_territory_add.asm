
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140256f70 <.text+0x255f70>:
   140256f70:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   140256f75:	57                   	push   rdi
   140256f76:	48 83 ec 20          	sub    rsp,0x20
   140256f7a:	8b fa                	mov    edi,edx
   140256f7c:	48 8b d9             	mov    rbx,rcx
   140256f7f:	ba 02 00 00 00       	mov    edx,0x2
   140256f84:	e8 07 22 05 00       	call   0x1402a9190
   140256f89:	ff c0                	inc    eax
   140256f8b:	83 f8 01             	cmp    eax,0x1
   140256f8e:	75 23                	jne    0x140256fb3
   140256f90:	48 8b 83 d0 01 00 00 	mov    rax,QWORD PTR [rbx+0x1d0]
   140256f97:	89 38                	mov    DWORD PTR [rax],edi
   140256f99:	b8 01 00 00 00       	mov    eax,0x1
   140256f9e:	c7 83 e8 01 00 00 01 	mov    DWORD PTR [rbx+0x1e8],0x1
   140256fa5:	00 00 00 
   140256fa8:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   140256fad:	48 83 c4 20          	add    rsp,0x20
   140256fb1:	5f                   	pop    rdi
   140256fb2:	c3                   	ret
   140256fb3:	ba 02 00 00 00       	mov    edx,0x2
   140256fb8:	48 8b cb             	mov    rcx,rbx
   140256fbb:	e8 d0 21 05 00       	call   0x1402a9190
   140256fc0:	48 63 93 e8 01 00 00 	movsxd rdx,DWORD PTR [rbx+0x1e8]
   140256fc7:	ff c0                	inc    eax
   140256fc9:	3b d0                	cmp    edx,eax
   140256fcb:	7c 0d                	jl     0x140256fda
   140256fcd:	33 c0                	xor    eax,eax
   140256fcf:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   140256fd4:	48 83 c4 20          	add    rsp,0x20
   140256fd8:	5f                   	pop    rdi
   140256fd9:	c3                   	ret
   140256fda:	33 c9                	xor    ecx,ecx
   140256fdc:	85 d2                	test   edx,edx
   140256fde:	7e 15                	jle    0x140256ff5
   140256fe0:	48 8b 83 d0 01 00 00 	mov    rax,QWORD PTR [rbx+0x1d0]
   140256fe7:	39 38                	cmp    DWORD PTR [rax],edi
   140256fe9:	74 e2                	je     0x140256fcd
   140256feb:	ff c1                	inc    ecx
   140256fed:	48 83 c0 04          	add    rax,0x4
   140256ff1:	3b ca                	cmp    ecx,edx
   140256ff3:	7c f2                	jl     0x140256fe7
   140256ff5:	48 8b 83 d0 01 00 00 	mov    rax,QWORD PTR [rbx+0x1d0]
   140256ffc:	48 8b cb             	mov    rcx,rbx
   140256fff:	89 3c 90             	mov    DWORD PTR [rax+rdx*4],edi
   140257002:	ff 83 e8 01 00 00    	inc    DWORD PTR [rbx+0x1e8]
   140257008:	e8 33 d1 fc ff       	call   0x140224140
   14025700d:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   140257012:	b8 01 00 00 00       	mov    eax,0x1
   140257017:	48 83 c4 20          	add    rsp,0x20
   14025701b:	5f                   	pop    rdi
   14025701c:	c3                   	ret
