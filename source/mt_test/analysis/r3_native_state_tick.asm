
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

0000000140732f50 <.text+0x731f50>:
   140732f50:	40 53                	rex push rbx
   140732f52:	48 83 ec 20          	sub    rsp,0x20
   140732f56:	48 8b 81 c8 04 00 00 	mov    rax,QWORD PTR [rcx+0x4c8]
   140732f5d:	48 8b d9             	mov    rbx,rcx
   140732f60:	83 b8 80 01 00 00 00 	cmp    DWORD PTR [rax+0x180],0x0
   140732f67:	75 30                	jne    0x140732f99
   140732f69:	e8 d2 fc cb ff       	call   0x1403f2c40
   140732f6e:	85 c0                	test   eax,eax
   140732f70:	74 27                	je     0x140732f99
   140732f72:	48 8b 83 c8 04 00 00 	mov    rax,QWORD PTR [rbx+0x4c8]
   140732f79:	48 8b 88 c8 01 00 00 	mov    rcx,QWORD PTR [rax+0x1c8]
   140732f80:	48 85 c9             	test   rcx,rcx
   140732f83:	74 0a                	je     0x140732f8f
   140732f85:	ba 04 00 00 00       	mov    edx,0x4
   140732f8a:	e8 a1 38 12 00       	call   0x140856830
   140732f8f:	48 83 c4 20          	add    rsp,0x20
   140732f93:	5b                   	pop    rbx
   140732f94:	e9 b7 f9 cb ff       	jmp    0x1403f2950
   140732f99:	48 8b 8b c8 04 00 00 	mov    rcx,QWORD PTR [rbx+0x4c8]
   140732fa0:	8b 91 80 01 00 00    	mov    edx,DWORD PTR [rcx+0x180]
   140732fa6:	83 ea 01             	sub    edx,0x1
   140732fa9:	74 12                	je     0x140732fbd
   140732fab:	83 fa 01             	cmp    edx,0x1
   140732fae:	75 2b                	jne    0x140732fdb
   140732fb0:	48 8b cb             	mov    rcx,rbx
   140732fb3:	48 83 c4 20          	add    rsp,0x20
   140732fb7:	5b                   	pop    rbx
   140732fb8:	e9 03 63 01 00       	jmp    0x1407492c0
   140732fbd:	48 8b cb             	mov    rcx,rbx
   140732fc0:	e8 cb 86 f9 ff       	call   0x1406cb690
   140732fc5:	85 c0                	test   eax,eax
   140732fc7:	74 12                	je     0x140732fdb
   140732fc9:	e8 52 df 8d ff       	call   0x140010f20
   140732fce:	48 8b c8             	mov    rcx,rax
   140732fd1:	48 83 c4 20          	add    rsp,0x20
   140732fd5:	5b                   	pop    rbx
   140732fd6:	e9 a5 f3 8d ff       	jmp    0x140012380
   140732fdb:	48 83 c4 20          	add    rsp,0x20
   140732fdf:	5b                   	pop    rbx
   140732fe0:	c3                   	ret
