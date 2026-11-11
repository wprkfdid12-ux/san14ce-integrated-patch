
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

00000001402a9190 <.text+0x2a8190>:
   1402a9190:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   1402a9195:	57                   	push   rdi
   1402a9196:	48 83 ec 20          	sub    rsp,0x20
   1402a919a:	8b fa                	mov    edi,edx
   1402a919c:	48 8b d9             	mov    rbx,rcx
   1402a919f:	e8 1c 53 08 00       	call   0x14032e4c0
   1402a91a4:	85 c0                	test   eax,eax
   1402a91a6:	75 0d                	jne    0x1402a91b5
   1402a91a8:	33 c0                	xor    eax,eax
   1402a91aa:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   1402a91af:	48 83 c4 20          	add    rsp,0x20
   1402a91b3:	5f                   	pop    rdi
   1402a91b4:	c3                   	ret
   1402a91b5:	e8 56 9d f2 ff       	call   0x1401d2f10
   1402a91ba:	48 85 c0             	test   rax,rax
   1402a91bd:	74 0f                	je     0x1402a91ce
   1402a91bf:	e8 4c 9d f2 ff       	call   0x1401d2f10
   1402a91c4:	e8 97 8f f2 ff       	call   0x1401d2160
   1402a91c9:	83 f8 ff             	cmp    eax,0xffffffff
   1402a91cc:	75 da                	jne    0x1402a91a8
   1402a91ce:	48 8b 05 43 4d 4d 02 	mov    rax,QWORD PTR [rip+0x24d4d43]        # 0x14277df18
   1402a91d5:	48 8b 88 e8 71 08 00 	mov    rcx,QWORD PTR [rax+0x871e8]
   1402a91dc:	e8 5f 65 f6 ff       	call   0x14020f740
   1402a91e1:	83 f8 03             	cmp    eax,0x3
   1402a91e4:	75 19                	jne    0x1402a91ff
   1402a91e6:	e8 35 7d d6 ff       	call   0x140010f20
   1402a91eb:	48 8b c8             	mov    rcx,rax
   1402a91ee:	48 8d 15 1b 38 10 01 	lea    rdx,[rip+0x110381b]        # 0x1413aca10
   1402a91f5:	e8 36 5f 2c 00       	call   0x14056f130
   1402a91fa:	48 85 c0             	test   rax,rax
   1402a91fd:	75 a9                	jne    0x1402a91a8
   1402a91ff:	8b d7                	mov    edx,edi
   1402a9201:	48 8b cb             	mov    rcx,rbx
   1402a9204:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   1402a9209:	48 83 c4 20          	add    rsp,0x20
   1402a920d:	5f                   	pop    rdi
   1402a920e:	e9 0d 6f 75 02       	jmp    0x142a00120
