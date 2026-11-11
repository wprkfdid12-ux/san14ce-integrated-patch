
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001406c4980 <.text+0x6c3980>:
   1406c4980:	40 53                	rex push rbx
   1406c4982:	48 83 ec 20          	sub    rsp,0x20
   1406c4986:	48 8b d9             	mov    rbx,rcx
   1406c4989:	e8 72 47 1a 00       	call   0x140869100
   1406c498e:	48 8b 8b c0 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1c0]
   1406c4995:	48 85 c9             	test   rcx,rcx
   1406c4998:	74 06                	je     0x1406c49a0
   1406c499a:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406c499d:	ff 50 40             	call   QWORD PTR [rax+0x40]
   1406c49a0:	48 8b 8b c8 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1c8]
   1406c49a7:	48 85 c9             	test   rcx,rcx
   1406c49aa:	74 06                	je     0x1406c49b2
   1406c49ac:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406c49af:	ff 50 40             	call   QWORD PTR [rax+0x40]
   1406c49b2:	48 8b 8b d0 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1d0]
   1406c49b9:	48 85 c9             	test   rcx,rcx
   1406c49bc:	74 06                	je     0x1406c49c4
   1406c49be:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406c49c1:	ff 50 40             	call   QWORD PTR [rax+0x40]
   1406c49c4:	48 8b 8b d8 01 00 00 	mov    rcx,QWORD PTR [rbx+0x1d8]
   1406c49cb:	48 85 c9             	test   rcx,rcx
   1406c49ce:	74 0c                	je     0x1406c49dc
   1406c49d0:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406c49d3:	48 83 c4 20          	add    rsp,0x20
   1406c49d7:	5b                   	pop    rbx
   1406c49d8:	48 ff 60 40          	rex.W jmp QWORD PTR [rax+0x40]
   1406c49dc:	48 83 c4 20          	add    rsp,0x20
   1406c49e0:	5b                   	pop    rbx
   1406c49e1:	c3                   	ret
