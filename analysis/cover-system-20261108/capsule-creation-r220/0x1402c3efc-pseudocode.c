_UNKNOWN **__fastcall sub_2C2EFC(
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
        __int64 a15,
        __int64 a16,
        __int64 a17,
        int a18,
        int a19,
        __int64 a20,
        __int64 a21)
{
  _UNKNOWN **result; // rax
  double v28; // xmm0_8
  __int64 v30; // rdx
  __int64 v31; // rdi
  __int64 v32; // rsi
  __int64 v33; // rdx
  __int64 v34; // rdi
  __int64 *v35; // rsi
  __int64 v36; // r8
  int v37; // r9d
  __int64 *v38; // rbx
  __int64 v39; // rdx
  __int64 v48; // rdx
  __int64 v49; // r8
  __int64 v50; // r9
  int v52; // edi
  int v53; // esi
  int v54; // r8d
  int v55; // r9d
  __int64 v56; // rdx
  __int64 v57; // r8
  __int64 v58; // rdi
  __int64 v59; // rsi
  __int64 v60; // rdi
  __int64 v61; // rsi
  __int64 v62; // rsi
  unsigned int v67; // edx
  __int64 v68; // rcx
  __int16 v70; // r15
  __int64 v71; // r14
  __int64 v72; // rax
  __int64 v73; // rcx
  _QWORD *v74; // rdx
  unsigned int v75; // eax
  __int64 v76; // rcx
  __int64 v77; // rdi
  __int64 v78; // rsi
  _QWORD *v79; // r8
  _QWORD *v80; // rax
  _QWORD *v81; // rdx
  unsigned __int64 v83; // rdi
  __int64 v84; // rsi
  __int64 v85; // r8
  int v86; // r9d
  int v93; // eax
  int v94; // r14d
  char v102; // cf
  int v105; // eax
  int v108; // eax
  int v111; // eax
  int v114; // eax
  int v117; // eax
  int v120; // eax
  int v123; // eax
  int v126; // eax
  int v129; // eax
  int v132; // eax
  int v135; // eax
  int v138; // eax
  int v141; // ebx
  _QWORD *v142; // rdx
  __int64 v143; // rdx
  __int64 v144; // rdi
  __int64 v145; // rsi
  __int64 v146; // rax
  __int64 v147; // rsi
  __int64 v148; // r10
  __int64 v149; // rax
  _QWORD *v150; // rbx
  int v165; // eax
  unsigned __int8 v170; // dl
  int v171; // eax
  int v172; // edi
  int v173; // esi
  __int64 v174; // rsi
  int v175; // edi
  int v176; // r8d
  int v177; // r9d
  int v178; // edi
  int v179; // eax
  bool v180; // zf
  bool v182; // al
  bool v183; // zf
  double v187; // xmm2_8
  double v188; // xmm3_8
  double v189; // xmm4_8
  double v190; // xmm5_8
  __int64 v191; // rdi
  __int64 v192; // rsi
  double v193; // xmm0_8
  double v194; // xmm3_8
  double v195; // xmm4_8
  double v196; // xmm5_8
  __int64 v205; // rdi
  __int64 v206; // rsi
  double v207; // xmm0_8
  __int64 v210; // rdx
  __int64 v211; // rdi
  __int64 v212; // rsi
  __int64 v213; // rdx
  __int64 v214; // rdi
  __int64 v215; // rsi
  __int64 v216; // rdx
  __int64 v217; // rdi
  __int64 v218; // rsi
  double v219; // xmm0_8
  double v220; // xmm1_8
  double v221; // xmm2_8
  double v222; // xmm3_8
  double v223; // xmm4_8
  double v224; // xmm5_8
  __int64 v235; // rcx
  __int64 v244; // rdi
  __int64 v245; // rsi
  __int64 v246; // [rsp+0h] [rbp-100h]
  __int64 v247; // [rsp+0h] [rbp-100h]
  __int64 v248; // [rsp+0h] [rbp-100h]
  __int64 v249; // [rsp+8h] [rbp-F8h]
  __int64 v250; // [rsp+10h] [rbp-F0h]
  __int64 v251; // [rsp+30h] [rbp-D0h] BYREF
  int v252; // [rsp+38h] [rbp-C8h]
  int v253; // [rsp+3Ch] [rbp-C4h]
  __int64 v254; // [rsp+40h] [rbp-C0h] BYREF
  unsigned int v256; // [rsp+50h] [rbp-B0h]
  __m256 v257; // [rsp+60h] [rbp-A0h] BYREF
  int v258; // [rsp+80h] [rbp-80h]
  __m256 *v262; // [rsp+90h] [rbp-70h]
  __int64 v264; // [rsp+9Ch] [rbp-64h]
  __int64 v265; // [rsp+A8h] [rbp-58h]
  __int64 v266; // [rsp+B0h] [rbp-50h]
  __int64 v267; // [rsp+B8h] [rbp-48h]
  int v268; // [rsp+C0h] [rbp-40h]
  _QWORD *v269; // [rsp+C8h] [rbp-38h]
  _QWORD *v270; // [rsp+D0h] [rbp-30h]
  unsigned __int64 v271; // [rsp+D8h] [rbp-28h]
  __int128 v272; // [rsp+F0h] [rbp-10h] BYREF
  __int64 v273; // [rsp+100h] [rbp+0h] BYREF
  __m256 v274; // [rsp+110h] [rbp+10h] BYREF
  int v294; // [rsp+17Ch] [rbp+7Ch]
  int v297; // [rsp+188h] [rbp+88h]
  __m256 v300; // [rsp+1A0h] [rbp+A0h] BYREF
  int v301; // [rsp+1C0h] [rbp+C0h]
  char v305; // [rsp+220h] [rbp+120h] BYREF
  _UNKNOWN *retaddr; // [rsp+258h] [rbp+158h] BYREF

  result = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
    vmovaps xmmword ptr [rax-68h], xmm8
    vmovaps xmmword ptr [rax-78h], xmm9
    vmovaps xmmword ptr [rax-88h], xmm10
  }
  _R13 = a12;
  *(_QWORD *)(a12 + 96) = a20;
  _R15 = *(_QWORD *)(a20 + 120);
  v273 = _R15;
  if ( *(_QWORD *)(a12 + 136) != 0 || a21 == 0 )
    goto LABEL_68;
  v28 = sub_1DD944(a14, a20, 22787212, (unsigned int)&v251, a13, a14);
  __asm { vmovss  xmm1, dword ptr cs:17060ACh }
  if ( (unsigned int)sub_2C4B5C(v31, v32, v30, v31, v28, *(double *)&_XMM1) != 0 )
  {
    v252 = 0;
    if ( v253 != 0 )
    {
      v253 = 0;
      sub_DC950(v34, (_DWORD)v35, 2, (unsigned int)&v251, 8, v37);
    }
    goto LABEL_67;
  }
  v38 = nullptr;
  if ( *(int *)(a11 + 228) <= 0LL )
    goto LABEL_81;
  v39 = 0;
  _R8 = *(_QWORD *)(a11 + 220);
  do
  {
    __asm
    {
      vmovss  xmm0, dword ptr [r8]
      vsubss  xmm1, xmm0, dword ptr [rdi]
      vmovss  dword ptr [rsp+250h+var_210], xmm1
      vmovss  xmm2, dword ptr [r8+4]
      vsubss  xmm0, xmm2, dword ptr [rdi+4]
      vmovss  dword ptr [rsp+250h+var_210+4], xmm0
      vmovss  xmm1, dword ptr [r8+8]
      vsubss  xmm2, xmm1, dword ptr [rdi+8]
      vmovss  [rsp+250h+var_208], xmm2
      vmovss  xmm1, dword ptr cs:17060ACh
    }
    if ( (unsigned int)sub_2C4B5C(v34, v35, v39, &v254, *(double *)&_XMM0, *(double *)&_XMM1) != 0 )
      v38 = (__int64 *)(*(_QWORD *)(*(_QWORD *)(a11 + 204) + 8 * v48) + 104LL);
    v39 = v48 + 1;
    _R8 = v49 + 12;
  }
  while ( v39 < v50 );
  if ( v38 == nullptr )
  {
LABEL_81:
    v171 = (*(__int64 (__fastcall **)(__int64, __int64 *, __int64, __int64 *))(*v35 + 760))(v34, v35, v34, v35);
    v35 = (__int64 *)sub_2CEA6C(v172, v173, v172, (int)a11 + 108, v171, 0, v246, v249, v250);
    if ( v35 == nullptr || (v38 = v35 + 13, v35 == (__int64 *)-104LL) )
    {
      v252 = 0;
      if ( v253 != 0 )
      {
        v253 = 0;
        sub_DC950(v34, (_DWORD)v35, 2, (unsigned int)&v251, 8, v37);
      }
      goto LABEL_67;
    }
    sub_19D00C(v34, (_DWORD)v35, v34, a11 + 220, v36, v37, v246);
    v254 = v174;
    sub_C9360(v175, v174, (unsigned int)&v254, a11 + 204, v176, v177, v248);
    LODWORD(v35) = a20;
  }
  _R12 = sub_2D2328(v34, (_DWORD)v35, v39, _R13, _R8, v50, v246);
  LODWORD(v254) = sub_2D184C(v52, v53, _R12, a21, v54, v55, v247, v249, v250);
  v56 = *(_QWORD *)(_R13 + 96);
  v57 = v56 + 348;
  LOBYTE(v56) = *(_BYTE *)(v56 + 340);
  sub_10A78C(v58, v59, v56, &v272, v57);
  sub_576E65(v60, v61, 0, &v257, 136);
  v262 = nullptr;
  __asm
  {
    vxorps  xmm6, xmm6, xmm6
    vmovss  [rbp+150h+var_1B8], xmm6
    vmovaps ymm0, ymmword ptr cs:1706860h
    vmovups [rsp+250h+var_1F0], ymm0
    vmovss  xmm8, dword ptr cs:17061B8h
    vmovss  [rbp+150h+var_1D0], xmm8
    vmovss  [rbp+150h+var_1C4], xmm6
    vmovss  [rbp+150h+var_1C8], xmm6
    vmovss  [rbp+150h+var_1CC], xmm6
  }
  v264 = 0;
  v265 = 0;
  v266 = 0;
  v267 = 0;
  v270 = v269;
  v268 = 1;
  v67 = 0;
  v256 = 0;
  v68 = *v38;
  if ( (unsigned int)((v38[1] - *v38) >> 3) != 0 )
  {
    __asm { vmovups xmm7, [rbp+150h+var_160] }
    v70 = v254;
    do
    {
      v71 = v67;
      v72 = *(_QWORD *)(v68 + 8LL * v67);
      if ( (*(_BYTE *)(v62 + 352) & 2) != 0 )
        *(_DWORD *)(v72 + 60) |= 0x100u;
      else
        *(_DWORD *)(v72 + 60) &= ~0x100u;
      v73 = *(_QWORD *)(*v38 + 8LL * v67);
      if ( (*(_BYTE *)(v62 + 352) & 4) != 0 )
        *(_DWORD *)(v73 + 60) |= 0x800u;
      else
        *(_DWORD *)(v73 + 60) &= ~0x800u;
      v254 = *v38;
      v74 = v270;
      if ( v271 <= (unsigned __int64)v270 )
      {
        v75 = 2 * (v270 - v269) + 2;
        if ( v269 != nullptr )
          v76 = (__int64)(v271 - (_QWORD)v269) >> 3;
        else
          LODWORD(v76) = 0;
        if ( (unsigned int)v76 < v75 )
        {
          __asm { vzeroupper }
          v77 = MEMORY[0x131BE18](0, 8LL * v75, v270, 8LL * v75);
          v79 = (_QWORD *)v77;
          v80 = v270;
          v81 = v269;
          if ( v269 != v270 )
          {
            do
              *v79++ = *v81++;
            while ( v81 != v80 );
            v81 = v269;
          }
          if ( v81 != nullptr )
          {
            MEMORY[0x131BE10](v77, v78, v81, v81, v79);
            v81 = v269;
          }
          v271 = v78 + v77;
          v74 = (_QWORD *)(v77 + 8LL * (unsigned int)(v270 - v81));
          v270 = v74;
          v269 = (_QWORD *)v77;
          v62 = a20;
        }
      }
      *v74 = *(_QWORD *)(v254 + 8 * v71);
      ++v270;
      *(_WORD *)(v269[v71] + 66LL) = v70;
      _RCX = v269[v71];
      __asm { vmovdqu xmmword ptr [rcx+70h], xmm7 }
      v67 = v256 + 1;
      v256 = v67;
      v68 = *v38;
    }
    while ( v67 < (unsigned int)((v38[1] - *v38) >> 3) );
    _R15 = v273;
    _R13 = a12;
  }
  __asm { vzeroupper }
  sub_F1610(0, v62, a13, &v300);
  __asm
  {
    vmovss  xmm2, dword ptr [r12+70h]
    vmovss  [rbp+150h+var_1B8], xmm2
    vmovups ymm0, [rbp+150h+var_B0]
    vmovups [rsp+250h+var_1F0], ymm0
  }
  v258 = v301;
  __asm
  {
    vmovss  xmm0, [rbp+150h+var_8C]
    vmovss  [rbp+150h+var_1CC], xmm0
    vmovss  xmm1, [rbp+150h+var_88]
    vmovss  [rbp+150h+var_1C8], xmm1
    vmovss  xmm0, [rbp+150h+var_84]
    vmovss  [rbp+150h+var_1C4], xmm0
  }
  if ( (*(_BYTE *)(a11 + 188) & 2) != 0 || (*(_DWORD *)(v84 + 336) & 0x80000) == 0 )
  {
    v93 = 1;
    LODWORD(v264) = 1;
  }
  else
  {
    v93 = v264;
  }
  if ( (*(_BYTE *)(_R12 + 108) & 1) != 0 )
    LODWORD(v264) = v93 | 0x20;
  if ( _R15 == 0 || (v94 = 1, (*(_BYTE *)(_R15 + 172) & 1) == 0) )
    v94 = v83;
  __asm
  {
    vmovaps ymm0, ymmword ptr cs:1706860h
    vmovups [rbp+150h+var_140], ymm0
    vmovss  [rbp+150h+var_120], xmm8
    vmovss  [rbp+150h+var_114], xmm6
    vmovss  [rbp+150h+var_118], xmm6
    vmovss  [rbp+150h+var_11C], xmm6
    vmovss  [rbp+150h+var_108], xmm6
    vmovss  [rbp+150h+var_10C], xmm6
    vmovss  [rbp+150h+var_110], xmm6
    vmovss  [rbp+150h+var_F8], xmm6
    vmovss  [rbp+150h+var_FC], xmm6
    vmovss  [rbp+150h+var_100], xmm6
    vmovss  [rbp+150h+var_EC], xmm6
    vmovss  [rbp+150h+var_F0], xmm6
    vmovss  [rbp+150h+var_F4], xmm6
    vmovss  xmm10, dword ptr cs:17064ECh
    vmovss  [rbp+150h+var_E8], xmm10
    vmovss  [rbp+150h+var_104], xmm6
    vmovss  [rbp+150h+var_E4], xmm6
    vmovss  xmm0, dword ptr cs:17061C4h
    vmovss  [rbp+150h+var_E0], xmm0
    vmovss  xmm1, dword ptr cs:1706190h
    vmovss  [rbp+150h+var_DC], xmm1
    vmovss  [rbp+150h+var_D0], xmm1
    vmovss  [rbp+150h+var_CC], xmm1
    vmovss  [rbp+150h+var_D8], xmm6
  }
  v297 = 4;
  v294 = 2304;
  __asm
  {
    vmovss  [rbp+150h+var_C4], xmm1
    vmovss  [rbp+150h+var_C0], xmm6
    vmovss  xmm9, dword ptr cs:17060C4h
  }
  if ( v94 != 0 )
  {
    _RBX = v262;
    _R12 = a11;
  }
  else
  {
    __asm
    {
      vmovss  xmm0, dword ptr [r12+74h]
      vmovss  [rbp+150h+var_E0], xmm0
      vmovss  xmm1, dword ptr [r12+78h]
      vmovss  [rbp+150h+var_E4], xmm1
      vmovss  xmm0, dword ptr cs:17060E4h
      vmovss  [rbp+150h+var_C4], xmm0
      vmovss  xmm1, dword ptr cs:170612Ch
      vmovss  [rbp+150h+var_C0], xmm1
    }
    v297 = 8;
    __asm
    {
      vmovaps xmm0, xmm6
      vmovaps xmm1, xmm6
      vmovaps xmm3, xmm6
    }
    if ( _R15 != 0 )
    {
      __asm
      {
        vmovsd  xmm0, qword ptr [r15+148h]
        vmovsd  qword ptr [r13+78h], xmm0
      }
      v165 = *(_DWORD *)(_R15 + 336);
      *(_DWORD *)(_R13 + 128) = v165;
      __asm
      {
        vmovsd  xmm0, qword ptr [r13+78h]
        vmovsd  qword ptr [r13+6Ch], xmm0
      }
      *(_DWORD *)(_R13 + 116) = v165;
      __asm
      {
        vmovsd  qword ptr [rbp+150h+var_160], xmm0
        vmovss  xmm3, dword ptr [r13+80h]
        vmovss  xmm1, dword ptr [rbp+150h+var_160+4]
        vmovss  xmm0, dword ptr [rbp+150h+var_160]
      }
    }
    _R12 = a11;
    if ( (*(_BYTE *)(a11 + 188) & 1) != 0 || a19 != (_DWORD)v83 )
      v294 = 2432;
    __asm
    {
      vmulss  xmm0, xmm0, xmm9
      vmovss  [rbp+150h+var_100], xmm0
      vmulss  xmm1, xmm1, xmm9
      vmovss  [rbp+150h+var_FC], xmm1
      vmulss  xmm0, xmm3, xmm9
      vmovss  [rbp+150h+var_F8], xmm0
    }
    v170 = 31;
    if ( *(_BYTE *)(v84 + 388) < 0x1Fu )
      v170 = *(_BYTE *)(v84 + 388);
    HIWORD(v264) = v170;
    _RBX = &v274;
    v262 = &v274;
  }
  v102 = 0;
  if ( _R15 != 0 )
  {
    __asm { vzeroupper }
    (*(void (__fastcall **)(unsigned __int64, __int64, __m256 *, __int64, __int64))(*(_QWORD *)_R15 + 1160LL))(
      v83,
      v84,
      &v257,
      _R15,
      v84);
    __asm { vmovss  xmm2, [rbp+150h+var_1B8] }
    _RBX = v262;
  }
  __asm { vcomiss xmm2, xmm6 }
  if ( v102 )
    goto LABEL_80;
  v84 = 519;
  if ( _RBX == nullptr )
  {
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0]
      vcvtss2sd xmm0, xmm0, xmm0
      vzeroupper
    }
    v105 = MEMORY[0x131BE68](v83, 519, *(double *)&_XMM0);
    if ( (v105 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+4]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v108 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v108 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+8]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v111 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v111 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+0Ch]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v114 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v114 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+10h]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v117 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v117 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+14h]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v120 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v120 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+18h]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v123 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v123 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, dword ptr [rsp+250h+var_1F0+1Ch]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v126 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v126 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, [rbp+150h+var_1D0]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v129 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v129 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, [rbp+150h+var_1CC]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v132 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v132 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, [rbp+150h+var_1C8]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v135 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v135 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    __asm
    {
      vmovss  xmm0, [rbp+150h+var_1C4]
      vcvtss2sd xmm0, xmm0, xmm0
    }
    v138 = MEMORY[0x131BE68](v83, v84, *(double *)&_XMM0);
    if ( (v138 & (unsigned int)v84) != 0 )
      goto LABEL_80;
    _RCX = v262;
    if ( v262 == nullptr && HIWORD(v264) != (_WORD)v83 )
      goto LABEL_80;
    v141 = v83;
    v85 = (__int64)v270;
    v142 = v269;
    if ( v262 == nullptr && (unsigned int)(v270 - v269) != 0 )
      goto LABEL_56;
    v85 = v270 - v269;
    if ( (_DWORD)v85 != 0 )
    {
      do
      {
        if ( (*(unsigned __int8 (**)(void))(*(_QWORD *)v142[(unsigned int)v83] + 16LL))() == 0 )
        {
          v83 = 0;
          goto LABEL_80;
        }
        v142 = v269;
        v179 = v141 + 1;
        if ( (*(_BYTE *)(v269[v84] + 60LL) & 7) != 0 )
          v179 = v141;
        v141 = v179;
        LODWORD(v83) = v178 + 1;
      }
      while ( (unsigned int)v83 < (unsigned int)(v270 - v269) );
      v83 = 0;
      _RCX = v262;
      v180 = v179 == 0;
      if ( v179 == 0 )
        goto LABEL_130;
    }
    else
    {
      v83 = 0;
LABEL_130:
      v180 = _RCX == nullptr;
      if ( _RCX != nullptr )
      {
        v180 = SLOBYTE(_RCX[3].m256_f32[3]) >= 0;
        if ( SLOBYTE(_RCX[3].m256_f32[3]) >= 0 )
        {
          __asm { vcomiss xmm6, dword ptr [rcx+3Ch] }
          v180 = fabs(_RCX[1].m256_f32[4]) == 0.0;
          if ( fabs(_RCX[1].m256_f32[4]) == 0.0 )
          {
            v180 = fabs(_RCX[1].m256_f32[5]) == 0.0;
            if ( fabs(_RCX[1].m256_f32[5]) == 0.0 )
            {
              v180 = fabs(_RCX[1].m256_f32[6]) == 0.0;
              if ( fabs(_RCX[1].m256_f32[6]) == 0.0 )
                goto LABEL_135;
            }
          }
        }
      }
    }
    __asm
    {
      vmovss  xmm0, [rbp+150h+var_1B8]
      vucomiss xmm0, xmm6
    }
    LOBYTE(v86) = !v180;
    if ( _RCX != nullptr )
    {
      __asm { vucomiss xmm6, dword ptr [rcx+3Ch] }
      LOBYTE(v85) = 1;
    }
    else
    {
      LOBYTE(v85) = 0;
    }
    v182 = _RCX != nullptr
        && (fabs(_RCX[1].m256_f32[4]) != 0.0 || fabs(_RCX[1].m256_f32[5]) != 0.0 || fabs(_RCX[1].m256_f32[6]) != 0.0);
    if ( v141 != 0 )
    {
      if ( !v180 )
      {
        if ( (_BYTE)v85 != 0 || v182 )
          goto LABEL_135;
LABEL_56:
        *(_QWORD *)(_R13 + 136) = v83;
        sub_2AB6C8(v83, a21, v142, a21, v85);
        v146 = sub_2D1CD8(v144, v145, v143, v145);
        if ( (*(_BYTE *)(a20 + 352) & 0x40) != 0 && v94 == 0 )
        {
          v235 = v267;
          if ( v146 != 0 )
            v235 = v146;
          v267 = v235;
        }
        v149 = (*(__int64 (__fastcall **)(__int64, __int64, __m256 *, __int64))(*(_QWORD *)v148 + 56LL))(
                 a20,
                 v147,
                 &v257,
                 v148);
        v150 = (_QWORD *)v149;
        if ( v149 != 0 )
        {
          if ( v94 == 0 )
          {
            *(double *)&_XMM0 = (*(double (__fastcall **)(__int64, __int64 *, _QWORD, __int64))(*(_QWORD *)v149 + 368LL))(
                                  v34,
                                  v35,
                                  *(_QWORD *)(*(_QWORD *)v149 + 368LL),
                                  v149);
            __asm
            {
              vmovaps xmm6, xmm0
              vmovss  xmm1, dword ptr cs:17066B4h
            }
            sub_576EC5(*(double *)&_XMM0, *(double *)&_XMM1, v187, v188, v189, v190, *(double *)&_XMM6);
            __asm
            {
              vmovss  xmm2, dword ptr [r12+0C8h]
              vmovss  xmm1, dword ptr cs:17061E4h
              vcomiss xmm2, xmm1
            }
            if ( !v102 )
              __asm { vminss  xmm1, xmm2, dword ptr cs:1706150h }
            __asm
            {
              vmulss  xmm7, xmm1, xmm0
              vdivss  xmm6, xmm7, xmm6
            }
            (*(void (__fastcall **)(__int64, __int64, __int64 *, _QWORD *, double, double, double, double, double, double, double))(*v150 + 384LL))(
              v191,
              v192,
              &v273,
              v150,
              v193,
              *(double *)&_XMM1,
              *(double *)&_XMM2,
              v194,
              v195,
              v196,
              *(double *)&_XMM6);
            __asm
            {
              vmulss  xmm2, xmm6, dword ptr [rbp+150h+var_150]
              vmovss  dword ptr [rsp+250h+var_210], xmm2
              vmulss  xmm0, xmm6, dword ptr [rbp+150h+var_150+4]
              vmovss  dword ptr [rsp+250h+var_210+4], xmm0
              vmulss  xmm3, xmm6, [rbp+150h+var_148]
              vmovss  [rsp+250h+var_208], xmm3
            }
            v207 = (*(double (__fastcall **)(__int64, __int64, __int64 *, _QWORD *))(*v150 + 376LL))(
                     v205,
                     v206,
                     &v254,
                     v150);
            __asm { vmovaps xmm1, xmm7 }
            (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD *, double, double))(*v150 + 360LL))(
              v211,
              v212,
              v210,
              v150,
              v207,
              *(double *)&_XMM1);
            if ( (unsigned int)sub_DA120(v214, v215, v213, _R12 + 96) == 0 )
            {
              __asm
              {
                vmulss  xmm8, xmm9, dword ptr [rcx+8]
                vmulss  xmm7, xmm9, dword ptr [rcx+4]
                vmulss  xmm6, xmm9, dword ptr [rcx]
              }
              (*(void (__fastcall **)(__int64, __int64, __int64 *, _QWORD *, double, double, double, double, double, double, double, double))(*v150 + 320LL))(
                v217,
                v218,
                &v254,
                v150,
                v219,
                v220,
                v221,
                v222,
                v223,
                v224,
                *(double *)&_XMM6,
                *(double *)&_XMM7);
              __asm
              {
                vaddss  xmm1, xmm6, dword ptr [rsp+250h+var_210]
                vmovss  dword ptr [rbp+150h+var_160], xmm1
                vaddss  xmm0, xmm7, dword ptr [rsp+250h+var_210+4]
                vmovss  dword ptr [rbp+150h+var_160+4], xmm0
                vaddss  xmm2, xmm8, [rsp+250h+var_208]
                vmovss  dword ptr [rbp+150h+var_160+8], xmm2
              }
              (*(void (__fastcall **)(__int64, __int64, __int128 *, _QWORD *))(*v150 + 248LL))(v244, v245, &v272, v150);
            }
            if ( _R15 != 0 )
            {
              __asm
              {
                vmovss  xmm5, dword ptr [r15+150h]
                vmovss  xmm2, dword ptr [r15+148h]
                vmovss  xmm0, dword ptr [r15+14Ch]
                vmulss  xmm3, xmm0, xmm0
                vmulss  xmm2, xmm2, xmm2
                vaddss  xmm4, xmm3, xmm2
                vmulss  xmm0, xmm5, xmm5
                vaddss  xmm3, xmm4, xmm0
                vsqrtss xmm2, xmm3, xmm3
                vcomiss xmm2, dword ptr cs:17060ACh
              }
              __asm { vmovaps xmm1, xmm10 }
              (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD *, double, double))(*v150 + 696LL))(
                v217,
                v218,
                v216,
                v150,
                *(double *)&_XMM0,
                *(double *)&_XMM1);
            }
            else
            {
              (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD *))(*v150 + 704LL))(v217, v218, v216, v150);
            }
          }
          if ( (*(_BYTE *)(v34 + 352) & 1) != 0 )
            (*(void (__fastcall **)(__int64, __int64 *, __int64, _QWORD *))(*v150 + 168LL))(v34, v35, 2, v150);
          *(_QWORD *)(_R13 + 136) = v150;
          v150[1] = _R13;
          *(_DWORD *)(_R13 + 132) = *((_DWORD *)v35 + 24);
          if ( _R15 != 0 )
            (*(void (__fastcall **)(__int64, __int64 *, _QWORD *, __int64, __m256 *, __int64))(*(_QWORD *)_R15 + 1168LL))(
              v34,
              v35,
              v150,
              _R15,
              &v257,
              v34);
        }
        if ( v269 != nullptr )
          MEMORY[0x131BE10](v34, v35, v33, v269, v36);
        v269 = nullptr;
        v270 = nullptr;
        v271 = 0;
        v252 = 0;
        if ( v253 != 0 )
        {
          v253 = 0;
          sub_DC950(v34, (_DWORD)v35, 2, (unsigned int)&v251, 8, v37);
        }
