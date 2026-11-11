
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

00000001402a9010 <.text+0x2a8010>:
   1402a9010:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   1402a9015:	48 89 74 24 10       	mov    QWORD PTR [rsp+0x10],rsi
   1402a901a:	57                   	push   rdi
   1402a901b:	48 83 ec 20          	sub    rsp,0x20
   1402a901f:	8b f2                	mov    esi,edx
   1402a9021:	48 8b d9             	mov    rbx,rcx
   1402a9024:	e8 87 90 f8 ff       	call   0x1402320b0
   1402a9029:	85 c0                	test   eax,eax
   1402a902b:	0f 85 43 01 00 00    	jne    0x1402a9174
   1402a9031:	33 d2                	xor    edx,edx
   1402a9033:	48 8b cb             	mov    rcx,rbx
   1402a9036:	e8 05 96 f8 ff       	call   0x140232640
   1402a903b:	85 c0                	test   eax,eax
   1402a903d:	0f 85 31 01 00 00    	jne    0x1402a9174
   1402a9043:	bf 00 40 00 00       	mov    edi,0x4000
   1402a9048:	66 85 bb 5e 01 00 00 	test   WORD PTR [rbx+0x15e],di
   1402a904f:	0f 85 1f 01 00 00    	jne    0x1402a9174
   1402a9055:	8d 50 01             	lea    edx,[rax+0x1]
   1402a9058:	48 8b cb             	mov    rcx,rbx
   1402a905b:	e8 10 9b f8 ff       	call   0x140232b70
   1402a9060:	85 c0                	test   eax,eax
   1402a9062:	74 1e                	je     0x1402a9082
   1402a9064:	b8 02 00 00 00       	mov    eax,0x2
   1402a9069:	33 d2                	xor    edx,edx
   1402a906b:	3b f0                	cmp    esi,eax
   1402a906d:	0f 44 d0             	cmove  edx,eax
   1402a9070:	8b c2                	mov    eax,edx
   1402a9072:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   1402a9077:	48 8b 74 24 38       	mov    rsi,QWORD PTR [rsp+0x38]
   1402a907c:	48 83 c4 20          	add    rsp,0x20
   1402a9080:	5f                   	pop    rdi
   1402a9081:	c3                   	ret
   1402a9082:	ba 01 00 00 00       	mov    edx,0x1
   1402a9087:	48 8b cb             	mov    rcx,rbx
   1402a908a:	e8 e1 9a f8 ff       	call   0x140232b70
   1402a908f:	85 c0                	test   eax,eax
   1402a9091:	0f 85 dd 00 00 00    	jne    0x1402a9174
   1402a9097:	33 d2                	xor    edx,edx
   1402a9099:	48 8b cb             	mov    rcx,rbx
   1402a909c:	e8 9f 95 f8 ff       	call   0x140232640
   1402a90a1:	85 c0                	test   eax,eax
   1402a90a3:	0f 85 cb 00 00 00    	jne    0x1402a9174
   1402a90a9:	66 85 bb 5e 01 00 00 	test   WORD PTR [rbx+0x15e],di
   1402a90b0:	0f 85 be 00 00 00    	jne    0x1402a9174
   1402a90b6:	0f b6 83 bc 01 00 00 	movzx  eax,BYTE PTR [rbx+0x1bc]
   1402a90bd:	85 c0                	test   eax,eax
   1402a90bf:	0f 84 af 00 00 00    	je     0x1402a9174
   1402a90c5:	0f b6 bb c2 01 00 00 	movzx  edi,BYTE PTR [rbx+0x1c2]
   1402a90cc:	33 d2                	xor    edx,edx
   1402a90ce:	4c 8b 1d 43 4e 4d 02 	mov    r11,QWORD PTR [rip+0x24d4e43]        # 0x14277df18
   1402a90d5:	44 8b ca             	mov    r9d,edx
   1402a90d8:	48 85 ff             	test   rdi,rdi
   1402a90db:	74 37                	je     0x1402a9114
   1402a90dd:	44 8b c2             	mov    r8d,edx
   1402a90e0:	4d 8d 93 f8 5d 08 00 	lea    r10,[r11+0x85df8]
   1402a90e7:	49 ff c0             	inc    r8
   1402a90ea:	49 83 f8 06          	cmp    r8,0x6
   1402a90ee:	7f 05                	jg     0x1402a90f5
   1402a90f0:	49 8b 0a             	mov    rcx,QWORD PTR [r10]
   1402a90f3:	eb 07                	jmp    0x1402a90fc
   1402a90f5:	49 8b 8b f0 5d 08 00 	mov    rcx,QWORD PTR [r11+0x85df0]
   1402a90fc:	0f b6 41 10          	movzx  eax,BYTE PTR [rcx+0x10]
   1402a9100:	3b c6                	cmp    eax,esi
   1402a9102:	75 07                	jne    0x1402a910b
   1402a9104:	0f b7 41 12          	movzx  eax,WORD PTR [rcx+0x12]
   1402a9108:	44 03 c8             	add    r9d,eax
   1402a910b:	49 83 c2 08          	add    r10,0x8
   1402a910f:	4c 3b c7             	cmp    r8,rdi
   1402a9112:	7c d3                	jl     0x1402a90e7
   1402a9114:	44 0f b6 93 c3 01 00 	movzx  r10d,BYTE PTR [rbx+0x1c3]
   1402a911b:	00 
   1402a911c:	4d 85 d2             	test   r10,r10
   1402a911f:	74 40                	je     0x1402a9161
   1402a9121:	4d 8d 83 10 5e 08 00 	lea    r8,[r11+0x85e10]
   1402a9128:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
   1402a912f:	00 
   1402a9130:	48 8d 42 04          	lea    rax,[rdx+0x4]
   1402a9134:	48 83 f8 06          	cmp    rax,0x6
   1402a9138:	7f 05                	jg     0x1402a913f
   1402a913a:	49 8b 00             	mov    rax,QWORD PTR [r8]
   1402a913d:	eb 07                	jmp    0x1402a9146
   1402a913f:	49 8b 83 f0 5d 08 00 	mov    rax,QWORD PTR [r11+0x85df0]
   1402a9146:	0f b6 48 10          	movzx  ecx,BYTE PTR [rax+0x10]
   1402a914a:	3b ce                	cmp    ecx,esi
   1402a914c:	75 07                	jne    0x1402a9155
   1402a914e:	0f b7 48 12          	movzx  ecx,WORD PTR [rax+0x12]
   1402a9152:	44 03 c9             	add    r9d,ecx
   1402a9155:	48 ff c2             	inc    rdx
   1402a9158:	49 83 c0 08          	add    r8,0x8
   1402a915c:	49 3b d2             	cmp    rdx,r10
   1402a915f:	7c cf                	jl     0x1402a9130
   1402a9161:	41 8b c1             	mov    eax,r9d
   1402a9164:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   1402a9169:	48 8b 74 24 38       	mov    rsi,QWORD PTR [rsp+0x38]
   1402a916e:	48 83 c4 20          	add    rsp,0x20
   1402a9172:	5f                   	pop    rdi
   1402a9173:	c3                   	ret
   1402a9174:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   1402a9179:	33 c0                	xor    eax,eax
   1402a917b:	48 8b 74 24 38       	mov    rsi,QWORD PTR [rsp+0x38]
   1402a9180:	48 83 c4 20          	add    rsp,0x20
   1402a9184:	5f                   	pop    rdi
   1402a9185:	c3                   	ret
