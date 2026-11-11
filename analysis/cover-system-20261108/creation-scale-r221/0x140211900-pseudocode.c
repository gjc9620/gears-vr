__int64 __fastcall sub_210900(
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
        __int64 a13,
        int a14,
        int a15,
        int a16,
        int a17,
        int a18,
        __int64 a19)
{
  __int64 v25; // rdx
  __int64 v27; // rsi
  int v28; // r8d
  int v29; // r9d
  double v30; // xmm0_8
  __int64 v31; // rcx
  int v32; // edi
  int v33; // esi
  int v34; // r8d
  int v35; // r9d
  int v36; // edx
  int v37; // edi
  int v38; // esi
  int v39; // r8d
  int v40; // r9d
  int v52; // r12d
  unsigned int v53; // ebx
  int v54; // r14d
  __int64 v55; // r15
  int v56; // eax
  __int64 v57; // r14
  int v58; // r15d
  __int64 v59; // rbx
  __int64 v60; // r12
  __int64 v61; // r14
  unsigned int v62; // eax
  __int64 v63; // rdx
  __int64 v64; // rdi
  __int64 v65; // rsi
  __int64 v66; // rdi
  __int64 v67; // rsi
  double v68; // xmm0_8
  double v69; // xmm1_8
  double v70; // xmm2_8
  double v71; // xmm3_8
  double v72; // xmm4_8
  double v73; // xmm5_8
  __m128 v74; // xmm6
  __m128 v75; // xmm7
  double v76; // xmm0_8
  double v77; // xmm2_8
  int v78; // eax
  __int64 v79; // rax
  __int64 v80; // rax
  int v81; // r12d
  BOOL v85; // r15d
  __int64 v86; // r11
  __int64 v87; // rbx
  __int64 v88; // r11
  unsigned int v90; // ebx
  int v91; // edi
  int v92; // esi
  int v93; // r8d
  int v94; // r9d
  int v95; // r11d
  int v96; // eax
  __int64 v97; // rdi
  __int64 v98; // rsi
  __int64 v99; // rdi
  __int64 v100; // rsi
  __int64 v101; // rdx
  __int64 v102; // rdi
  __int64 v103; // rsi
  int v104; // r8d
  int v105; // r9d
  char v108; // cf
  char v109; // zf
  int v123; // ebx
  __int64 v129; // rsi
  __int64 v130; // rcx
  __int64 v131; // rdi
  __int64 v133; // r12
  int v134; // r15d
  __int64 v135; // rax
  __int64 v136; // r13
  __int64 v137; // rdx
  __int64 v138; // rdi
  __int64 v139; // rsi
  __int64 result; // rax
  __int64 v148; // rcx
  __int64 v149; // r14
  __int64 v150; // rax
  __int64 v151; // rbx
  __int64 v152; // rdi
  __int64 v153; // rsi
  int v154; // eax
  __int64 v155; // rdi
  __int64 v156; // rsi
  __int64 v157; // rcx
  __int64 v158; // rdx
  __int64 v159; // rdi
  __int64 v160; // rsi
  __int64 v161; // rax
  __int64 v162; // rcx
  __int64 v224; // [rsp+0h] [rbp-1E8h] BYREF
  __int64 v225; // [rsp+8h] [rbp-1E0h]
  __int64 v226; // [rsp+10h] [rbp-1D8h]
  int v227; // [rsp+18h] [rbp-1D0h]
  int v228; // [rsp+20h] [rbp-1C8h]
  __int64 v229; // [rsp+28h] [rbp-1C0h]
  __int64 v230; // [rsp+30h] [rbp-1B8h]
  int v231; // [rsp+38h] [rbp-1B0h]
  __int64 v232; // [rsp+40h] [rbp-1A8h]
  _DWORD v233[2]; // [rsp+48h] [rbp-1A0h] BYREF
  int v234; // [rsp+50h] [rbp-198h]
  int v235; // [rsp+54h] [rbp-194h]
  int v236; // [rsp+58h] [rbp-190h] BYREF
  __int64 v239; // [rsp+68h] [rbp-180h] BYREF
  char v240; // [rsp+70h] [rbp-178h] BYREF
  int v241; // [rsp+78h] [rbp-170h]
  int v242; // [rsp+7Ch] [rbp-16Ch]
  __int128 v243; // [rsp+80h] [rbp-168h] BYREF
  _BYTE v247[48]; // [rsp+C0h] [rbp-128h] BYREF
  char v249; // [rsp+100h] [rbp-E8h] BYREF
  unsigned __int64 v250; // [rsp+140h] [rbp-A8h]
  char v251; // [rsp+1B0h] [rbp-38h] BYREF
  _UNKNOWN *retaddr; // [rsp+1E8h] [rbp+0h] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
    vmovaps xmmword ptr [rax-68h], xmm8
    vmovaps xmmword ptr [rax-78h], xmm9
    vmovaps xmmword ptr [rax-88h], xmm10
    vmovaps xmmword ptr [rax-98h], xmm11
  }
  v250 = (unsigned __int64)&v224 ^ MEMORY[0x1ACA440];
  LODWORD(v232) = a14;
  v239 = a19;
  *(_QWORD *)(a12 + 96) = *(_QWORD *)(a11 + 120);
  v30 = sub_1DD944(a11, a12, 22787200, (unsigned int)&v240, a13, a14);
  v31 = *(_QWORD *)(v27 + 96);
  if ( v31 != 0 )
  {
    if ( *(_DWORD *)(v31 + 8) == -1 )
      sub_1DD944(_RDI, v27, 20260504, (unsigned int)v233, v28, v29);
    else
      sub_1E0740(_RDI, v27, v233, v31 + 72, v30);
    sub_1E13DC(v32, v33, (unsigned int)v233, (unsigned int)&v240, v34, v35, v224, v225, v226);
    v234 = 0;
    if ( v235 != 0 )
    {
      v235 = 0;
      sub_DC950(v37, v38, 2, (unsigned int)v233, 8, v40);
    }
    v234 = 0;
    sub_DC200(v37, v38, v36, (unsigned int)v233, v39, v40, v224, v225, v226);
  }
  if ( a19 != 0 )
  {
    v25 = *(_QWORD *)(_RDI + 536);
    if ( v25 != 0 )
    {
      __asm
      {
        vmovss  xmm0, dword ptr [rdi+1E8h]
        vmulss  xmm3, xmm0, dword ptr [rdi+1F4h]
        vmulss  xmm4, xmm0, dword ptr [rdi+1F0h]
        vmulss  xmm11, xmm0, dword ptr [rdi+1ECh]
        vmovss  [rsp+1E8h+var_190], xmm11
        vmovss  [rsp+1E8h+var_18C], xmm4
        vmovss  [rsp+1E8h+var_188], xmm3
      }
      _RAX = *(_QWORD *)(v27 + 96);
      if ( _RAX != 0 )
      {
        __asm
        {
          vmovss  xmm0, dword ptr [rax+198h]
          vmulss  xmm2, xmm0, dword ptr [rax+1A4h]
          vmulss  xmm1, xmm0, dword ptr [rax+1A0h]
          vmulss  xmm0, xmm0, dword ptr [rax+19Ch]
          vmulss  xmm11, xmm0, xmm11
          vmovss  [rsp+1E8h+var_190], xmm11
          vmulss  xmm1, xmm1, xmm4
          vmovss  [rsp+1E8h+var_18C], xmm1
          vmulss  xmm0, xmm2, xmm3
          vmovss  [rsp+1E8h+var_188], xmm0
        }
      }
      if ( (unsigned int)sub_2CF600(_RDI, v27, v25, &v236) != 0 )
      {
        v52 = *(_DWORD *)(v27 + 116);
        if ( v52 == *(_DWORD *)(a13 + 112) && *(_DWORD *)(v27 + 132) == *(_DWORD *)(a13 + 144) )
        {
          *(_DWORD *)(v27 + 104) = -1;
          v53 = 0;
          v54 = *(_DWORD *)(v25 + 172);
          if ( v54 > 0 )
          {
            v55 = *(_QWORD *)(v25 + 164);
            while ( 1 )
            {
              v56 = sub_212DDC(_RDI, v27, *(_QWORD *)(68LL * v53 + v55), a13, v28, v29, v224, v225);
              if ( v56 != -1 )
                break;
              if ( (int)++v53 >= v54 )
                goto LABEL_18;
            }
            *(_DWORD *)(v27 + 104) = v56;
          }
LABEL_18:
          v57 = 0;
          if ( *(_DWORD *)(v27 + 104) != -1 )
          {
            v58 = 0;
            if ( v52 > 0 )
            {
              v59 = 0;
              v60 = v239;
              do
              {
                v61 = *(_QWORD *)(v59 + *(_QWORD *)(v27 + 108));
                *(_DWORD *)(v61 + 104) = v58;
                v62 = sub_1D28D4(
                        _RDI,
                        v27,
                        *(_QWORD *)(*(_QWORD *)(v59 + *(_QWORD *)(a13 + 104)) + 180LL),
                        _RDI,
                        v28,
                        v29,
                        v224,
                        v225);
                if ( v62 != -1 )
                {
                  sub_1D30B8(_RDI, v27, &v243, _RDI, v62);
                  sub_F10F4(v64, v65, v63, &v243);
                  sub_2C2EFC(
                    v68,
                    v69,
                    v70,
                    v71,
                    v72,
                    v73,
                    v74,
                    v75,
                    v66,
                    v67,
                    *(_QWORD *)(v59 + *(_QWORD *)(a13 + 104)),
                    v61,
                    (__int64)&v243,
                    (int)&v236,
                    v224,
                    v225,
                    v226,
                    v227,
                    v232,
                    v66,
                    v60);
                  v78 = *(_DWORD *)(v61 + 160);
                  if ( (v78 & 3) != 0 )
                  {
                    if ( (v78 & 0x20) != 0 )
                    {
                      v161 = *(_QWORD *)(_RDI + 120);
                      if ( v161 != 0 )
                      {
                        v162 = *(_QWORD *)(v161 + 148);
                        if ( v162 != 0 )
                        {
                          _RAX = *(_QWORD *)(v162 + 436);
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
                                                  _RDI,
                                                  v27,
                                                  v25,
                                                  0,
                                                  v76,
                                                  *(double *)&_XMM1,
                                                  v77,
                                                  *(double *)&_XMM3);
                            __asm
                            {
                              vmovups xmm7, xmm3
                              vmovups xmm9, xmm2
                              vmovups xmm8, xmm1
                              vmovups xmm10, xmm0
                              vmovups xmm6, [rsp+1E8h+var_168]
                              vshufps xmm4, xmm6, xmm6, 0FFh
                              vmulps  xmm5, xmm4, xmm3
                              vshufps xmm3, xmm6, xmm6, 55h ; 'U'
                              vmulps  xmm2, xmm3, xmm1
                              vaddps  xmm4, xmm5, xmm2
                              vshufps xmm0, xmm6, xmm6, 0AAh
                              vmulps  xmm3, xmm0, xmm9
                              vshufps xmm1, xmm6, xmm6, 0
                              vmulps  xmm2, xmm1, xmm10
                              vaddps  xmm0, xmm3, xmm2
                              vaddps  xmm3, xmm4, xmm0
                              vmovups [rsp+1E8h+var_168], xmm3
                              vmovups xmm6, [rsp+1E8h+var_158]
                              vshufps xmm0, xmm6, xmm6, 0FFh
                              vmulps  xmm3, xmm0, xmm7
                              vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                              vmulps  xmm2, xmm1, xmm8
                              vaddps  xmm5, xmm3, xmm2
                              vshufps xmm0, xmm6, xmm6, 0AAh
                              vmulps  xmm4, xmm0, xmm9
                              vshufps xmm1, xmm6, xmm6, 0
                              vmulps  xmm2, xmm1, xmm10
                              vaddps  xmm0, xmm4, xmm2
                              vaddps  xmm3, xmm5, xmm0
                              vmovups [rsp+1E8h+var_158], xmm3
                              vmovups xmm6, [rsp+1E8h+var_148]
                              vshufps xmm0, xmm6, xmm6, 0FFh
                              vmulps  xmm3, xmm0, xmm7
                              vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                              vmulps  xmm2, xmm1, xmm8
                              vaddps  xmm5, xmm3, xmm2
                              vshufps xmm0, xmm6, xmm6, 0AAh
                              vmulps  xmm4, xmm0, xmm9
                              vshufps xmm1, xmm6, xmm6, 0
                              vmulps  xmm2, xmm1, xmm10
                              vaddps  xmm0, xmm4, xmm2
                              vaddps  xmm3, xmm5, xmm0
                              vmovups [rsp+1E8h+var_148], xmm3
                              vmovups xmm6, [rsp+1E8h+var_138]
                              vshufps xmm0, xmm6, xmm6, 0FFh
                              vmulps  xmm3, xmm0, xmm7
                              vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                              vmulps  xmm2, xmm1, xmm8
                              vaddps  xmm5, xmm3, xmm2
                              vshufps xmm0, xmm6, xmm6, 0AAh
                              vmulps  xmm4, xmm0, xmm9
                              vshufps xmm1, xmm6, xmm6, 0
                              vmulps  xmm2, xmm1, xmm10
                              vaddps  xmm0, xmm4, xmm2
                              vaddps  xmm3, xmm5, xmm0
                              vmovups [rsp+1E8h+var_138], xmm3
                            }
                          }
                        }
                      }
                    }
                    sub_214A8C(_RDI, v27, *(_DWORD *)(v61 + 160) & 1, v61, (*(_DWORD *)(v61 + 160) & 2) != 0, &v243);
                  }
                }
                ++v58;
                v59 += 8;
              }
              while ( v58 < *(_DWORD *)(v27 + 116) );
              v57 = 0;
            }
            if ( MEMORY[0x1D6FD34] > *(_DWORD *)(*(_QWORD *)__readgsqword(0x58u) + 20LL) )
            {
              sub_53E120(_RDI, v27, v25, 30866740);
              if ( MEMORY[0x1D6FD34] == -1 )
              {
                sub_1DEBA0(_RDI, v27, 22787216, 30866744, 0, 1, v224, v225, v226, v227, 1);
                sub_53E0B4(v159, v160, v158, 30866740);
              }
            }
            v79 = *(_QWORD *)(_RDI + 552);
            if ( v79 == 0
              || (*(_DWORD *)(v79 + 8) == -1
                ? (sub_1DEBA0(_RDI, v27, 20260504, (unsigned int)&v239, 0, 1, v224, v225, v226, v227, 1), v80 = v239)
                : (v80 = *(_QWORD *)(v79 + 72)),
                  (_DWORD)v80 != MEMORY[0x1D6FD38] || (LODWORD(v232) = 1, HIDWORD(v80) != MEMORY[0x1D6FD3C])) )
            {
              LODWORD(v232) = 0;
            }
            v81 = 0;
            if ( *(int *)(v27 + 132) > 0 )
            {
              __asm
              {
                vmovdqu xmm8, xmmword ptr cs:1706090h
                vmovss  xmm9, dword ptr cs:17060ACh
                vmovss  xmm7, dword ptr cs:17060C4h
              }
              while ( 1 )
              {
                *(_DWORD *)(*(_QWORD *)(v57 + *(_QWORD *)(v27 + 124)) + 112LL) = v81;
                v85 = true;
                v86 = *(_QWORD *)(v57 + *(_QWORD *)(a13 + 136));
                v87 = *(_QWORD *)(v86 + 104);
                if ( v87 != 0 )
                  v85 = sub_1D28D4(_RDI, v27, *(_QWORD *)(v86 + 104), _RDI, v28, v29, v224, v225) != -1;
                v88 = *(_QWORD *)(v86 + 112);
                if ( v88 != 0 && (unsigned int)sub_1D28D4(_RDI, v27, v88, _RDI, v28, v29, v224, v225) == -1 || !v85 )
                  goto LABEL_43;
                _R15 = *(_QWORD *)(v57 + *(_QWORD *)(a13 + 136));
                v90 = sub_1D28D4(_RDI, v27, v87, _RDI, v28, v29, v224, v225);
                v96 = sub_1D28D4(v91, v92, v95, v91, v93, v94, v224, v225);
                v233[0] = v96;
                if ( (_DWORD)v232 == 0 || v90 == -1 || v96 == -1 )
                  break;
                sub_1D30B8(v97, v98, v247, v97, v90);
                sub_1D30B8(v99, v100, &v243, v99, v233[0]);
                *(double *)&_XMM0 = sub_131F10(v102, v103, v101, &v243);
                __asm
                {
                  vandps  xmm0, xmm0, xmm8
                  vcomiss xmm0, xmm9
                }
                if ( v108 | v109 )
                {
                  v231 = 0;
                  goto LABEL_61;
                }
                sub_15E17C(v97, v98, (unsigned int)&v249, (unsigned int)&v243, v104, v105, v224, v225, v226, v227);
                __asm
                {
                  vmovups xmm6, [rsp+1E8h+var_F8]
                  vshufps xmm0, xmm6, xmm6, 0FFh
                  vmulps  xmm3, xmm0, xmmword ptr [rax+30h]
                  vshufps xmm1, xmm6, xmm6, 55h ; 'U'
                  vmulps  xmm2, xmm1, xmmword ptr [rax+10h]
                  vaddps  xmm5, xmm3, xmm2
                  vshufps xmm0, xmm6, xmm6, 0AAh
                  vmulps  xmm4, xmm0, xmmword ptr [rax+20h]
                  vshufps xmm1, xmm6, xmm6, 0
                  vmulps  xmm2, xmm1, xmmword ptr [rax]
                  vaddps  xmm0, xmm4, xmm2
                  vaddps  xmm3, xmm5, xmm0
                  vmovsd  xmm6, qword ptr [r15+9Ch]
                }
                v123 = *(_DWORD *)(_R15 + 164);
                __asm
                {
                  vmulss  xmm0, xmm3, xmm7
                  vmovss  dword ptr [r15+9Ch], xmm0
                  vshufps xmm1, xmm3, xmm3, 55h ; 'U'
                  vmulss  xmm0, xmm1, xmm7
                  vmovss  dword ptr [r15+0A0h], xmm0
                  vshufps xmm3, xmm3, xmm3, 0AAh
                  vmulss  xmm0, xmm3, xmm7
                  vmovss  dword ptr [r15+0A4h], xmm0
                }
                v130 = *(_QWORD *)(v129 + 124);
                v231 = 0;
                v230 = v131;
                v229 = *(_QWORD *)(v129 + 96);
                __asm { vmovss  [rsp+1E8h+var_1C8], xmm11 }
                sub_211CB8(
                  v131,
                  v129,
                  v131,
                  *(_QWORD *)(v57 + v130),
                  v131,
                  _R15,
                  v224,
                  v225,
                  v226,
                  v227,
                  v228,
                  v229,
                  v131,
                  0);
                __asm { vmovsd  qword ptr [r15+9Ch], xmm6 }
                *(_DWORD *)(_R15 + 164) = v123;
LABEL_43:
                ++v81;
                v57 += 8;
                if ( v81 >= *(_DWORD *)(v27 + 132) )
                  goto LABEL_44;
              }
              v231 = 0;
LABEL_61:
              v230 = v97;
              v229 = *(_QWORD *)(v98 + 96);
              v157 = *(_QWORD *)(v98 + 124);
              __asm { vmovss  [rsp+1E8h+var_1C8], xmm11 }
              sub_211CB8(
                v97,
                v98,
                v97,
                *(_QWORD *)(v57 + v157),
                v97,
                _R15,
                v224,
                v225,
                v226,
                v227,
                v228,
                v229,
                v97,
                v231);
              goto LABEL_43;
            }
LABEL_44:
            _RDI = 1;
            if ( *(int *)(v27 + 116) > 1 )
            {
              v133 = 8;
              do
              {
                v134 = 0;
                if ( (int)_RDI > 0 )
                {
                  v135 = v27 + 140;
                  v136 = 0;
                  do
                  {
                    v233[0] = v134;
                    v233[1] = _RDI;
                    if ( sub_211000(_RDI, v27, v233, v135) != 0 )
                    {
                      v148 = *(_QWORD *)(v27 + 108);
                      v149 = *(_QWORD *)(*(_QWORD *)(v133 + v148) + 136LL);
                      v150 = *(_QWORD *)(*(_QWORD *)(v148 + v136) + 136LL);
                      v232 = v150;
                      if ( v149 != 0 && v150 != 0 )
                      {
                        v151 = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v149 + 8LL))(
                                 _RDI,
                                 v27,
                                 v25,
                                 v149);
                        v154 = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64, __int64))(*(_QWORD *)v151 + 192LL))(
                                 v152,
                                 v153,
                                 v149,
                                 v151,
                                 v232);
                        (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64, _QWORD))(*(_QWORD *)v151 + 184LL))(
                          v155,
                          v156,
                          v149,
                          v151,
                          v232,
                          v154 | 1u);
                      }
                    }
                    ++v134;
                    v136 += 8;
                    v135 = v27 + 140;
                  }
                  while ( v134 < (int)_RDI );
                }
                _RDI = (unsigned int)(_RDI + 1);
                v133 += 8;
              }
              while ( (int)_RDI < *(_DWORD *)(v27 + 116) );
            }
          }
        }
      }
    }
  }
  v241 = 0;
  if ( v242 != 0 )
  {
    v242 = 0;
    sub_DC950(_RDI, v27, 2, (unsigned int)&v240, 8, v29);
  }
  v241 = 0;
  sub_DC200(_RDI, v27, v25, (unsigned int)&v240, v28, v29, v224, v225, v226);
  result = sub_53D9B0(v138, v139, v137, (unsigned __int64)&v224 ^ v250);
  _R11 = &v251;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
    vmovaps xmm11, xmmword ptr [r11-60h]
  }
  return result;
}
