
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001406b4f20 <.text+0x6b3f20>:
   1406b4f20:	40 53                	rex push rbx
   1406b4f22:	48 83 ec 20          	sub    rsp,0x20
   1406b4f26:	48 8b d9             	mov    rbx,rcx
   1406b4f29:	48 8b 89 d0 01 00 00 	mov    rcx,QWORD PTR [rcx+0x1d0]
   1406b4f30:	48 85 c9             	test   rcx,rcx
   1406b4f33:	74 06                	je     0x1406b4f3b
   1406b4f35:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406b4f38:	ff 50 38             	call   QWORD PTR [rax+0x38]
   1406b4f3b:	48 8b cb             	mov    rcx,rbx
   1406b4f3e:	e8 1d 18 1b 00       	call   0x140866760
   1406b4f43:	48 8b 8b c0 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1c0]
   1406b4f4a:	48 85 c9             	test   rcx,rcx
   1406b4f4d:	74 06                	je     0x1406b4f55
   1406b4f4f:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406b4f52:	ff 50 38             	call   QWORD PTR [rax+0x38]
   1406b4f55:	48 8b 8b c8 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1c8]
   1406b4f5c:	48 85 c9             	test   rcx,rcx
   1406b4f5f:	74 06                	je     0x1406b4f67
   1406b4f61:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406b4f64:	ff 50 38             	call   QWORD PTR [rax+0x38]
   1406b4f67:	48 8b 8b d8 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1d8]
   1406b4f6e:	48 85 c9             	test   rcx,rcx
   1406b4f71:	74 17                	je     0x1406b4f8a
   1406b4f73:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406b4f76:	ff 50 38             	call   QWORD PTR [rax+0x38]
   1406b4f79:	48 8b 8b d8 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1d8]
   1406b4f80:	48 83 c4 20          	add    rsp,0x20
   1406b4f84:	5b                   	pop    rbx
   1406b4f85:	e9 c6 0d 1a 00       	jmp    0x140855d50
   1406b4f8a:	48 83 c4 20          	add    rsp,0x20
   1406b4f8e:	5b                   	pop    rbx
   1406b4f8f:	c3                   	ret
