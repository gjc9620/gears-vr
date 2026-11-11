_UNKNOWN **__fastcall sub_CED150(
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
        _QWORD *_RCX)
{
  _UNKNOWN **result; // rax
  __int64 v13; // r8
  __int64 v27; // rdx
  __int64 v28; // rdi
  __int64 v29; // rsi
  __int64 v30; // r8
  double v31; // xmm0_8
  __int64 v34; // rdi
  __int64 v35; // rsi
  int v36; // r9d
  __int64 v47; // rax
  int v52; // r15d
  int v53; // r12d
  int v54; // ebx
  __int64 *v55; // rbx
  __int64 v56; // rdi
  int v57; // esi
  int v58; // r9d
  __int64 *v59; // r14
  __int64 v60; // rdi
  int v61; // r9d
  int v62; // esi
  __int64 v63; // rdi
  _QWORD *v64; // rsi
  __int64 v66; // rcx
  int v67; // r8d
  __int64 v68; // rdx
  __int64 v70; // rdx
  __int64 v71; // rdi
  __int64 *v72; // rsi
  int v73; // r9d
  int v75; // esi
  __int64 *v76; // r14
  bool v77; // zf
  int v78; // r12d
  __int64 v79; // rdi
  int v80; // esi
  int v81; // r9d
  __int64 *v82; // r15
  __int64 v83; // rdi
  int v84; // r9d
  int v85; // esi
  __int64 v88; // rdi
  int v89; // esi
  int v90; // r8d
  __int64 *v91; // rsi
  int v92; // r8d
  __int64 v98; // rdx
  __int64 v99; // rsi
  __int64 i; // rdx
  __int64 v101; // rdi
  __int64 v102; // rsi
  __int64 v103; // r8
  __int64 v105; // r10
  int v107; // eax
  __int64 v113; // rcx
  __int64 v126; // rdx
  __int64 v127; // rbx
  __int64 v128; // rdi
  __int64 v129; // rsi
  int v130; // ecx
  __int64 v133; // rdi
  __int64 v134; // rsi
  __int64 v135; // rbx
  __int64 v136; // rdi
  __int64 v137; // rsi
  int v138; // eax
  _DWORD *v139; // rsi
  __int64 v140; // rdi
  int v141; // edx
  int v142; // r8d
  int v143; // r9d
  int v144; // edi
  int v145; // esi
  int v146; // r8d
  int v147; // r9d
  __int64 v148; // rdi
  int v149; // esi
  int v150; // r8d
  int v151; // r9d
  __int64 v152; // rdx
  __int64 v153; // rdi
  __int64 v154; // rsi
  int v155; // edx
  int v156; // edi
  int v157; // esi
  int v158; // r8d
  int v159; // r9d
  __int64 v163; // [rsp+0h] [rbp-100h]
  int v164; // [rsp+8h] [rbp-F8h]
  _QWORD v165[6]; // [rsp+20h] [rbp-E0h] BYREF
  _UNKNOWN **v166; // [rsp+50h] [rbp-B0h] BYREF
  int v168; // [rsp+78h] [rbp-88h]
  int v182; // [rsp+BCh] [rbp-44h]
  __int64 v188; // [rsp+E0h] [rbp-20h]
  __int64 v189; // [rsp+E8h] [rbp-18h]
  int v190; // [rsp+F0h] [rbp-10h]
  int v191; // [rsp+F4h] [rbp-Ch]
  int v192; // [rsp+F8h] [rbp-8h]
  __int64 v193; // [rsp+100h] [rbp+0h]
  __int64 v194; // [rsp+108h] [rbp+8h]
  int v195; // [rsp+110h] [rbp+10h]
  int v196; // [rsp+114h] [rbp+14h]
  __int64 v197; // [rsp+118h] [rbp+18h]
  __int64 v198; // [rsp+120h] [rbp+20h]
  __int64 v199; // [rsp+128h] [rbp+28h]
  __int64 v200; // [rsp+130h] [rbp+30h]
  __int64 v201; // [rsp+138h] [rbp+38h]
  int v202; // [rsp+140h] [rbp+40h]
  __int64 v203; // [rsp+148h] [rbp+48h]
  __int16 v204; // [rsp+150h] [rbp+50h]
  int v213; // [rsp+180h] [rbp+80h]
  __int64 v214; // [rsp+188h] [rbp+88h]
  __int128 v215; // [rsp+190h] [rbp+90h]
  __m256 v216; // [rsp+1A0h] [rbp+A0h] BYREF
  char v218; // [rsp+200h] [rbp+100h] BYREF
  _UNKNOWN *retaddr; // [rsp+228h] [rbp+128h] BYREF

  result = &retaddr;
  v13 = _RCX[67];
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
  }
  if ( v13 != 0 && a11 != 0 && *(_QWORD *)((char *)_RCX + 892) == 0 )
  {
    __asm { vmovss  xmm0, dword ptr [rcx+1E8h] }
    _RAX = _RCX[15];
    __asm
    {
      vmulss  xmm6, xmm0, dword ptr [rcx+1ECh]
      vmulss  xmm1, xmm0, dword ptr [rcx+1F0h]
      vmulss  xmm3, xmm0, dword ptr [rcx+1F4h]
      vmovss  dword ptr [rsp+220h+var_200], xmm6
      vmovss  dword ptr [rsp+220h+var_200+4], xmm1
      vmovss  dword ptr [rsp+220h+var_200+8], xmm3
    }
    if ( _RAX != 0 )
    {
      __asm
      {
        vmovss  xmm2, dword ptr [rax+198h]
        vmulss  xmm0, xmm2, dword ptr [rax+19Ch]
        vmulss  xmm6, xmm0, xmm6
        vmulss  xmm0, xmm2, dword ptr [rax+1A0h]
        vmulss  xmm1, xmm0, xmm1
        vmulss  xmm0, xmm2, dword ptr [rax+1A4h]
        vmovss  dword ptr [rsp+220h+var_200+4], xmm1
        vmulss  xmm1, xmm0, xmm3
        vmovss  dword ptr [rsp+220h+var_200+8], xmm1
        vmovss  dword ptr [rsp+220h+var_200], xmm6
      }
    }
    result = (_UNKNOWN **)sub_2CF600(_RCX, 0, a11, v165);
    if ( (_DWORD)result != 0 )
    {
      __asm { vmovaps xmm1, xmm6 }
      result = (_UNKNOWN **)sub_CF6E74(v28, v29, v27, v30, v31, *(double *)&_XMM1);
      if ( result != nullptr )
      {
        __asm
        {
          vmovss  xmm2, dword ptr cs:17061B8h
          vmovss  xmm7, dword ptr cs:17061C0h
          vmovdqu xmm0, xmmword ptr cs:17061F0h
          vmovdqu xmm1, xmmword ptr cs:17E16E0h
          vmovups xmmword ptr [rsp+220h+var_1C8+8], xmm0
          vmovss  xmm0, dword ptr cs:17061E4h
          vmovss  [rbp+120h+var_198], xmm0
          vmovss  xmm0, dword ptr cs:17063BCh
          vmovss  [rbp+120h+var_17C], xmm0
          vmovss  [rbp+120h+var_170], xmm0
          vmovss  xmm0, dword ptr cs:17064ECh
          vmovss  [rbp+120h+var_148], xmm0
          vmovups [rbp+120h+var_190], xmm1
          vmovss  xmm1, dword ptr cs:170612Ch
          vxorps  xmm3, xmm3, xmm3
        }
        v166 = result;
        v47 = *(_QWORD *)(v34 + 536);
        __asm
        {
          vmovss  [rbp+120h+var_174], xmm1
          vmovss  [rbp+120h+var_178], xmm1
          vmovss  xmm1, dword ptr cs:1706190h
          vxorps  xmm0, xmm0, xmm0
          vmovups [rbp+120h+var_160], xmm0
          vmovss  xmm0, dword ptr cs:17061BCh
          vmovss  [rbp+120h+var_BC], xmm0
          vmovss  [rbp+120h+var_B8], xmm0
          vmovss  [rbp+120h+var_B4], xmm0
          vmovss  dword ptr [rsp+220h+var_1C8], xmm2
          vmovss  dword ptr [rsp+220h+var_1C8+4], xmm3
          vmovss  dword ptr [rsp+220h+var_1C8+18h], xmm3
          vmovss  dword ptr [rsp+220h+var_1C8+1Ch], xmm3
          vmovss  [rsp+220h+var_1A8], xmm2
          vmovss  [rbp+120h+var_180], xmm2
          vmovss  [rbp+120h+var_16C], xmm2
          vmovss  [rbp+120h+var_168], xmm2
          vmovss  [rbp+120h+var_144], xmm1
          vmovss  [rbp+120h+var_150], xmm3
          vmovss  [rbp+120h+var_14C], xmm3
          vmovss  [rbp+120h+var_B0], xmm7
          vmovss  [rbp+120h+var_AC], xmm7
          vmovss  [rbp+120h+var_A8], xmm7
        }
        v213 = 32;
        v182 = 5;
        v204 = v35;
        v198 = v35;
        v199 = v35;
        v200 = v35;
        v202 = v35;
        v203 = v35;
        v214 = v35;
        __asm
        {
          vpxor   xmm0, xmm0, xmm0
          vmovdqu [rbp+120h+var_90], xmm0
        }
        v52 = *(_DWORD *)(v47 + 408);
        v53 = *(_DWORD *)(v47 + 468);
        if ( (*(_BYTE *)(v47 + 584) & 1) != 0 && *(_DWORD *)(v47 + 424) == (_DWORD)v35 )
        {
          v52 += *(_DWORD *)(v47 + 592);
          v54 = v34 + 960;
          *(_DWORD *)(v34 + 968) = v35;
          if ( *(_DWORD *)(v34 + 972) != (_DWORD)v35 )
          {
            *(_DWORD *)(v34 + 972) = v35;
            sub_DC950(v34, v35, 4, v34 + 960, 8, v36);
          }
          sub_1BEC94(v34, v35, v53, v54, 4, 8);
        }
        v55 = (__int64 *)(v34 + 904);
        *(_DWORD *)(v34 + 912) = v35;
        if ( *(_DWORD *)(v34 + 916) != (_DWORD)v35 )
        {
          *(_DWORD *)(v34 + 916) = v35;
          sub_DC950(v34, v35, 12, v34 + 904, 8, v36);
        }
        sub_1BEC94(v34, v35, v52, (_DWORD)v55, 12, 8);
        v59 = (__int64 *)(v56 + 920);
        *(_DWORD *)(v56 + 928) = v57;
        if ( *(_DWORD *)(v56 + 932) != v57 )
        {
          *(_DWORD *)(v56 + 932) = v57;
          sub_DC950(v56, v57, 12, v56 + 920, 8, v58);
        }
        sub_1BEC94(v56, v57, v52, (_DWORD)v59, 12, 8);
        v62 = v60 + 936;
        *(_DWORD *)(v60 + 944) = 0;
        if ( *(_DWORD *)(v60 + 948) != 0 )
        {
          *(_DWORD *)(v60 + 948) = 0;
          sub_DC950(v60, v62, 4, v60 + 936, 8, v61);
        }
        sub_1BEC94(v60, v62, v53, v62, 4, 8);
        v66 = *(_QWORD *)(v63 + 536);
        v67 = 0;
        if ( *(int *)(v66 + 468) > 0 )
        {
          v68 = 0;
          do
          {
            ++v67;
            *(_DWORD *)(v68 + *v64) = *(_DWORD *)(v68 + *(_QWORD *)(v66 + 460));
            v68 += 4;
            v66 = *(_QWORD *)(v63 + 536);
          }
          while ( v67 < *(_DWORD *)(v66 + 468) );
        }
        __asm { vmovaps xmm2, xmm6 }
        sub_CF3BF4(v63, (_DWORD)v64, (_DWORD)v55, v66, v67, 1);
        _R10 = *(_QWORD *)(v71 + 536);
        if ( *(_DWORD *)(_R10 + 424) != 0 )
        {
          v75 = *(_DWORD *)(_R10 + 432);
          v76 = (__int64 *)(v71 + 980);
          v77 = *(_DWORD *)(v71 + 992) == 0;
          v78 = *(_DWORD *)(_R10 + 444);
          *(_DWORD *)(v71 + 988) = 0;
          if ( !v77 )
          {
            *(_DWORD *)(v71 + 992) = 0;
            sub_DC950(v71, v75, 12, v71 + 980, 8, v73);
          }
          sub_1BEC94(v71, v75, v75, (_DWORD)v76, 12, 8);
          v82 = (__int64 *)(v79 + 996);
          *(_DWORD *)(v79 + 1004) = 0;
          if ( *(_DWORD *)(v79 + 1008) != 0 )
          {
            *(_DWORD *)(v79 + 1008) = 0;
            sub_DC950(v79, v80, 12, v79 + 996, 8, v81);
          }
          sub_1BEC94(v79, v80, v80, (_DWORD)v82, 12, 8);
          v85 = v83 + 1012;
          *(_DWORD *)(v83 + 1020) = 0;
          if ( *(_DWORD *)(v83 + 1024) != 0 )
          {
            *(_DWORD *)(v83 + 1024) = 0;
            sub_DC950(v83, v85, 4, v83 + 1012, 8, v84);
          }
          sub_1BEC94(v83, v85, v78, v85, 4, 8);
          __asm { vmovaps xmm2, xmm6 }
          sub_CF3BF4(v88, v89, (_DWORD)v55, *(_QWORD *)(v88 + 536), v90, 0);
          _R10 = *(_QWORD *)(v71 + 536);
          v92 = 0;
          if ( *(int *)(_R10 + 444) > 0 )
          {
            v70 = 0;
            do
            {
              ++v92;
              *(_DWORD *)(v70 + *v91) = *(_DWORD *)(v70 + *(_QWORD *)(_R10 + 436));
              v70 += 4;
              _R10 = *(_QWORD *)(v71 + 536);
            }
            while ( v92 < *(_DWORD *)(_R10 + 444) );
          }
          v188 = *v76;
          v189 = *v82;
          v194 = *v91;
          v192 = *(_DWORD *)(v71 + 988);
          v196 = *(_DWORD *)(v71 + 1020);
          v190 = 12;
          v191 = 12;
          v195 = 4;
        }
        else
        {
          v188 = *v55;
          v189 = *v59;
          v194 = *v72;
          v190 = 12;
          v191 = 12;
          v195 = 4;
          v192 = v52;
          v196 = v53;
          if ( (*(_BYTE *)(_R10 + 584) & 1) != 0 )
          {
            v198 = *(_QWORD *)(v71 + 960);
            v200 = v71 + 976;
            LODWORD(v199) = 4;
            HIDWORD(v199) = v53;
          }
        }
        LOBYTE(v70) = *(_BYTE *)(v71 + 1032);
        v197 = v71 + 956;
        v193 = v71 + 952;
        v201 = v71 + 1028;
        __asm { vmovss  xmm0, dword ptr [r10+1F8h] }
        __asm { vmovss  [rbp+120h+var_194], xmm0 }
        sub_10A78C(v71, v71 + 952, v70, v165, v71 + 1036);
        __asm
        {
          vmovups xmm0, [rsp+220h+var_200]
          vmovups ymm1, ymmword ptr [rdi+0A0h]
          vmovdqu [rbp+120h+var_CC], xmm0
          vmovups ymm0, ymmword ptr [rdi+0C0h]
          vmovups [rbp+120h+var_60], ymm0
          vmovups [rbp+120h+var_80], ymm1
        }
        __asm { vzeroupper }
        sub_F10F4(_RDI, v99, v98, &v216);
        if ( (*(_BYTE *)(v105 + 568) & 2) != 0 )
        {
          __asm { vmovdqu xmm0, xmmword ptr cs:17E1880h }
          v107 = *(_DWORD *)(v101 + 912);
          __asm
          {
            vmovups [rsp+220h+var_200], xmm0
            vmovss  xmm0, dword ptr [rsp+220h+var_200+0Ch]
            vmovss  xmm4, dword ptr [rsp+220h+var_200+8]
            vmovss  xmm1, dword ptr [rsp+220h+var_200+4]
            vmovss  xmm2, dword ptr [rsp+220h+var_200]
            vmovaps xmm5, xmm7
          }
          if ( v107 > 0 )
          {
            v113 = *v55;
            for ( i = (unsigned int)v107; i != 0; --i )
            {
              __asm
              {
                vmaxss  xmm0, xmm0, dword ptr [rcx]
                vmaxss  xmm7, xmm7, dword ptr [rcx+4]
                vmaxss  xmm5, xmm5, dword ptr [rcx+8]
                vminss  xmm2, xmm2, dword ptr [rcx]
                vminss  xmm1, xmm1, dword ptr [rcx+4]
                vminss  xmm4, xmm4, dword ptr [rcx+8]
              }
              v113 += 12;
            }
          }
          __asm
          {
            vmovss  xmm3, dword ptr cs:1706680h
            vaddss  xmm0, xmm0, xmm2
            vmulss  xmm0, xmm0, xmm3
            vaddss  xmm1, xmm7, xmm1
            vmulss  xmm2, xmm1, xmm3
            vmovss  [rsp+220h+var_1A4], xmm0
            vaddss  xmm0, xmm5, xmm4
            vmulss  xmm1, xmm0, xmm3
            vmovss  [rbp+120h+var_19C], xmm1
            vmovss  [rbp+120h+var_1A0], xmm2
          }
        }
        else
        {
          _RAX = sub_F1610(v101, v102, &v216, v165);
          __asm
          {
            vmovups ymm0, ymmword ptr [rax]
            vmovups [rsp+220h+var_1C8], ymm0
          }
          v168 = *(_DWORD *)(_RAX + 32);
          __asm
          {
            vmovss  xmm0, dword ptr [rax+24h]
            vmovss  [rsp+220h+var_1A4], xmm0
            vmovss  xmm1, dword ptr [rax+28h]
            vmovss  [rbp+120h+var_1A0], xmm1
            vmovss  xmm0, dword ptr [rax+2Ch]
            vmovss  [rbp+120h+var_19C], xmm0
          }
        }
        __asm { vzeroupper }
        v127 = sub_2AB6C8(v101, v102, i, a11, v103);
        if ( (*(_BYTE *)(v128 + 352) & 0x40) != 0
          && (*((_QWORD *)&v215 + 1) = sub_D0FDF4(v128, v129, v126, a11), (unsigned int)sub_467AA0() != 0) )
        {
          v130 = v213 | 0x2000;
          v213 |= 0x2000u;
        }
        else
        {
          v130 = v213;
        }
        _RAX = *(_QWORD *)(v128 + 536);
        __asm
        {
          vmovss  xmm0, dword ptr [rax+20Ch]
          vmovss  [rbp+120h+var_A4], xmm0
        }
        if ( (*(_BYTE *)(_RAX + 584) & 1) != 0 && *(_DWORD *)(_RAX + 424) == 0 )
          v213 = v130 | 0x1000;
        sub_D0FEB4(v128, v129, v126, v127);
        v135 = (*(__int64 (__fastcall **)(__int64, __int64, _UNKNOWN ***, __int64))(*(_QWORD *)v127 + 904LL))(
                 v133,
                 v134,
                 &v166,
                 v127);
        v138 = (*(__int64 (__fastcall **)(__int64, __int64, _QWORD, __int64))(*(_QWORD *)v135 + 424LL))(
                 v136,
                 v137,
                 *(_QWORD *)(*(_QWORD *)v135 + 424LL),
                 v135);
        *v139 = v138;
        *(_QWORD *)(v140 + 892) = v135;
        *(_DWORD *)(v140 + 900) = *(_DWORD *)(a11 + 96);
        sub_CEFE30(v140, (_DWORD)v139, v141, v140, v142, v143);
        sub_CEF000(v144, v145, v144 + 852, v144, v146, v147);
        sub_CEF074(v148, v149, *(_BYTE *)(v148 + 851) & 1, v148, v150, v151);
        sub_CF047C(v153, v154, v152, v153);
        result = (_UNKNOWN **)sub_CECC10(v156, v157, v155, v156, v158, v159, v163, v164);
      }
    }
  }
  _R11 = &v218;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
  }
  return result;
}
