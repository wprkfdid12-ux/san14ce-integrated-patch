
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001408a44b0 <.text+0x8a34b0>:
   1408a44b0:	40 57                	rex push rdi
   1408a44b2:	48 83 ec 30          	sub    rsp,0x30
   1408a44b6:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   1408a44bd:	ff ff 
   1408a44bf:	48 89 5c 24 40       	mov    QWORD PTR [rsp+0x40],rbx
   1408a44c4:	48 8b f9             	mov    rdi,rcx
   1408a44c7:	e8 f4 36 fd ff       	call   0x140877bc0
   1408a44cc:	48 8b 9f b0 01 00 00 	mov    rbx,QWORD PTR [rdi+0x1b0]
   1408a44d3:	48 85 db             	test   rbx,rbx
   1408a44d6:	74 35                	je     0x1408a450d
   1408a44d8:	48 8d 4b 10          	lea    rcx,[rbx+0x10]
   1408a44dc:	e8 3f 58 76 ff       	call   0x140009d20
   1408a44e1:	90                   	nop
   1408a44e2:	48 8d 05 3f 5e bb 00 	lea    rax,[rip+0xbb5e3f]        # 0x14145a328
   1408a44e9:	48 89 03             	mov    QWORD PTR [rbx],rax
   1408a44ec:	48 8b cb             	mov    rcx,rbx
   1408a44ef:	e8 9c 34 dc ff       	call   0x140667990
   1408a44f4:	90                   	nop
   1408a44f5:	ba 28 00 00 00       	mov    edx,0x28
   1408a44fa:	48 8b cb             	mov    rcx,rbx
   1408a44fd:	e8 02 48 75 00       	call   0x140ff8d04
   1408a4502:	48 c7 87 b0 01 00 00 	mov    QWORD PTR [rdi+0x1b0],0x0
   1408a4509:	00 00 00 00 
   1408a450d:	48 8b 5c 24 40       	mov    rbx,QWORD PTR [rsp+0x40]
   1408a4512:	48 83 c4 30          	add    rsp,0x30
   1408a4516:	5f                   	pop    rdi
   1408a4517:	c3                   	ret
