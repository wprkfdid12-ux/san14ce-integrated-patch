Captured original constructor: bottom group allocation and command links; complete source kept in CP03 analysis.
   1406efeb4:	e8 f7 5c d0 ff       	call   0x1403f5bb0
   1406efeb9:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406efebe:	48 85 c0             	test   rax,rax
   1406efec1:	74 0d                	je     0x1406efed0
   1406efec3:	48 8b c8             	mov    rcx,rax
   1406efec6:	e8 35 a8 15 00       	call   0x14084a700
   1406efecb:	48 8b c8             	mov    rcx,rax
   1406efece:	eb 03                	jmp    0x1406efed3
   1406efed0:	49 8b cd             	mov    rcx,r13
   1406efed3:	48 89 8e c8 01 00 00 	mov    QWORD PTR [rsi+0x1c8],rcx
   1406efeda:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406efedd:	33 d2                	xor    edx,edx
   1406efedf:	44 8d 42 01          	lea    r8d,[rdx+0x1]
   1406efee3:	ff 90 88 01 00 00    	call   QWORD PTR [rax+0x188]
   1406efee9:	41 83 c9 ff          	or     r9d,0xffffffff
   1406efeed:	45 33 c0             	xor    r8d,r8d
   1406efef0:	41 8d 51 06          	lea    edx,[r9+0x6]
   1406efef4:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406efefb:	e8 20 f9 16 00       	call   0x14085f820
   1406eff00:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406eff07:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406f01c8:	4c 8b c6             	mov    r8,rsi
   1406f01cb:	ba 01 00 00 00       	mov    edx,0x1
   1406f01d0:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f01d7:	e8 74 64 16 00       	call   0x140856650
   1406f01dc:	4c 8b 86 c0 01 00 00 	mov    r8,QWORD PTR [rsi+0x1c0]
   1406f01e3:	ba 04 00 00 00       	mov    edx,0x4
   1406f01e8:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f01ef:	e8 5c 64 16 00       	call   0x140856650
   1406f01f4:	4c 8b 86 c8 01 00 00 	mov    r8,QWORD PTR [rsi+0x1c8]
   1406f01fb:	ba 06 00 00 00       	mov    edx,0x6
   1406f0200:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f0207:	e8 44 64 16 00       	call   0x140856650
   1406f020c:	4c 8b 86 d0 01 00 00 	mov    r8,QWORD PTR [rsi+0x1d0]
   1406f0213:	ba ff ff ff 7f       	mov    edx,0x7fffffff
   1406f0218:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f021f:	e8 2c 64 16 00       	call   0x140856650
   1406f0224:	48 8d 05 0d 0f d6 00 	lea    rax,[rip+0xd60f0d]        # 0x141451138
   1406f022b:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406f0230:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406f0235:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
