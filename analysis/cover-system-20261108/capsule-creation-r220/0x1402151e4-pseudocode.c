__int64 __fastcall sub_2141E4(
        __int64 a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        double a7,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a10,
        __int64 _RDX,
        __int64 a12,
        int a13)
{
  __int64 v24; // r14
  __int64 v27; // rdi
  __int64 v28; // rsi
  __int64 v29; // rdi
  __int64 v30; // rsi
  int v32; // edi
  int v33; // esi
  int v34; // edi
  int v35; // esi
  __int64 v36; // rdi
  __int64 v37; // rsi
  __int64 v41; // rcx
  char *v42; // rax
  __int64 v47; // rcx
  char *v48; // rax
  double v54; // xmm0_8
  __int64 v63; // rdx
  __int64 v64; // rdi
  __int64 v65; // rsi
  __int64 v66; // rdx
  __int64 v67; // rdi
  __int64 v68; // rsi
  double v69; // xmm0_8
  double v78; // xmm0_8
  double v79; // xmm1_8
  double v80; // xmm2_8
  double v81; // xmm3_8
  double v82; // xmm4_8
  double v83; // xmm5_8
  __int64 v86; // rdi
  __int64 v87; // rsi
  char v101; // cf
  char v102; // zf
  __int64 result; // rax
  int v115; // eax
  __int64 v116; // rax
  __int64 v117; // rbx
  int v118; // r14d
  __int64 v119; // r15
  __int64 v120; // rcx
  __int64 v128; // rdi
  __int64 v129; // rsi
  __int64 v138; // rdi
  __int64 v139; // rsi
  __int64 *v140; // r9
  __int64 v141; // r10
  __int64 v143; // rsi
  __int64 v145; // rdi
  void (__fastcall *v146)(__int64, __int64, int *, __int64, int *, __int128 *); // rbx
  __int64 v151; // rdi
  __int64 v152; // rsi
  __int64 v153; // [rsp+0h] [rbp-100h] BYREF
  __int64 v154; // [rsp+8h] [rbp-F8h]
  __int64 v155; // [rsp+10h] [rbp-F0h]
  char v156; // [rsp+20h] [rbp-E0h]
  __int64 v157; // [rsp+30h] [rbp-D0h] BYREF
  int v159; // [rsp+40h] [rbp-C0h] BYREF
  int v162; // [rsp+4Ch] [rbp-B4h] BYREF
  _DWORD v165[4]; // [rsp+58h] [rbp-A8h] BYREF
  _QWORD v166[3]; // [rsp+68h] [rbp-98h] BYREF
  __m256 v167; // [rsp+80h] [rbp-80h] BYREF
  __int128 v169; // [rsp+C0h] [rbp-40h] BYREF
  _BYTE v170[48]; // [rsp+D0h] [rbp-30h] BYREF
  __int64 v171; // [rsp+100h] [rbp+0h] BYREF
  int v172; // [rsp+108h] [rbp+8h]
  _QWORD v173[2]; // [rsp+110h] [rbp+10h]
  char v174; // [rsp+120h] [rbp+20h] BYREF
  char v178; // [rsp+138h] [rbp+38h] BYREF
  char v179; // [rsp+13Ch] [rbp+3Ch] BYREF
  char v183; // [rsp+150h] [rbp+50h] BYREF
  int v190; // [rsp+180h] [rbp+80h]
  int v191; // [rsp+188h] [rbp+88h]
  int v192; // [rsp+18Ch] [rbp+8Ch]
  int v193; // [rsp+190h] [rbp+90h]
  int v194; // [rsp+194h] [rbp+94h]
  int v195; // [rsp+198h] [rbp+98h]
  int v196; // [rsp+19Ch] [rbp+9Ch]
  int v202; // [rsp+1F0h] [rbp+F0h]
  int v206; // [rsp+200h] [rbp+100h]
  int v210; // [rsp+210h] [rbp+110h]
  int v214; // [rsp+220h] [rbp+120h]
  int v218; // [rsp+230h] [rbp+130h]
  int v222; // [rsp+240h] [rbp+140h]
  int v230; // [rsp+284h] [rbp+184h]
  int v234; // [rsp+294h] [rbp+194h]
  unsigned __int64 v235; // [rsp+2A0h] [rbp+1A0h]
  char v236; // [rsp+350h] [rbp+250h] BYREF
  _UNKNOWN *retaddr; // [rsp+388h] [rbp+288h] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
    vmovaps xmmword ptr [rax-68h], xmm8
    vmovaps xmmword ptr [rax-78h], xmm9
    vmovaps xmmword ptr [rax-88h], xmm10
    vmovaps xmmword ptr [rax-98h], xmm11
    vmovaps xmmword ptr [rax-0A8h], xmm12
    vmovaps xmmword ptr [rax-0B8h], xmm13
    vmovaps xmmword ptr [rax-0C8h], xmm14
    vmovaps xmmword ptr [rax-0D8h], xmm15
  }
  v235 = (unsigned __int64)&v153 ^ MEMORY[0x1ACA440];
  _RSI = a12;
  v24 = *(_QWORD *)(a12 + 144);
  if ( v24 != 0 )
  {
    __asm
    {
      vmovups ymm0, ymmword ptr [rdx]
      vmovups [rbp+280h+var_300], ymm0
      vmovups ymm1, ymmword ptr [rdx+20h]
      vmovups [rbp+280h+var_2E0], ymm1
    }
    __asm { vzeroupper }
    sub_F10F4(a1, a12, _RDX, &v167);
    v29 = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v24 + 136LL))(v27, v28, 9, v24);
    __asm { vmovss  xmm7, dword ptr cs:17060C4h }
    if ( *(_QWORD *)(v30 + 152) != 0 )
    {
      sub_F1610(v29, v30, &v167, v170);
      v141 = *v140;
      if ( a13 != 0 )
        (*(void (__fastcall **)(__int64, __int64, _BYTE *, __int64 *))(v141 + 40))(v138, v139, v170, v140);
      else
        (*(void (__fastcall **)(__int64, __int64, _BYTE *, __int64 *))(v141 + 104))(v138, v139, v170, v140);
      __asm
      {
        vmovss  xmm8, dword ptr [rbp+280h+var_2E0+18h]
        vmovss  xmm15, dword ptr [rbp+280h+var_2E0+14h]
      }
    }
    else
    {
      v171 = 20175656;
      v172 = 9;
      sub_1B0884(v29, v30, 12, (unsigned int)&v174, 2, (unsigned int)sub_1A34E0, v153, v154, v155);
      sub_1B0884(v32, v33, 12, (unsigned int)&v178, 2, (unsigned int)sub_1A34E0, v153, v154, v155);
      sub_1B0884(v34, v35, 12, (unsigned int)&v183, 2, (unsigned int)sub_1A34E0, v153, v154, v155);
      v41 = 0;
      v42 = &v179;
      do
      {
        v173[v41] = 0;
        *(_QWORD *)(v42 - 4) = 0;
        *((_DWORD *)v42 + 1) = 1065353216;
        *(_QWORD *)(v42 - 28) = 1065353216;
        *((_DWORD *)v42 - 5) = 0;
        *((_QWORD *)v42 + 3) = 0;
        *((_DWORD *)v42 + 5) = 0;
        ++v41;
        v42 += 12;
      }
      while ( v41 < 2 );
      v171 = 20175632;
      __asm
      {
        vxorps  xmm0, xmm0, xmm0
        vmovups [rbp+280h+var_1E0], xmm0
        vxorps  xmm1, xmm1, xmm1
        vmovups [rbp+280h+var_1D0], xmm1
        vmovups [rbp+280h+var_1C0], xmm0
        vmovups [rbp+280h+var_1B0], xmm1
        vmovups [rbp+280h+var_1A0], xmm0
        vxorps  xmm6, xmm6, xmm6
        vmovss  [rbp+280h+var_18C], xmm6
        vmovss  [rbp+280h+var_188], xmm6
        vmovss  xmm0, dword ptr cs:17061BCh
        vmovss  [rbp+280h+var_184], xmm0
      }
      v202 = 0;
      __asm
      {
        vmovss  [rbp+280h+var_17C], xmm6
        vmovss  [rbp+280h+var_178], xmm6
        vmovss  [rbp+280h+var_174], xmm0
      }
      v206 = 0;
      __asm
      {
        vmovss  [rbp+280h+var_16C], xmm6
        vmovss  [rbp+280h+var_168], xmm6
        vmovss  [rbp+280h+var_164], xmm0
      }
      v210 = 0;
      __asm
      {
        vmovss  [rbp+280h+var_15C], xmm6
        vmovss  [rbp+280h+var_158], xmm6
        vmovss  [rbp+280h+var_154], xmm0
      }
      v214 = 0;
      __asm
      {
        vmovss  [rbp+280h+var_14C], xmm6
        vmovss  [rbp+280h+var_148], xmm6
        vmovss  [rbp+280h+var_144], xmm0
      }
      v218 = 0;
      __asm
      {
        vmovss  [rbp+280h+var_13C], xmm6
        vmovss  [rbp+280h+var_138], xmm6
        vmovss  [rbp+280h+var_134], xmm0
      }
      v222 = 0;
      v47 = 0;
      v48 = &v179;
      do
      {
        v173[v47] = 0;
        *(_QWORD *)(v48 - 4) = 0;
        *((_DWORD *)v48 + 1) = 1065353216;
        *(_QWORD *)(v48 - 28) = 1065353216;
        *((_DWORD *)v48 - 5) = 0;
        *((_QWORD *)v48 + 3) = 0;
        *((_DWORD *)v48 + 5) = 0;
        ++v47;
        v48 += 12;
      }
      while ( v47 < 2 );
      __asm
      {
        vmovss  [rbp+280h+var_218], xmm0
        vmovss  [rbp+280h+var_214], xmm0
        vpxor   xmm0, xmm0, xmm0
        vmovdqu [rbp+280h+var_210], xmm0
      }
      v190 = 2;
      v191 = 2;
      v192 = 2;
      v193 = 2;
      v196 = 2;
      v194 = 2;
      v195 = 2;
      __asm
      {
        vmovups [rbp+280h+var_130], xmm0
        vmovdqu xmm1, xmmword ptr cs:17061F0h
        vmovups [rbp+280h+var_120], xmm1
        vmovups [rbp+280h+var_110], xmm0
        vmovss  [rbp+280h+var_100], xmm6
      }
      v230 = 0;
      __asm
      {
        vmovss  xmm0, dword ptr cs:17063C0h
        vmovss  [rbp+280h+var_F8], xmm0
        vmovss  xmm1, dword ptr cs:17064E8h
        vmovss  [rbp+280h+var_F4], xmm1
      }
      v234 = 0;
      __asm
      {
        vmovss  xmm8, dword ptr cs:17061B8h
        vmovss  [rbp+280h+var_F0], xmm8
      }
      v54 = (*(double (__fastcall **)(__int64, __int64, __int64 *, __int64))(*(_QWORD *)v36 + 176LL))(
              v36,
              v37,
              &v171,
              v36);
      __asm
      {
        vmovss  xmm9, dword ptr [rbp+280h+var_300]
        vmovss  [rsp+380h+var_328], xmm9
        vmovss  xmm10, dword ptr [rbp+280h+var_300+4]
        vmovss  [rsp+380h+var_324], xmm10
        vmovss  xmm11, dword ptr [rbp+280h+var_300+8]
        vmovss  [rsp+380h+var_320], xmm11
        vmovss  xmm12, dword ptr [rbp+280h+var_300+10h]
        vmovss  dword ptr [rsp+380h+var_350], xmm12
        vmovss  xmm13, dword ptr [rbp+280h+var_300+14h]
        vmovss  dword ptr [rsp+380h+var_350+4], xmm13
        vmovss  xmm14, dword ptr [rbp+280h+var_300+18h]
        vmovss  [rsp+380h+var_348], xmm14
        vmovss  xmm15, dword ptr cs:17060ACh
        vmovaps xmm1, xmm15
      }
      if ( (unsigned int)sub_2C4B5C(v64, v65, v63, v165, v54, *(double *)&_XMM1) != 0 )
        goto LABEL_31;
      __asm { vmovaps xmm1, xmm15 }
      if ( (unsigned int)sub_2C4B5C(v67, v68, v66, &v157, v69, *(double *)&_XMM1) != 0 )
      {
LABEL_31:
        __asm
        {
          vmovaps xmm9, xmm8
          vmovaps xmm10, xmm6
          vmovaps xmm11, xmm6
          vmovaps xmm12, xmm6
          vmovaps xmm13, xmm8
          vmovaps xmm14, xmm6
        }
      }
      __asm
      {
        vmulss  xmm0, xmm7, dword ptr [rbp+280h+var_2E0+10h]
        vmovss  [rbp+280h+var_224], xmm0
        vmovss  xmm15, dword ptr [rbp+280h+var_2E0+14h]
        vmulss  xmm1, xmm15, xmm7
        vmovss  [rbp+280h+var_220], xmm1
        vmovss  xmm8, dword ptr [rbp+280h+var_2E0+18h]
        vmulss  xmm0, xmm8, xmm7
        vmovss  [rbp+280h+var_21C], xmm0
        vmovss  [rbp+280h+var_23C], xmm9
        vmovss  [rbp+280h+var_238], xmm10
        vmovss  [rbp+280h+var_234], xmm11
        vmovss  [rbp+280h+var_254], xmm12
        vmovss  [rbp+280h+var_250], xmm13
        vmovss  [rbp+280h+var_24C], xmm14
      }
      (*(void (__fastcall **)(__int64, __int64, __int64 *, __int64))(*(_QWORD *)v67 + 168LL))(v67, v68, &v171, v67);
    }
    __asm { vmovss  xmm6, dword ptr cs:1706220h }
    if ( MEMORY[0x1DA8118] != 0
      && ((*(void (__fastcall **)(__int64, __int64, __int64 *, __int64, _DWORD *, double, double, double, double, double, double, double))(*(_QWORD *)v24 + 8LL))(
            a1,
            _RSI,
            &v157,
            v24,
            v165,
            v78,
            v79,
            v80,
            v81,
            v82,
            v83,
            *(double *)&_XMM6),
          v157 != 0) )
    {
      __asm
      {
        vmovdqu xmm0, xmmword ptr cs:17E17E0h
        vmovups [rbp+280h+var_2C0], xmm0
      }
      if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)v157 + 728LL))(a1, _RSI, 128) != 0 )
      {
        __asm
        {
          vmovss  xmm0, dword ptr cs:17062FCh
          vmovss  dword ptr [rbp+280h+var_2C0+4], xmm0
          vmovss  dword ptr [rbp+280h+var_2C0+8], xmm0
        }
      }
      v145 = *(_QWORD *)(MEMORY[0x1C1F890] + 440LL);
      v146 = *(void (__fastcall **)(__int64, __int64, int *, __int64, int *, __int128 *))(*(_QWORD *)(v145 + 520) + 32LL);
      (*(void (__fastcall **)(__int64, __int64, _QWORD *))(*(_QWORD *)v157 + 80LL))(v145, v143, v166);
      __asm
      {
        vmulss  xmm4, xmm6, dword ptr [rax+8]
        vmulss  xmm3, xmm6, dword ptr [rax+4]
        vmulss  xmm2, xmm6, dword ptr [rax]
        vmovss  [rsp+380h+var_340], xmm2
        vmovss  [rsp+380h+var_33C], xmm3
        vmovss  [rsp+380h+var_338], xmm4
        vmovss  xmm9, dword ptr [rbp+280h+var_2E0+10h]
        vmovss  [rsp+380h+var_334], xmm9
        vmovss  [rsp+380h+var_330], xmm15
        vmovss  [rsp+380h+var_32C], xmm8
      }
      v156 = 2;
      v146(v151, v152, &v162, v151 + 520, &v159, &v169);
    }
    else
    {
      __asm { vmovss  xmm9, dword ptr [rbp+280h+var_2E0+10h] }
    }
    if ( (*(_BYTE *)(_RSI + 160) & 0xC) != 0 )
    {
      (*(void (__fastcall **)(__int64, __int64, __int64 *, __int64, _DWORD *, double, double, double, double, double, double, double))(*(_QWORD *)v24 + 8LL))(
        a1,
        _RSI,
        &v157,
        v24,
        v165,
        v78,
        v79,
        v80,
        v81,
        v82,
        v83,
        *(double *)&_XMM6);
      (*(void (__fastcall **)(__int64, __int64, _QWORD *))(*(_QWORD *)v157 + 80LL))(v86, v87, v166);
      __asm
      {
        vmulss  xmm1, xmm6, dword ptr [rax]
        vsubss  xmm9, xmm9, xmm1
        vmulss  xmm1, xmm6, dword ptr [rax+4]
        vsubss  xmm10, xmm15, xmm1
        vmulss  xmm1, xmm6, dword ptr [rax+8]
        vsubss  xmm6, xmm8, xmm1
        vmulss  xmm2, xmm10, xmm10
        vmulss  xmm0, xmm9, xmm9
        vaddss  xmm3, xmm2, xmm0
        vmulss  xmm1, xmm6, xmm6
        vaddss  xmm2, xmm3, xmm1
        vsqrtss xmm0, xmm2, xmm2
        vcomiss xmm0, dword ptr [rsi+0B4h]
      }
      if ( !(v101 | v102) )
      {
        v115 = *(_DWORD *)(_RSI + 160);
        if ( (v115 & 4) != 0 )
        {
          __asm
          {
            vmovups ymm0, ymmword ptr cs:1AF8A70h
            vmovups [rbp+280h+var_300], ymm0
            vmovups ymm1, ymmword ptr cs:1AF8A90h
            vmovups [rbp+280h+var_2E0], ymm1
          }
          __asm { vzeroupper }
          sub_214A8C(a1, _RSI, 0, _RSI, 0, &v167);
        }
        else if ( (v115 & 8) != 0 )
        {
          v116 = sub_C99F0(a1, _RSI, _RDX, *(_QWORD *)(_RSI + 96));
          if ( v116 != 0 )
          {
            v117 = *(_QWORD *)(v116 + 592);
            if ( v117 != 0 )
            {
              v118 = 0;
              if ( *(int *)(v117 + 116) > 0 )
              {
                v119 = 0;
                do
                {
                  v120 = *(_QWORD *)(*(_QWORD *)(v117 + 108) + v119);
                  a1 = *(_QWORD *)(v120 + 136);
                  if ( a1 != 0 )
                  {
                    (*(void (__fastcall **)(__int64, __int64, __int128 *, _QWORD))(*(_QWORD *)a1 + 80LL))(
                      a1,
                      _RSI,
                      &v169,
                      *(_QWORD *)(v120 + 136));
                    __asm
                    {
                      vmulss  xmm0, xmm9, xmm7
                      vaddss  xmm1, xmm0, dword ptr [rax]
                      vmovss  [rsp+380h+var_334], xmm1
                      vmulss  xmm2, xmm10, xmm7
                      vaddss  xmm0, xmm2, dword ptr [rax+4]
                      vmovss  [rsp+380h+var_330], xmm0
                      vmulss  xmm1, xmm6, xmm7
                      vaddss  xmm2, xmm1, dword ptr [rax+8]
                      vmovss  [rsp+380h+var_32C], xmm2
                    }
                    (*(void (__fastcall **)(__int64, __int64, int *, __int64))(*(_QWORD *)v128 + 48LL))(
                      v128,
                      v129,
                      &v162,
                      v128);
                    _RCX = *(_QWORD *)(*(_QWORD *)(_RSI + 96) + 120LL);
                    if ( _RCX != 0 )
                    {
                      __asm
                      {
                        vmovsd  xmm0, qword ptr [rcx+148h]
                        vmovsd  [rsp+380h+var_318], xmm0
                        vmulss  xmm1, xmm7, dword ptr [rsp+380h+var_318]
                        vmovss  [rsp+380h+var_340], xmm1
                        vmulss  xmm0, xmm7, dword ptr [rsp+380h+var_318+4]
                        vmovss  [rsp+380h+var_33C], xmm0
                        vmulss  xmm2, xmm7, dword ptr [rcx+150h]
                        vmovss  [rsp+380h+var_338], xmm2
                      }
                      (*(void (__fastcall **)(__int64, __int64, int *, __int64))(*(_QWORD *)a1 + 448LL))(
                        a1,
                        _RSI,
                        &v159,
                        a1);
                    }
                  }
                  ++v118;
                  v119 += 8;
                }
                while ( v118 < *(_DWORD *)(v117 + 116) );
              }
            }
          }
        }
      }
    }
  }
  result = sub_53D9B0(a1, _RSI, _RDX, (unsigned __int64)&v153 ^ v235);
  _R11 = &v236;
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