LABEL_67:
        v252 = 0;
        result = (_UNKNOWN **)sub_DC200(v34, (_DWORD)v35, v33, (unsigned int)&v251, v36, v37, v246, v249, v250);
        goto LABEL_68;
      }
      v183 = (_BYTE)v85 == 0;
    }
    else
    {
      if ( !v180 || (_BYTE)v85 == 0 )
        goto LABEL_135;
      v183 = !v182;
    }
    if ( v183 )
      goto LABEL_135;
    goto LABEL_56;
  }
  __asm { vcomiss xmm6, dword ptr [rbx+3Ch] }
LABEL_80:
  v142 = v269;
LABEL_135:
  if ( v142 != nullptr )
  {
    __asm { vzeroupper }
    MEMORY[0x131BE10](v83, v84, v142, v142, v85);
  }
  v269 = (_QWORD *)v83;
  v270 = (_QWORD *)v83;
  v271 = v83;
  v252 = v83;
  if ( v253 != (_DWORD)v83 )
  {
    v253 = v83;
    __asm { vzeroupper }
    sub_DC950(v83, v84, 2, (unsigned int)&v251, 8, v86);
  }
  v252 = v83;
  __asm { vzeroupper }
  result = (_UNKNOWN **)sub_DC200(v83, v84, (_DWORD)v142, (unsigned int)&v251, v85, v86, v246, v249, v250);
LABEL_68:
  _R11 = &v305;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
  }
  return result;
}
