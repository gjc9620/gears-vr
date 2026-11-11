_UNKNOWN **__fastcall sub_2D10E4(
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
        __int64 a11,
        _QWORD *_RCX,
        int a13,
        int a14)
{
  _UNKNOWN **result; // rax
  __int64 v16; // rsi
  __int64 v29; // rax
  int v30; // edi
  int v31; // edi
  int v32; // esi
  int v33; // edx
  int v34; // edi
  int v35; // esi
  int v36; // r8d
  int v37; // r9d
  __int64 v40; // rdi
  __int64 v41; // rsi
  __int64 v42; // rdi
  __int64 v43; // rsi
  int v44; // edi
  int v45; // esi
  int v46; // r8d
  int v47; // r9d
  __int64 v53; // rdx
  __int64 v54; // rdx
  __int64 v55; // rdi
  __int64 v56; // rsi
  __int64 v57; // rdx
  __int64 v58; // rdi
  __int64 v59; // rsi
  _QWORD *v60; // r8
  int v61; // r9d
  int v64; // r10d
  __int64 v66; // rax
  int v67; // r12d
  unsigned int v68; // r13d
  __int64 v69; // rcx
  __int64 v70; // rsi
  __int64 v71; // rax
  __int64 v72; // rax
  _QWORD *v73; // rdx
  unsigned int v74; // eax
  __int64 v75; // rcx
  __int64 v76; // r15
  _QWORD *v77; // rax
  _QWORD *v78; // r14
  _QWORD *v79; // rdx
  __int64 v86; // rax
  __int64 v87; // rdx
  __int64 v88; // rdi
  __int64 v89; // rsi
  __int64 v90; // r8
  __int64 v91; // rax
  __int64 v92; // rdi
  __int64 v93; // rsi
  __int64 v94; // rax
  __int64 v95; // rdx
  __int64 v96; // rdi
  __int64 v97; // rsi
  int v101; // eax
  int v102; // edx
  int v103; // edi
  int v104; // esi
  int v105; // r8d
  int v106; // r9d
  __int64 v108; // [rsp+0h] [rbp-100h]
  int v109; // [rsp+0h] [rbp-100h]
  __int64 v110; // [rsp+0h] [rbp-100h]
  __int64 v111; // [rsp+8h] [rbp-F8h]
  __int64 v112; // [rsp+8h] [rbp-F8h]
  __int64 v113; // [rsp+10h] [rbp-F0h]
  __int64 v114; // [rsp+10h] [rbp-F0h]
  int v115; // [rsp+30h] [rbp-D0h] BYREF
  __int128 v118; // [rsp+40h] [rbp-C0h] BYREF
  __m256 v119; // [rsp+50h] [rbp-B0h] BYREF
  int v120; // [rsp+70h] [rbp-90h]
  __m256 *v124; // [rsp+80h] [rbp-80h]
  __int64 v126; // [rsp+8Ch] [rbp-74h]
  __int64 v127; // [rsp+98h] [rbp-68h]
  __int64 v128; // [rsp+A0h] [rbp-60h]
  __int64 v129; // [rsp+A8h] [rbp-58h]
  int v130; // [rsp+B0h] [rbp-50h]
  _QWORD *v131; // [rsp+B8h] [rbp-48h]
  _QWORD *v132; // [rsp+C0h] [rbp-40h]
  _QWORD *v133; // [rsp+C8h] [rbp-38h]
  __m256 v134; // [rsp+E0h] [rbp-20h] BYREF
  int v154; // [rsp+14Ch] [rbp+4Ch]
  int v157; // [rsp+158h] [rbp+58h]
  __m256 v160; // [rsp+170h] [rbp+70h] BYREF
  int v161; // [rsp+190h] [rbp+90h]
  __m256 v165; // [rsp+1A0h] [rbp+A0h] BYREF
  char v167; // [rsp+200h] [rbp+100h] BYREF
  _UNKNOWN *retaddr; // [rsp+238h] [rbp+138h] BYREF
  int v170; // [rsp+240h] [rbp+140h]
  unsigned int v174; // [rsp+258h] [rbp+158h]
  __int64 v175; // [rsp+258h] [rbp+158h]

  result = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
  }
  _RBX = _RCX;
  if ( _RCX[65] != 0 )
  {
    result = (_UNKNOWN **)MEMORY[0x1C1F890];
    if ( *(_QWORD *)(MEMORY[0x1C1F890] + 468LL) != 0 && *(_QWORD *)((char *)_RCX + 380) == 0 )
    {
      v16 = 0;
      v170 = 0;
      __asm
      {
        vmovss  xmm0, dword ptr [rcx+1E8h]
        vmulss  xmm4, xmm0, dword ptr [rcx+1F4h]
        vmulss  xmm5, xmm0, dword ptr [rcx+1F0h]
        vmulss  xmm1, xmm0, dword ptr [rcx+1ECh]
        vmovss  [rsp+230h+var_200], xmm1
        vmovss  [rsp+230h+var_1FC], xmm5
        vmovss  [rsp+230h+var_1F8], xmm4
      }
      _RCX = _RCX[15];
      if ( _RCX == 0 )
        goto LABEL_66;
      __asm
      {
        vmovss  xmm0, dword ptr [rcx+198h]
        vmulss  xmm3, xmm0, dword ptr [rcx+1A4h]
        vmulss  xmm2, xmm0, dword ptr [rcx+1A0h]
        vmulss  xmm0, xmm0, dword ptr [rcx+19Ch]
        vmulss  xmm1, xmm0, xmm1
        vmovss  [rsp+230h+var_200], xmm1
        vmulss  xmm2, xmm2, xmm5
        vmovss  [rsp+230h+var_1FC], xmm2
        vmulss  xmm0, xmm3, xmm4
        vmovss  [rsp+230h+var_1F8], xmm0
      }
      v29 = sub_18CE74(a1, 0);
      if ( v29 != 0 )
      {
        if ( (*(_DWORD *)(v29 + 668) & 0x100) != 0 )
          v16 = 1;
        v170 = v16;
      }
      result = (_UNKNOWN **)_RBX[15];
      if ( (*((_BYTE *)result + 172) & 1) == 0 || (_RBX[42] & 0x80000) != 0 || (_DWORD)v16 != 0 )
      {
LABEL_66:
        if ( _RBX[75] != 0 )
          goto LABEL_14;
        v30 = (_DWORD)_RBX + 608;
        if ( *((_DWORD *)_RBX + 154) == 0 || *((_DWORD *)_RBX + 156) != MEMORY[0x164E7AC] )
        {
          sub_307354(v30, v16, a11, (_DWORD)_RBX, a13, a14);
          sub_4C2A20(v103, v104, v102, (_DWORD)_RBX, v105, v106, v108);
        }
        if ( *((int *)_RBX + 154) > 0 )
        {
          sub_2D19A4(v30, v16, (unsigned int)&v118, (_DWORD)_RBX, 0, a14, v108, v111, v113);
          _RBX[75] = sub_2CEA6C(v31, v32, (unsigned int)&v115, (int)_RBX + 528, v31, 0, v109, v112, v114);
          sub_1DDA20(v34, v35, v33, (unsigned int)&v118, v36, v37, v110);
        }
        result = (_UNKNOWN **)sub_2CFFA0(v30, v16, 0, v30, a13, a14, v108, v111);
        if ( _RBX[75] != 0 )
        {
LABEL_14:
          __asm
          {
            vmovups ymm0, ymmword ptr [rbx+0A0h]
            vmovups [rbp+130h+var_90], ymm0
            vmovups ymm1, ymmword ptr [rbx+0C0h]
            vmovups [rbp+130h+var_70], ymm1
          }
          __asm { vzeroupper }
          sub_F10F4(a1, v16, a11, &v165);
          sub_F1610(v40, v41, &v165, &v160);
          sub_576E65(v42, v43, 0, &v119, 136);
          v124 = nullptr;
          v126 = 0;
          v127 = 0;
          v128 = 0;
          v129 = 0;
          v132 = v131;
          v130 = 1;
          __asm
          {
            vmovups ymm0, [rbp+130h+var_C0]
            vmovups [rsp+230h+var_1E0], ymm0
          }
          v120 = v161;
          __asm
          {
            vmovss  xmm0, [rbp+130h+var_9C]
            vmovss  [rsp+230h+var_1BC], xmm0
            vmovss  xmm1, [rbp+130h+var_98]
            vmovss  [rsp+230h+var_1B8], xmm1
            vmovss  xmm0, [rbp+130h+var_94]
            vmovss  [rsp+230h+var_1B4], xmm0
            vmovss  xmm7, dword ptr cs:17061B8h
            vmovss  [rbp+130h+var_1A8], xmm7
          }
          v53 = *(_QWORD *)(MEMORY[0x1C1F350] + 768LL);
          if ( *(_QWORD *)((char *)_RBX + 372) != 0 )
            v53 = *(_QWORD *)((char *)_RBX + 372);
          __asm { vzeroupper }
          v174 = sub_2D184C(v44, v45, v53, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL), v46, v47, v108, v111);
          LOBYTE(v54) = *((_BYTE *)_RBX + 340);
          sub_10A78C(v55, v56, v54, &v118, (char *)_RBX + 348);
          v64 = *((_DWORD *)_RBX + 88);
          if ( (v64 & 2) == 0 || (_RBX[42] & 0x80000) != 0 )
          {
            __asm { vmovups xmm6, [rsp+230h+var_1F0] }
          }
          else
          {
            LOBYTE(v57) = 12;
            _RAX = sub_10A78C(v58, v59, v57, &v118, v60);
            __asm { vmovups xmm6, xmmword ptr [rax] }
          }
          v66 = _RBX[15];
          if ( v66 == 0 || (v67 = 1, (*(_BYTE *)(v66 + 172) & 1) == 0) )
            v67 = 0;
          if ( (v64 & 0x40) != 0 && v67 == 0 )
            v129 = sub_2D1CD8(v58, v59, v57, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL));
          v68 = 0;
          v69 = _RBX[75];
          if ( (unsigned int)((__int64)(*(_QWORD *)(v69 + 112) - *(_QWORD *)(v69 + 104)) >> 3) != 0 )
          {
            v70 = v174;
            do
            {
              v58 = v68;
              v71 = *(_QWORD *)(*(_QWORD *)(v69 + 104) + 8LL * v68);
              if ( (_RBX[44] & 2) != 0 )
                *(_DWORD *)(v71 + 60) |= 0x100u;
              else
                *(_DWORD *)(v71 + 60) &= ~0x100u;
              v72 = *(_QWORD *)(*(_QWORD *)(_RBX[75] + 104LL) + 8LL * v68);
              if ( (_RBX[44] & 4) != 0 )
                *(_DWORD *)(v72 + 60) |= 0x800u;
              else
                *(_DWORD *)(v72 + 60) &= ~0x800u;
              v175 = *(_QWORD *)(_RBX[75] + 104LL);
              v73 = v132;
              if ( v133 <= v132 )
              {
                LODWORD(v60) = (_DWORD)v131;
                v74 = 2 * (v132 - v131) + 2;
                if ( v131 != nullptr )
                  v75 = v133 - v131;
                else
                  LODWORD(v75) = 0;
                if ( (unsigned int)v75 < v74 )
                {
                  v76 = v74;
                  v77 = (_QWORD *)MEMORY[0x131BE18](v68, v70, v132, v76 * 8);
                  v78 = v77;
                  v60 = v132;
                  v79 = v131;
                  if ( v131 != v132 )
                  {
                    do
                      *v77++ = *v79++;
                    while ( v79 != v60 );
                    v79 = v131;
                  }
                  if ( v79 != nullptr )
                  {
                    MEMORY[0x131BE10](v58, v70, v79, v79, v60);
                    v79 = v131;
                  }
                  v133 = &v78[v76];
                  v73 = &v78[(unsigned int)(v132 - v79)];
                  v132 = v73;
                  v131 = v78;
                }
              }
              *v73 = *(_QWORD *)(v175 + 8 * v58);
              ++v132;
              *(_WORD *)(v131[v58] + 66LL) = v70;
              _RCX = v131[v58];
              __asm { vmovdqu xmmword ptr [rcx+70h], xmm6 }
              ++v68;
              v69 = _RBX[75];
            }
            while ( v68 < (unsigned int)((__int64)(*(_QWORD *)(v69 + 112) - *(_QWORD *)(v69 + 104)) >> 3) );
            LODWORD(v59) = v170;
          }
          __asm
          {
            vmovaps ymm0, ymmword ptr cs:1706860h
            vmovups [rbp+130h+var_150], ymm0
            vmovss  [rbp+130h+var_130], xmm7
            vxorps  xmm2, xmm2, xmm2
            vmovss  [rbp+130h+var_124], xmm2
            vmovss  [rbp+130h+var_128], xmm2
            vmovss  [rbp+130h+var_12C], xmm2
            vmovss  [rbp+130h+var_118], xmm2
            vmovss  [rbp+130h+var_11C], xmm2
            vmovss  [rbp+130h+var_120], xmm2
            vmovss  [rbp+130h+var_108], xmm2
            vmovss  [rbp+130h+var_10C], xmm2
            vmovss  [rbp+130h+var_110], xmm2
            vmovss  [rbp+130h+var_FC], xmm2
            vmovss  [rbp+130h+var_100], xmm2
            vmovss  [rbp+130h+var_104], xmm2
            vmovss  xmm0, dword ptr cs:17064ECh
            vmovss  [rbp+130h+var_F8], xmm0
            vmovss  [rbp+130h+var_114], xmm2
            vmovss  [rbp+130h+var_F4], xmm2
            vmovss  xmm0, dword ptr cs:17061C4h
            vmovss  [rbp+130h+var_F0], xmm0
            vmovss  xmm1, dword ptr cs:1706190h
            vmovss  [rbp+130h+var_EC], xmm1
            vmovss  [rbp+130h+var_E0], xmm1
            vmovss  [rbp+130h+var_DC], xmm1
            vmovss  [rbp+130h+var_E8], xmm2
          }
          v157 = 4;
          v154 = 2304;
          __asm
          {
            vmovss  [rbp+130h+var_D4], xmm1
            vmovss  [rbp+130h+var_D0], xmm2
          }
          if ( v67 == 0 )
          {
            v154 = 2432;
            v124 = &v134;
          }
          if ( (_RBX[42] & 0x80000) == 0 && (_DWORD)v59 == 0 )
          {
            v101 = v126;
            if ( (_RBX[44] & 2) == 0 )
              v101 = 1;
            LODWORD(v126) = v101;
          }
          __asm { vzeroupper }
          v86 = sub_2D0F74(v58, v59, 0, v69, (_DWORD)v60, v61);
          *(_QWORD *)((char *)_RBX + 380) = v86;
          *(_QWORD *)(v86 + 136) = 0;
          *(_QWORD *)(*(_QWORD *)((char *)_RBX + 380) + 96LL) = _RBX;
          *(_DWORD *)(*(_QWORD *)((char *)_RBX + 380) + 132LL) = *(_DWORD *)(*(_QWORD *)(MEMORY[0x1C1F890] + 468LL)
                                                                           + 96LL);
          v91 = sub_2AB6C8(v88, v89, v87, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL), v90);
          v94 = (*(__int64 (__fastcall **)(__int64, __int64, __m256 *, __int64))(*(_QWORD *)v91 + 56LL))(
                  v92,
                  v93,
                  &v119,
                  v91);
          if ( v94 != 0 )
          {
            *(_QWORD *)(*(_QWORD *)((char *)_RBX + 380) + 136LL) = v94;
            *(_QWORD *)(v94 + 8) = *(_QWORD *)((char *)_RBX + 380);
          }
          result = (_UNKNOWN **)sub_2CFE88(v96, v97, v95, &v119);
        }
      }
    }
  }
  _R11 = &v167;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
  }
  return result;
}
