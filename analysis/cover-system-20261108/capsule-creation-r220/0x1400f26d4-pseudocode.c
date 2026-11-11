_UNKNOWN **__fastcall sub_F16D4(
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a9,
        __int64 a10,
        __int64 a11,
        __int64 a12,
        int a13,
        unsigned int a14)
{
  _UNKNOWN **result; // rax
  bool v23; // zf
  __int64 v25; // rdi
  int v26; // r14d
  __int64 v27; // rsi
  __int64 v28; // r15
  __int64 v29; // rbx
  int v30; // r8d
  int v31; // r9d
  int v32; // eax
  __int64 v33; // r13
  __int64 v34; // rdx
  __int64 v35; // rdi
  __int64 v36; // rsi
  __int64 v91; // rdi
  __int64 v92; // rsi
  __int64 v93; // rdi
  __int64 v94; // rsi
  char v115; // cf
  __int64 v116; // rdi
  __int64 v117; // rsi
  __int64 v129; // rdx
  int v185; // ecx
  int v186; // ebx
  __int64 v188; // r14
  __int64 v191; // rax
  __int64 v235; // rcx
  __int64 (__fastcall *v237)(__int64, __int64, _DWORD *, __int64, _DWORD *, __int128 *); // r10
  __int64 v246; // rsi
  __int64 v247; // rax
  __int64 v248; // rcx
  int v306; // [rsp+0h] [rbp-1E8h]
  __int64 v307; // [rsp+8h] [rbp-1E0h]
  __int128 v310; // [rsp+40h] [rbp-1A8h] BYREF
  _DWORD v311[3]; // [rsp+50h] [rbp-198h] BYREF
  _DWORD v312[4]; // [rsp+80h] [rbp-168h] BYREF
  _DWORD v313[4]; // [rsp+90h] [rbp-158h] BYREF
  __int128 v314; // [rsp+A0h] [rbp-148h] BYREF
  _QWORD v318[28]; // [rsp+E0h] [rbp-108h] BYREF
  char v319; // [rsp+1C0h] [rbp-28h] BYREF
  _UNKNOWN *retaddr; // [rsp+1E8h] [rbp+0h] BYREF

  result = &retaddr;
  v23 = *(_DWORD *)(a12 + 800) == 0;
  _R12 = a11;
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
    vmovaps xmmword ptr [rax-58h], xmm8
    vmovaps xmmword ptr [rax-68h], xmm9
    vmovaps xmmword ptr [rax-78h], xmm10
    vmovaps xmmword ptr [rax-88h], xmm11
    vmovaps xmmword ptr [rax-98h], xmm12
    vmovaps xmmword ptr [rax-0A8h], xmm13
    vmovaps xmmword ptr [rax-0B8h], xmm14
    vmovaps xmmword ptr [rax-0C8h], xmm15
  }
  v25 = a12;
  if ( !v23 )
  {
    result = *(_UNKNOWN ***)(a12 + 536);
    v185 = *(_DWORD *)(a12 + 628);
    if ( v185 == *((_DWORD *)result + 43) )
    {
      v186 = 1;
      if ( v185 > 1 )
      {
        _RSI = 64;
        v188 = 68;
        _R15 = 25032016;
        do
        {
          _RDX = *(_QWORD *)(v25 + 620);
          v191 = *(_QWORD *)(v25 + 536);
          __asm
          {
            vmovss  xmm6, dword ptr [rsi+rdx+30h]
            vmovss  xmm5, dword ptr [rsi+rdx+34h]
            vmovss  xmm7, dword ptr [rsi+rdx+38h]
            vmulss  xmm1, xmm5, dword ptr [r12+10h]
            vmulss  xmm0, xmm6, dword ptr [r12]
            vaddss  xmm3, xmm1, xmm0
            vmulss  xmm1, xmm7, dword ptr [r12+20h]
            vaddss  xmm2, xmm1, dword ptr [r12+30h]
            vmulss  xmm1, xmm6, dword ptr [r12+4]
            vaddss  xmm0, xmm3, xmm2
            vmulss  xmm3, xmm5, dword ptr [r12+14h]
            vmovss  [rsp+1E8h+var_158], xmm0
            vmulss  xmm0, xmm7, dword ptr [r12+24h]
            vaddss  xmm2, xmm0, dword ptr [r12+34h]
            vmulss  xmm0, xmm6, dword ptr [r12+8]
            vaddss  xmm4, xmm3, xmm1
            vmulss  xmm3, xmm5, dword ptr [r12+18h]
            vaddss  xmm1, xmm4, xmm2
            vmovss  [rsp+1E8h+var_154], xmm1
            vmulss  xmm1, xmm7, dword ptr [r12+28h]
            vaddss  xmm2, xmm1, dword ptr [r12+38h]
            vaddss  xmm4, xmm3, xmm0
            vaddss  xmm0, xmm4, xmm2
            vmovss  [rsp+1E8h+var_150], xmm0
          }
          _RAX = (__int64)*(int *)(*(_QWORD *)(v191 + 164) + v188 + 56) << 6;
          __asm
          {
            vmovss  xmm6, dword ptr [rax+rdx+30h]
            vmovss  xmm5, dword ptr [rax+rdx+34h]
            vmovss  xmm7, dword ptr [rax+rdx+38h]
            vmulss  xmm1, xmm6, dword ptr [r12]
            vmulss  xmm0, xmm5, dword ptr [r12+10h]
          }
          __asm
          {
            vaddss  xmm3, xmm1, xmm0
            vmulss  xmm1, xmm7, dword ptr [r12+20h]
            vaddss  xmm2, xmm1, dword ptr [r12+30h]
            vmulss  xmm1, xmm6, dword ptr [r12+4]
            vaddss  xmm0, xmm3, xmm2
            vmulss  xmm3, xmm5, dword ptr [r12+14h]
            vmovss  [rsp+1E8h+var_168], xmm0
            vmulss  xmm0, xmm7, dword ptr [r12+24h]
            vaddss  xmm2, xmm0, dword ptr [r12+34h]
            vmulss  xmm0, xmm6, dword ptr [r12+8]
            vaddss  xmm4, xmm3, xmm1
            vmulss  xmm3, xmm5, dword ptr [r12+18h]
            vaddss  xmm1, xmm4, xmm2
            vmovss  [rsp+1E8h+var_164], xmm1
            vmulss  xmm1, xmm7, dword ptr [r12+28h]
            vaddss  xmm2, xmm1, dword ptr [r12+38h]
            vaddss  xmm4, xmm3, xmm0
            vaddss  xmm0, xmm4, xmm2
            vmovss  [rsp+1E8h+var_160], xmm0
          }
          v235 = *(_QWORD *)(MEMORY[0x1C1F890] + 440LL) + 520LL;
          __asm { vxorps  xmm1, xmm1, xmm1 }
          v237 = *(__int64 (__fastcall **)(__int64, __int64, _DWORD *, __int64, _DWORD *, __int128 *))(*(_QWORD *)v235 + 32LL);
          _RAX = MEMORY[0x1AF9B4A];
          __asm { vmovss  xmm0, dword ptr [r15+rax*4] }
          _RAX = MEMORY[0x1AF9B49];
          __asm
          {
            vmovss  dword ptr [rsp+1E8h+var_1A8], xmm0
            vmovss  xmm0, dword ptr [r15+rax*4]
          }
          _RAX = MEMORY[0x1AF9B48];
          __asm
          {
            vmovss  dword ptr [rsp+1E8h+var_1A8+4], xmm0
            vmovss  xmm0, dword ptr [r15+rax*4]
          }
          __asm
          {
            vcvtsi2ss xmm1, xmm1, eax
            vmovss  dword ptr [rsp+1E8h+var_1A8+8], xmm0
            vmulss  xmm0, xmm1, dword ptr cs:17060BCh
            vmovss  dword ptr [rsp+1E8h+var_1A8+0Ch], xmm0
          }
          result = (_UNKNOWN **)v237(v25, _RSI, v313, v235, v312, &v310);
          ++v186;
          _RSI = v246 + 64;
          v188 += 68;
        }
        while ( v186 < *(_DWORD *)(v25 + 628) );
      }
    }
  }
  if ( *(_QWORD *)(v25 + 584) != 0 )
  {
    result = *(_UNKNOWN ***)(v25 + 592);
    if ( result != nullptr && *(_QWORD *)(v25 + 536) != 0 )
    {
      v26 = 0;
      if ( *((int *)result + 29) > 0 )
      {
        v27 = 0;
        do
        {
          v28 = *(_QWORD *)(*(char **)((char *)result + 108) + v27);
          v29 = *(_QWORD *)(v28 + 136);
          if ( v29 != 0
            && (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64, _QWORD))(*(_QWORD *)v29 + 728LL))(
                 v25,
                 v27,
                 126,
                 *(_QWORD *)(v28 + 136)) == 0 )
          {
            v32 = sub_F8D80(
                    v25,
                    v27,
                    *(_QWORD *)(*(_QWORD *)(*(_QWORD *)(*(_QWORD *)(v25 + 584) + 104LL) + v27) + 180LL),
                    *(_QWORD *)(v25 + 536),
                    v30,
                    v31,
                    v306,
                    v307);
            v33 = v32;
            if ( v32 != -1 )
            {
              if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v29 + 728LL))(
                     v25,
                     v27,
                     128,
                     v29) != 0
                || a13 != 0 )
              {
                _RAX = *(_QWORD *)(v35 + 620);
                __asm
                {
                  vmovups xmm11, xmmword ptr [r12+10h]
                  vmovups xmm10, xmmword ptr [r12+30h]
                  vmovups xmm13, xmmword ptr [r12]
                  vmovups xmm12, xmmword ptr [r12+20h]
                }
                _RCX = v33 << 6;
                __asm
                {
                  vmovups xmm6, xmmword ptr [rcx+rax]
                  vmovups xmm7, xmmword ptr [rcx+rax+10h]
                  vmovups xmm9, xmmword ptr [rcx+rax+20h]
                  vmovups xmm14, xmmword ptr [rcx+rax+30h]
                  vshufps xmm0, xmm6, xmm6, 0FFh
                  vmulps  xmm3, xmm0, xmm10
                  vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                  vmulps  xmm2, xmm1, xmm11
                  vaddps  xmm5, xmm3, xmm2
                  vshufps xmm0, xmm6, xmm6, 0AAh
                  vmulps  xmm4, xmm0, xmm12
                  vshufps xmm1, xmm6, xmm6, 0
                  vmulps  xmm2, xmm1, xmm13
                  vaddps  xmm0, xmm4, xmm2
                  vaddps  xmm8, xmm5, xmm0
                  vshufps xmm0, xmm7, xmm7, 55h ; 'U'
                  vmulps  xmm2, xmm0, xmm11
                  vshufps xmm1, xmm7, xmm7, 0FFh
                  vmulps  xmm3, xmm1, xmm10
                  vaddps  xmm5, xmm3, xmm2
                  vshufps xmm0, xmm7, xmm7, 0
                  vmulps  xmm2, xmm0, xmm13
                  vshufps xmm1, xmm7, xmm7, 0AAh
                  vmulps  xmm4, xmm1, xmm12
                  vaddps  xmm1, xmm4, xmm2
                  vaddps  xmm7, xmm5, xmm1
                  vshufps xmm1, xmm9, xmm9, 55h ; 'U'
                  vmulps  xmm2, xmm1, xmm11
                  vshufps xmm0, xmm9, xmm9, 0FFh
                  vmulps  xmm3, xmm0, xmm10
                  vaddps  xmm5, xmm3, xmm2
                  vshufps xmm1, xmm9, xmm9, 0
                  vmulps  xmm2, xmm1, xmm13
                  vshufps xmm0, xmm9, xmm9, 0AAh
                  vmulps  xmm4, xmm0, xmm12
                  vaddps  xmm0, xmm4, xmm2
                  vaddps  xmm6, xmm5, xmm0
                  vshufps xmm0, xmm14, xmm14, 55h ; 'U'
                  vmulps  xmm2, xmm0, xmm11
                  vshufps xmm1, xmm14, xmm14, 0FFh
                  vmulps  xmm3, xmm1, xmm10
                  vshufps xmm1, xmm14, xmm14, 0AAh
                  vaddps  xmm5, xmm3, xmm2
                  vshufps xmm0, xmm14, xmm14, 0
                  vmulps  xmm2, xmm0, xmm13
                  vmulps  xmm4, xmm1, xmm12
                  vaddps  xmm1, xmm4, xmm2
                  vaddps  xmm3, xmm5, xmm1
                }
                __asm
                {
                  vmovups [rsp+1E8h+var_D8], xmm3
                  vmovups [rsp+1E8h+var_108], xmm8
                  vmovups [rsp+1E8h+var_F8], xmm7
                  vmovups [rsp+1E8h+var_E8], xmm6
                }
                sub_F10F4(v35, v36, v34, v318);
                sub_F1610(v91, v92, v318, v311);
                (*(void (__fastcall **)(__int64, __int64, __int128 *, __int64))(*(_QWORD *)v29 + 72LL))(
                  v93,
                  v94,
                  &v314,
                  v29);
                __asm
                {
                  vmovss  xmm4, [rsp+1E8h+var_190]
                  vmovss  xmm6, [rsp+1E8h+var_194]
                  vmulss  xmm0, xmm6, [rsp+1E8h+var_184]
                  vmulss  xmm1, xmm0, [rsp+1E8h+var_180]
                  vmovss  xmm5, [rsp+1E8h+var_198]
                  vmulss  xmm0, xmm5, [rsp+1E8h+var_188]
                  vmulss  xmm0, xmm0, [rsp+1E8h+var_178]
                  vaddss  xmm2, xmm1, xmm0
                  vmulss  xmm0, xmm4, [rsp+1E8h+var_18C]
                  vmulss  xmm0, xmm0, [rsp+1E8h+var_17C]
                  vmulss  xmm1, xmm4, [rsp+1E8h+var_188]
                  vaddss  xmm3, xmm2, xmm0
                  vmulss  xmm2, xmm1, [rsp+1E8h+var_180]
                  vmulss  xmm0, xmm6, [rsp+1E8h+var_18C]
                  vmulss  xmm1, xmm0, [rsp+1E8h+var_178]
                  vsubss  xmm4, xmm3, xmm2
                  vmulss  xmm2, xmm5, [rsp+1E8h+var_184]
                  vmulss  xmm0, xmm2, [rsp+1E8h+var_17C]
                  vsubss  xmm3, xmm4, xmm1
                  vsubss  xmm1, xmm3, xmm0
                  vcomiss xmm1, dword ptr cs:17060ACh
                }
                if ( !(v115 | v23) && (unsigned int)sub_F1278(v35, v36, &v314, v311) == 0 )
                {
                  if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v29 + 728LL))(
                         v35,
                         v36,
                         128,
                         v29) == 0
                    || a14 != 0 )
                  {
                    (*(void (__fastcall **)(__int64, __int64, _DWORD *, __int64))(*(_QWORD *)v29 + 40LL))(
                      v116,
                      v117,
                      v311,
                      v29);
                  }
                  else
                  {
                    (*(void (__fastcall **)(__int64, __int64, _DWORD *, __int64))(*(_QWORD *)v29 + 104LL))(
                      v116,
                      v117,
                      v311,
                      v29);
                  }
                }
              }
              if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v29 + 728LL))(
                     v35,
                     v36,
                     128,
                     v29) == 0 )
              {
                v129 = *(unsigned int *)(v28 + 160);
                if ( (v129 & 3) != 0 )
                {
                  __asm
                  {
                    vmovups xmm10, xmmword ptr [r12+10h]
                    vmovups xmm11, xmmword ptr [r12+20h]
                  }
                  _RAX = *(_QWORD *)(v25 + 620);
                  __asm
                  {
                    vmovups xmm9, xmmword ptr [r12+30h]
                    vmovups xmm12, xmmword ptr [r12]
                  }
                  _RCX = v33 << 6;
                  __asm
                  {
                    vmovups xmm6, xmmword ptr [rcx+rax]
                    vmovups xmm7, xmmword ptr [rcx+rax+10h]
                    vmovups xmm8, xmmword ptr [rcx+rax+20h]
                    vmovups xmm13, xmmword ptr [rcx+rax+30h]
                    vshufps xmm0, xmm6, xmm6, 0FFh
                    vmulps  xmm3, xmm0, xmm9
                    vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                    vmulps  xmm2, xmm1, xmm10
                    vaddps  xmm5, xmm3, xmm2
                    vshufps xmm0, xmm6, xmm6, 0AAh
                    vmulps  xmm4, xmm0, xmm11
                    vshufps xmm1, xmm6, xmm6, 0
                    vmulps  xmm2, xmm1, xmm12
                    vaddps  xmm0, xmm4, xmm2
                    vaddps  xmm14, xmm5, xmm0
                    vshufps xmm0, xmm7, xmm7, 55h ; 'U'
                    vmulps  xmm2, xmm0, xmm10
                    vshufps xmm1, xmm7, xmm7, 0FFh
                    vmulps  xmm3, xmm1, xmm9
                    vaddps  xmm5, xmm3, xmm2
                    vshufps xmm0, xmm7, xmm7, 0
                    vmulps  xmm2, xmm0, xmm12
                    vshufps xmm1, xmm7, xmm7, 0AAh
                    vmulps  xmm4, xmm1, xmm11
                    vaddps  xmm1, xmm4, xmm2
                    vaddps  xmm15, xmm5, xmm1
                    vshufps xmm1, xmm8, xmm8, 55h ; 'U'
                    vmulps  xmm2, xmm1, xmm10
                    vshufps xmm0, xmm8, xmm8, 0FFh
                    vmulps  xmm3, xmm0, xmm9
                    vaddps  xmm5, xmm3, xmm2
                    vshufps xmm1, xmm8, xmm8, 0
                    vmulps  xmm2, xmm1, xmm12
                    vshufps xmm0, xmm8, xmm8, 0AAh
                    vmulps  xmm4, xmm0, xmm11
                    vaddps  xmm0, xmm4, xmm2
                    vaddps  xmm3, xmm5, xmm0
                    vshufps xmm0, xmm13, xmm13, 55h ; 'U'
                    vmulps  xmm2, xmm0, xmm10
                    vshufps xmm1, xmm13, xmm13, 0FFh
                    vmovups [rsp+1E8h+var_1A8], xmm3
                    vmovups xmm10, [rsp+1E8h+var_1A8]
                    vmulps  xmm3, xmm1, xmm9
                    vshufps xmm1, xmm13, xmm13, 0AAh
                    vaddps  xmm5, xmm3, xmm2
                    vmulps  xmm4, xmm1, xmm11
                    vshufps xmm0, xmm13, xmm13, 0
                    vmulps  xmm2, xmm0, xmm12
                    vaddps  xmm1, xmm4, xmm2
                    vaddps  xmm11, xmm5, xmm1
                    vmovups [rsp+1E8h+var_118], xmm11
                    vmovups [rsp+1E8h+var_148], xmm14
                    vmovups [rsp+1E8h+var_138], xmm15
                    vmovups [rsp+1E8h+var_128], xmm10
                  }
                  if ( (v129 & 0x20) != 0 )
                  {
                    v247 = *(_QWORD *)(v25 + 120);
                    if ( v247 != 0 )
                    {
                      v248 = *(_QWORD *)(v247 + 148);
                      if ( v248 != 0 )
                      {
                        _RAX = *(_QWORD *)(v248 + 436);
                        if ( _RAX != 0 )
                        {
                          __asm
                          {
                            vmovups ymm0, ymmword ptr [rax+0A0h]
                            vmovups ymm2, ymmword ptr [rax+0C0h]
                            vextractf128 xmm1, ymm0, 1
                            vextractf128 xmm3, ymm2, 1
                          }
                          __asm { vzeroupper }
                          *(double *)&_XMM0 = sub_136A60(
                                                v25,
                                                v27,
                                                v129,
                                                0,
                                                *(double *)&_XMM0,
                                                *(double *)&_XMM1,
                                                *(double *)&_XMM2,
                                                *(double *)&_XMM3);
                          __asm
                          {
                            vmovups xmm9, xmm0
                            vmovups xmm6, xmm3
                            vmovups xmm7, xmm1
                            vmovups xmm8, xmm2
                            vshufps xmm5, xmm14, xmm14, 55h ; 'U'
                            vshufps xmm0, xmm14, xmm14, 0
                            vshufps xmm4, xmm14, xmm14, 0FFh
                            vmulps  xmm4, xmm4, xmm3
                            vmulps  xmm3, xmm5, xmm1
                            vaddps  xmm3, xmm4, xmm3
                            vshufps xmm1, xmm14, xmm14, 0AAh
                            vmulps  xmm5, xmm1, xmm2
                            vmulps  xmm2, xmm0, xmm9
                            vaddps  xmm1, xmm5, xmm2
                            vaddps  xmm3, xmm3, xmm1
                            vmovups [rsp+1E8h+var_148], xmm3
                            vshufps xmm1, xmm15, xmm15, 55h ; 'U'
                            vmulps  xmm2, xmm1, xmm7
                            vshufps xmm0, xmm15, xmm15, 0FFh
                            vmulps  xmm4, xmm0, xmm6
                            vaddps  xmm5, xmm4, xmm2
                            vshufps xmm1, xmm15, xmm15, 0
                            vmulps  xmm2, xmm1, xmm9
                            vshufps xmm0, xmm15, xmm15, 0AAh
                            vmulps  xmm3, xmm0, xmm8
                            vaddps  xmm0, xmm3, xmm2
                            vaddps  xmm3, xmm5, xmm0
                            vmovups [rsp+1E8h+var_138], xmm3
                            vshufps xmm0, xmm10, xmm10, 55h ; 'U'
                            vmulps  xmm2, xmm0, xmm7
                            vshufps xmm1, xmm10, xmm10, 0FFh
                            vmulps  xmm4, xmm1, xmm6
                            vaddps  xmm5, xmm4, xmm2
                            vshufps xmm0, xmm10, xmm10, 0
                            vmulps  xmm2, xmm0, xmm9
                            vshufps xmm1, xmm10, xmm10, 0AAh
                            vmulps  xmm3, xmm1, xmm8
                            vaddps  xmm1, xmm3, xmm2
                            vaddps  xmm3, xmm5, xmm1
                            vshufps xmm1, xmm11, xmm11, 55h ; 'U'
                            vmulps  xmm2, xmm1, xmm7
                            vshufps xmm0, xmm11, xmm11, 0FFh
                            vmulps  xmm4, xmm0, xmm6
                            vshufps xmm0, xmm11, xmm11, 0AAh
                            vaddps  xmm5, xmm4, xmm2
                            vmovups [rsp+1E8h+var_128], xmm3
                            vmulps  xmm3, xmm0, xmm8
                            vshufps xmm1, xmm11, xmm11, 0
                            vmulps  xmm2, xmm1, xmm9
                            vaddps  xmm0, xmm3, xmm2
                            vaddps  xmm3, xmm5, xmm0
                            vmovups [rsp+1E8h+var_118], xmm3
                          }
                        }
                      }
                    }
                  }
                  sub_2141E4(v25, v27, &v314, v28, a14);
                }
              }
            }
          }
          result = *(_UNKNOWN ***)(v25 + 592);
          ++v26;
          v27 += 8;
        }
        while ( v26 < *((_DWORD *)result + 29) );
      }
    }
  }
  _R11 = &v319;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
    vmovaps xmm11, xmmword ptr [r11-60h]
    vmovaps xmm12, xmmword ptr [r11-70h]
    vmovaps xmm13, xmmword ptr [r11-80h]
    vmovaps xmm14, xmmword ptr [r11-90h]
    vmovaps xmm15, xmmword ptr [r11-0A0h]
  }
  return result;
}
