__int64 __fastcall sub_1015350(
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
        int a11,
        __int64 a12,
        __int64 a13,
        __int64 a14,
        int a15,
        int a16,
        int a17,
        __int64 a18)
{
  int v21; // r12d
  __int64 v22; // rsi
  __int64 v23; // rax
  __int64 v24; // rdi
  __int64 v25; // rsi
  int v26; // r15d
  __int64 v27; // rdx
  int v28; // edi
  int v29; // esi
  __int64 v30; // rdx
  __int64 v31; // rdi
  __int64 v32; // rsi
  int v33; // edx
  int v34; // edi
  int v35; // esi
  int v36; // r8d
  int v37; // r9d
  __int64 v38; // rdx
  __int64 v39; // rdi
  __int64 v40; // rsi
  __int64 v41; // rsi
  _QWORD *v42; // rbx
  int v43; // edx
  int v44; // r8d
  int v45; // r9d
  __int64 v46; // rsi
  _QWORD *v47; // r13
  int v48; // edi
  int v49; // r8d
  int v50; // r9d
  __int64 v51; // rsi
  _QWORD *v52; // rdi
  __int64 v53; // rbx
  __int64 v54; // rsi
  __int64 v55; // rax
  __int64 v56; // rdx
  __int64 v57; // rdi
  __int64 v58; // rsi
  __int64 v59; // rax
  __int64 v60; // rsi
  __int64 v61; // rax
  __int64 v62; // rdx
  __int64 v63; // rdi
  __int64 v64; // rsi
  __int64 v65; // rax
  __int64 v66; // rsi
  int v67; // edx
  int v70; // edi
  int v71; // r8d
  int v72; // r9d
  __int64 v73; // rax
  __int64 v74; // rsi
  __int64 v75; // rdx
  __int64 v76; // rdi
  __int64 v77; // rax
  __int64 v78; // rdi
  __int64 v79; // rsi
  int v80; // r9d
  __int64 v81; // rax
  __int64 v82; // rdx
  __int64 v83; // rdi
  __int64 v84; // rsi
  __int64 v85; // rax
  __int64 v86; // rdx
  __int64 v87; // rdi
  _QWORD *v88; // rsi
  int v89; // r8d
  __int64 v90; // rax
  __int64 v94; // rax
  __int64 v95; // rdi
  __int64 v96; // rsi
  __int64 v97; // rax
  __int64 v98; // rsi
  __int64 v99; // rcx
  __int64 v101; // rdi
  double v102; // xmm1_8
  double v103; // xmm2_8
  double v104; // xmm3_8
  double v105; // xmm4_8
  double v106; // xmm5_8
  __m128 v107; // xmm6
  __m128 v108; // xmm7
  __int64 v109; // rdx
  __int64 v110; // rdi
  __int64 v111; // rsi
  __int64 v112; // rdx
  __int64 v113; // rdi
  __int64 v114; // rsi
  __int64 v115; // r14
  __int64 v116; // rdi
  __int64 v117; // rsi
  __int64 v118; // rdi
  __int64 v119; // rsi
  __int64 v120; // rdi
  __int64 v121; // rsi
  __int64 v122; // rdi
  __int64 v123; // rsi
  __int64 v124; // rdx
  __int64 v125; // rdi
  __int64 v126; // rsi
  __int64 v127; // rax
  __int64 v128; // rdx
  __int64 v129; // rdi
  __int64 v130; // rsi
  __int64 v131; // rcx
  __int64 v132; // rdi
  __int64 v133; // rsi
  __int64 v134; // rax
  __int64 v135; // rsi
  __int64 v136; // r9
  int v137; // edx
  int v138; // edi
  int v139; // esi
  int v140; // r8d
  int v141; // r9d
  __int64 v142; // rdx
  __int64 v143; // rdi
  __int64 v144; // rsi
  __int64 v145; // rax
  __int64 v146; // rdx
  __int64 v147; // rdi
  __int64 v148; // rsi
  __int64 v149; // rcx
  __int64 v150; // rdi
  __int64 v151; // rsi
  __int64 v152; // rax
  int v153; // edi
  __int64 v154; // rsi
  __int64 v155; // r9
  int v156; // edx
  int v157; // edi
  int v158; // esi
  int v159; // r8d
  int v160; // r9d
  int v161; // edx
  int v162; // edi
  int v163; // esi
  int v164; // r8d
  int v165; // r9d
  __int64 v166; // rdx
  __int64 v167; // rdi
  __int64 v168; // rsi
  __int64 v169; // rax
  int v170; // edx
  int v171; // edi
  __int64 v172; // rsi
  int v173; // r8d
  int v174; // r9d
  __int64 v175; // rax
  __int64 v176; // rdi
  __int64 v177; // rsi
  __int64 v178; // rdi
  __int64 v179; // rsi
  __int64 v180; // rcx
  int v181; // edx
  int v182; // r14d
  __int64 v183; // rbx
  __int64 v187; // rax
  int v188; // edi
  int v189; // esi
  int v190; // r8d
  int v191; // r9d
  __int64 v192; // rdx
  int v193; // edx
  int v194; // edi
  int v195; // esi
  int v196; // r8d
  int v197; // r9d
  __int64 v198; // rdi
  __int64 v199; // rsi
  __int64 v200; // rdi
  __int64 v201; // rsi
  int v202; // edi
  __int64 v203; // rsi
  int v204; // r9d
  __int64 v205; // rdx
  __int64 v206; // rdi
  __int64 v207; // rsi
  int v208; // edx
  int v209; // edi
  __int64 v210; // rsi
  int v211; // r8d
  int v212; // r9d
  __int64 v213; // rsi
  int v214; // r9d
  int v215; // r8d
  int v216; // edx
  __int64 v217; // rdi
  __int64 v218; // rax
  __int64 v219; // rdi
  double v220; // xmm0_8
  __int64 v221; // rax
  int v222; // edi
  int v223; // esi
  int v224; // r9d
  __int64 v225; // r8
  int v226; // edi
  int v227; // esi
  int v228; // r8d
  int v229; // r9d
  int v230; // edx
  int v231; // edx
  int v232; // edi
  int v233; // esi
  int v234; // r8d
  int v235; // r9d
  int v236; // edx
  int v237; // edi
  int v238; // esi
  int v239; // r8d
  int v240; // r9d
  _QWORD *v241; // rax
  int v242; // edx
  int v243; // edi
  int v244; // esi
  int v245; // r8d
  int v246; // r9d
  int v247; // eax
  int v248; // edi
  int v249; // esi
  int v250; // r9d
  int v251; // edx
  int v252; // edi
  int v253; // esi
  int v254; // r8d
  int v255; // r9d
  __int64 v259; // rsi
  __int64 v264; // rdi
  __int64 v265; // rdi
  __int64 v266; // rsi
  __int64 v270; // rsi
  __int64 v277; // rdi
  double v278; // xmm1_8
  double v279; // xmm2_8
  double v280; // xmm3_8
  double v281; // xmm4_8
  double v282; // xmm5_8
  int v283; // edx
  int v284; // edi
  __int64 v285; // rsi
  int v286; // r8d
  int v287; // r9d
  char v300; // cf
  char v301; // zf
  __int64 v305; // rax
  __int64 v309; // rax
  __int64 v310; // rdx
  __int64 v311; // rdi
  __int64 v312; // rsi
  __int64 v313; // rdi
  __int64 v314; // rsi
  __int64 v315; // rdi
  __int64 v316; // rsi
  __int64 v317; // rdi
  __int64 v318; // rsi
  __int64 v319; // rdx
  __int64 v320; // rdi
  __int64 v321; // rsi
  __int64 v322; // rdx
  __int64 v323; // rdi
  __int64 v324; // rsi
  __int64 v326; // rdi
  __int64 v327; // rsi
  __int64 v328; // rdx
  __int64 v329; // rdi
  __int64 v330; // rsi
  int v331; // edx
  int v332; // edi
  int v333; // esi
  int v334; // r8d
  int v335; // r9d
  __int64 v336; // rsi
  __int64 v341; // [rsp+0h] [rbp-D9h]
  __int64 v342; // [rsp+0h] [rbp-D9h]
  int v343; // [rsp+0h] [rbp-D9h]
  __int64 v344; // [rsp+0h] [rbp-D9h]
  char v345; // [rsp+0h] [rbp-D9h]
  char v346; // [rsp+0h] [rbp-D9h]
  int v347; // [rsp+0h] [rbp-D9h]
  __int64 v348; // [rsp+0h] [rbp-D9h]
  __int64 v349; // [rsp+0h] [rbp-D9h]
  int v350; // [rsp+0h] [rbp-D9h]
  __int64 v351; // [rsp+0h] [rbp-D9h]
  __int64 v352; // [rsp+0h] [rbp-D9h]
  int v353; // [rsp+0h] [rbp-D9h]
  __int64 v354; // [rsp+8h] [rbp-D1h]
  int v355; // [rsp+8h] [rbp-D1h]
  __int64 v356; // [rsp+8h] [rbp-D1h]
  __int64 v357; // [rsp+8h] [rbp-D1h]
  __int64 v358; // [rsp+8h] [rbp-D1h]
  char v359; // [rsp+8h] [rbp-D1h]
  __int64 v360; // [rsp+8h] [rbp-D1h]
  __int64 v361; // [rsp+8h] [rbp-D1h]
  __int64 v362; // [rsp+8h] [rbp-D1h]
  __int64 v363; // [rsp+8h] [rbp-D1h]
  __int64 v364; // [rsp+10h] [rbp-C9h]
  int v365; // [rsp+10h] [rbp-C9h]
  int v366; // [rsp+10h] [rbp-C9h]
  __int64 v367; // [rsp+10h] [rbp-C9h]
  __int64 v368; // [rsp+10h] [rbp-C9h]
  __int64 v369; // [rsp+10h] [rbp-C9h]
  __int64 v370; // [rsp+10h] [rbp-C9h]
  __int64 v371; // [rsp+10h] [rbp-C9h]
  int v372; // [rsp+18h] [rbp-C1h]
  __int64 v373; // [rsp+18h] [rbp-C1h]
  __int64 v374; // [rsp+18h] [rbp-C1h]
  __int64 v375; // [rsp+18h] [rbp-C1h]
  __int64 v376; // [rsp+18h] [rbp-C1h]
  __int64 v377; // [rsp+18h] [rbp-C1h]
  __int64 v378; // [rsp+40h] [rbp-99h] BYREF
  int v379; // [rsp+48h] [rbp-91h]
  __int64 v381; // [rsp+50h] [rbp-89h] BYREF
  int v382; // [rsp+58h] [rbp-81h]
  __int64 v384; // [rsp+70h] [rbp-69h] BYREF
  __int64 v385; // [rsp+78h] [rbp-61h]
  __m256 v386; // [rsp+80h] [rbp-59h] BYREF
  _BYTE v388[8]; // [rsp+C0h] [rbp-19h] BYREF
  char v390; // [rsp+100h] [rbp+27h] BYREF
  _UNKNOWN *retaddr; // [rsp+138h] [rbp+5Fh] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
  }
  v21 = 0;
  MEMORY[0x13213A0](a1, a12, *MEMORY[0x131F8B8], &v378);
  v23 = sub_982820(MEMORY[0x131F8A8], v22, 25756692, &v381, 25756672, 0);
  v26 = 20167936;
  if ( *(_DWORD *)(v23 + 8) != 0 )
    v27 = *(_QWORD *)v23;
  else
    v27 = 20167936;
  MEMORY[0x13213A0](v24, v25, v27, &a18);
  sub_10915AC(v28, v29, a11, v29, -1, (unsigned int)&a18);
  MEMORY[0x13213C8](v31, v32, v30, &a18);
  sub_1DDA20(v34, v35, v33, (unsigned int)&v381, v36, v37);
  MEMORY[0x13213C8](v39, v40, v38, &v378);
  *(_QWORD *)(v41 + 680) = 23683640;
  v42 = (_QWORD *)(v41 + 688);
  sub_267074(v41 + 680, v41, v43, v41 + 688, v44, v45, v341);
  v47 = (_QWORD *)(v46 + 696);
  sub_110DB30(v48, v46, v46, v46 + 696, v49, v50, v342, v354);
  *(_QWORD *)v51 = 26105984;
  *v52 = 26105920;
  *v42 = 26105968;
  *v47 = 26105896;
  v53 = 0;
  *(_QWORD *)(v51 + 752) = 0;
  *(_QWORD *)(v51 + 760) = 0;
  *(_QWORD *)(v51 + 768) = 0;
  *(_DWORD *)(v51 + 776) = 8;
  *(_QWORD *)(v51 + 780) = 0;
  *(_QWORD *)(v51 + 788) = 0;
  *(_QWORD *)(v51 + 796) = 0;
  *(_DWORD *)(v51 + 804) = 8;
  *(_QWORD *)(v51 + 808) = 0;
  *(_QWORD *)(v51 + 816) = 0;
  *(_QWORD *)(v51 + 824) = 0;
  *(_DWORD *)(v51 + 832) = 8;
  *(_QWORD *)(v51 + 868) = 0;
  *(_QWORD *)(v51 + 876) = 0;
  *(_QWORD *)(v51 + 884) = 0;
  *(_QWORD *)(v51 + 892) = 0;
  *(_QWORD *)(v51 + 900) = 0;
  *(_QWORD *)(v51 + 908) = 0;
  *(_QWORD *)(v51 + 916) = 0;
  *(_QWORD *)(v51 + 924) = 0;
  MEMORY[0x1320740](v52, v51, 0xFFFFFFFFLL, v51, 12656, sub_118807C);
  *(_QWORD *)(v54 + 860) = a14;
  *(_QWORD *)(v54 + 836) = 0;
  *(_QWORD *)(v54 + 844) = 0;
  *(_QWORD *)(v54 + 932) = 0;
  *(_QWORD *)(v54 + 940) = 0;
  *(_QWORD *)(v54 + 948) = 0;
  *(_DWORD *)(v54 + 988) = 0;
  *(_DWORD *)(v54 + 992) = 1;
  *(_DWORD *)(v54 + 996) = 1;
  *(_DWORD *)(v54 + 1000) = 2;
  *(_DWORD *)(v54 + 1004) = 1;
  *(_QWORD *)(v54 + 1008) = 1;
  *(_QWORD *)(v54 + 1016) = 1;
  *(_QWORD *)(v54 + 1072) = 1;
  *(_QWORD *)(v54 + 1080) = 0;
  *(_DWORD *)(v54 + 1088) = 0;
  *(_DWORD *)(v54 + 1104) = 0;
  *(_QWORD *)(v54 + 1028) = 0;
  *(_DWORD *)(v54 + 1024) = 0;
  *(_QWORD *)(v54 + 1036) = 0;
  *(_QWORD *)(v54 + 1044) = 0;
  *(_QWORD *)(v54 + 1052) = 0;
  *(_DWORD *)(v54 + 1060) = 0;
  *(_QWORD *)(v54 + 1184) = 0;
  *(_DWORD *)(v54 + 1192) = 0;
  *(_DWORD *)(v54 + 1068) = 0;
  *(_QWORD *)(v54 + 980) = 0;
  a18 = 0;
  v55 = sub_4F62B0();
  v59 = sub_14BF38(v57, v58, v56, v55, 0);
  *(_QWORD *)(v60 + 964) = v59;
  a18 = 0;
  v61 = sub_42140C();
  v65 = sub_14BF38(v63, v64, v62, v61, 0);
  *(_QWORD *)(v66 + 972) = v65;
  v67 = *(_DWORD *)(*(_QWORD *)(v66 + 964) + 176LL);
  *(_DWORD *)(v65 + 144) = v67;
  *(_DWORD *)(*(_QWORD *)(v66 + 972) + 148LL) = *(_DWORD *)(*(_QWORD *)(v66 + 964) + 180LL);
  *(_DWORD *)(*(_QWORD *)(v66 + 972) + 152LL) = *(_DWORD *)(*(_QWORD *)(v66 + 964) + 184LL);
  *(_DWORD *)(*(_QWORD *)(v66 + 972) + 156LL) = *(_DWORD *)(*(_QWORD *)(v66 + 964) + 188LL);
  __asm
  {
    vxorps  xmm7, xmm7, xmm7
    vmovss  dword ptr [rsp+130h+var_F0], xmm7
    vmovss  dword ptr [rsp+130h+var_F0+4], xmm7
    vmovss  [rsp+130h+var_E8], xmm7
  }
  v73 = sub_467570(v70, v66, v67, (unsigned int)&v378, v71, v72, v343, v355);
  *(_QWORD *)(v74 + 852) = v73;
  v77 = sub_1BF984(v76, v74, v75, 1792);
  a18 = v77;
  if ( v77 != 0 )
    v81 = sub_1016B00(v78, v79, v79, v77, -1, v80, v344, v356);
  else
    v81 = 0;
  *(_QWORD *)(v79 + 720) = v81;
  MEMORY[0x131FF30](v78, v79, v81, v79);
  v85 = sub_1BF984(v83, v84, v82, 560);
  a18 = v85;
  if ( v85 != 0 )
    v90 = sub_10169A0(v87, (_DWORD)v88, (_DWORD)v88, v85, v89, (_DWORD)v88, v344, v356);
  else
    v90 = 0;
  v88[92] = v90;
  *(_QWORD *)((char *)v88 + 844) = *(_QWORD *)(v90 + 552);
  _RAX = *(_QWORD *)((char *)v88 + 932);
  __asm
  {
    vmovups ymm0, ymmword ptr [rax+0A0h]
    vmovups [rbp+57h+var_B0], ymm0
    vmovups ymm1, ymmword ptr [rax+0C0h]
    vmovups [rbp+57h+var_90], ymm1
  }
  __asm { vzeroupper }
  sub_F10F4(v87, v88, v86, &v386);
  a18 = 0;
  v94 = sub_1918A8();
  v97 = sub_1918D8(v95, v96, -96, v94, 0);
  *(_QWORD *)(*(_QWORD *)(v98 + 932) + 380LL) = v97;
  v99 = *(_QWORD *)(v98 + 932);
  __asm
  {
    vmovss  xmm0, dword ptr cs:17061CCh
    vmovss  dword ptr [rsp+130h+var_F0], xmm0
    vmovss  dword ptr [rsp+130h+var_F0+4], xmm0
    vmovss  [rsp+130h+var_E8], xmm0
  }
  sub_2C2EFC(
    *(double *)&_XMM0,
    v102,
    v103,
    v104,
    v105,
    v106,
    v107,
    v108,
    v101,
    v98,
    *(_QWORD *)(*(_QWORD *)(v99 + 544) + 248LL),
    *(_QWORD *)(v99 + 380),
    (__int64)&v386,
    (int)&v378,
    v344,
    v356,
    v364,
    v372,
    1,
    v99,
    *(_QWORD *)(v98 + 852));
  v115 = sub_1BF984(v110, v111, v109, 632);
  v378 = v115;
  if ( v115 != 0 )
  {
    MEMORY[0x13213A0](MEMORY[0x131F8B0], v114, 25756752, &a18);
    v21 = 1;
    MEMORY[0x131D2F0](v116, v117, v117, v115, 0xFFFFFFFFLL, &a18);
    *(_QWORD *)v115 = 23749272;
    v113 = v114 + 680;
    v53 = 0;
  }
  else
  {
    v115 = 0;
  }
  if ( (v21 & 1) != 0 )
  {
    v21 &= ~1u;
    MEMORY[0x13213C8](v113, v114, v112, &a18);
  }
  MEMORY[0x131D2D8](v113, v114, 0, v115);
  MEMORY[0x131D2D0](v118, v119, 0, v115);
  (*(void (__fastcall **)(__int64, __int64, _QWORD, __int64, _QWORD))(*(_QWORD *)v115 + 1440LL))(
    v120,
    v121,
    *(_QWORD *)(v121 + 736),
    v115,
    0);
  MEMORY[0x131D2C0](v122, v123, 2, *(_QWORD *)(v123 + 712), v115);
  v127 = sub_1BF984(v125, v126, v124, 680);
  a18 = v127;
  if ( v127 != 0 )
    v131 = sub_AC638C(v129, v130, v128, v127);
  else
    v131 = 0;
  *(_QWORD *)(v130 + 836) = v131;
  (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64))(*(_QWORD *)v131 + 1408LL))(
    v129,
    v130,
    v130,
    v131,
    v129);
  v134 = sub_9828A0(v132, v133, 25756704, &v381, 25756768, 0);
  if ( *(_DWORD *)(v134 + 8) != 0 )
    v136 = *(_QWORD *)v134;
  else
    LODWORD(v136) = 20167936;
  sub_110DE7C(4, v135, *(_QWORD *)(v135 + 836), (_DWORD)v47, 4, v136, v345, v357, v365, v373, 0);
  sub_1DDA20(v138, v139, v137, (unsigned int)&v381, v140, v141);
  v145 = sub_1BF984(v143, v144, v142, 764);
  a18 = v145;
  if ( v145 != 0 )
    v149 = sub_101AAE0(v147, v148, v146, v145);
  else
    v149 = 0;
  *(_QWORD *)(v148 + 744) = v149;
  (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64, _QWORD))(*(_QWORD *)v149 + 1640LL))(
    v147,
    v148,
    v148,
    v149,
    12655,
    0);
  v152 = sub_9828A0(v150, v151, 25757384, &v381, 25756728, 0);
  if ( *(_DWORD *)(v152 + 8) != 0 )
    v155 = *(_QWORD *)v152;
  else
    LODWORD(v155) = 20167936;
  sub_110DE7C(v153, v154, *(_QWORD *)(v154 + 744), (_DWORD)v47, v153, v155, v346, v358, v366, v374, 0);
  sub_1DDA20(v157, v158, v156, (unsigned int)&v381, v159, v160);
  sub_110E318(v162, v163, v161, (_DWORD)v47, v164, v165, v347, v359, v367);
  v169 = sub_1BF984(v167, v168, v166, 744);
  a18 = v169;
  if ( v169 != 0 )
    v175 = sub_1016130(v171, v172, v170, v169, v173, v174, v348, v360, v368, v375);
  else
    v175 = 0;
  *(_QWORD *)(v172 + 728) = v175;
  sub_110E0E0(v171, v172, v175, (_DWORD)v47, v173, v174, v348, v360, v368);
  MEMORY[0x131FED8](v176, v177, *(_QWORD *)(v177 + 728), v177);
  v180 = *(_QWORD *)(v179 + 860);
  *(_QWORD *)(v179 + 956) = *(_QWORD *)(v180 + 96);
  v181 = 0;
  v182 = 0;
  if ( *(int *)(v180 + 112) > 0 )
  {
    v178 = 0;
    do
    {
      v183 = *(_QWORD *)(v178 + *(_QWORD *)(v180 + 104));
      if ( *(_DWORD *)(v183 + 116) + *(_DWORD *)(v183 + 132) + *(_DWORD *)(v183 + 148) + *(_DWORD *)(v183 + 164) == 0 )
      {
        __asm { vzeroupper }
        sub_1BEC94(v178, v179, 1, v183 + 124, 80, 8);
        _RAX = *(_DWORD **)(v183 + 124);
        __asm
        {
          vmovups ymm0, ymmword ptr cs:1AF8A70h
          vmovups ymmword ptr [rax], ymm0
          vmovups ymm1, ymmword ptr cs:1AF8A90h
          vmovups ymmword ptr [rax+20h], ymm1
        }
        _RAX[16] = 1097859072;
        _RAX[17] = 1097859072;
        _RAX[18] = 1097859072;
        v181 = 1;
      }
      ++v182;
      v178 += 8;
      v180 = *(_QWORD *)(v179 + 860);
    }
    while ( v182 < *(_DWORD *)(v180 + 112) );
    v53 = 0;
    if ( v181 != 0 )
    {
      __asm { vzeroupper }
      v187 = sub_982820(v178, v179, 25757352, &v381, 25757400, 0);
      if ( *(_DWORD *)(v187 + 8) != 0 )
        v192 = *(_QWORD *)v187;
      else
        LODWORD(v192) = 20167936;
      sub_4FFEE8(v188, v189, v192, 0, v190, v191, v349, v361, v369);
      sub_1DDA20(v194, v195, v193, (unsigned int)&v381, v196, v197);
    }
  }
  __asm { vzeroupper }
  sub_CF3564(v178, v179, v179 + 868, *(_QWORD *)(v179 + 956), 1);
  sub_CF3564(v198, v199, v199 + 884, *(_QWORD *)(v199 + 956), 0);
  sub_3F0250(v200, v201, *(_QWORD *)(v201 + 956), *(_QWORD *)(v201 + 940), 0);
  sub_48B660(v202, v203, *(_QWORD *)(v203 + 860), *(_QWORD *)(v203 + 940), 0, v204, v349);
  sub_C60EC4(v206, v207, v205, *(_QWORD *)(v207 + 860));
  sub_DDF594(v209, v210, v208, *(_QWORD *)(v210 + 860), v211, v212, v350, v361, v369, v375);
  *(_DWORD *)(*(_QWORD *)(v213 + 972) + 120LL) = *(_DWORD *)(*(_QWORD *)(*(_QWORD *)(v213 + 860) + 152LL) + 180LL);
  *(_DWORD *)(*(_QWORD *)(v213 + 972) + 124LL) = *(_DWORD *)(*(_QWORD *)(*(_QWORD *)(v213 + 860) + 152LL) + 184LL);
  v215 = 0;
  v384 = 0;
  v216 = 0;
  v385 = 0;
  v217 = 0;
  v218 = *(_QWORD *)(v213 + 860);
  if ( *(int *)(v218 + 112) > 0 )
  {
    do
    {
      a18 = *(_QWORD *)(*(_QWORD *)(*(_QWORD *)(v218 + 104) + v53) + 180LL);
      if ( (unsigned int)sub_F8D80(v217, v213, a18, *(_QWORD *)(v213 + 956), v215, v214, v351, v362) == -1 )
      {
        v221 = sub_1E0740(v219, v213, &v381, &a18, v220);
        if ( *(_DWORD *)(v221 + 8) != 0 )
          v225 = *(_QWORD *)v221;
        else
          LODWORD(v225) = 20167936;
        sub_1E15EC(v222, v223, 25757368, (unsigned int)&v378, v225, v224, v351, v362, v370, v376);
        v21 |= 2u;
        v230 = 20167936;
        if ( v379 != 0 )
          v230 = v378;
        sub_1E127C(v226, v227, v230, (unsigned int)&v384, v228, v229, v352, v363, v371, v377);
        sub_1DDA20(v232, v233, v231, (unsigned int)&v378, v234, v235);
        sub_1DDA20(v237, v238, v236, (unsigned int)&v381, v239, v240);
      }
      v217 = (unsigned int)(v219 + 1);
      v53 += 8;
      v218 = *(_QWORD *)(v213 + 860);
    }
    while ( (int)v217 < *(_DWORD *)(v218 + 112) );
    v216 = v385;
    v215 = v384;
  }
  if ( (v216 != 0 ? v216 - 1 : 0) > 0 )
  {
    if ( v216 != 0 )
      v26 = v215;
    v241 = (_QWORD *)sub_982820(v217, v213, 25757480, &v381, 25757456, 0);
    v247 = sub_46B868(v243, v244, v242, *v241, v245, v246, v351);
    sub_4FFEE8(v248, v249, v247, 0, v26, v250, v353, v362, v370);
    sub_1DDA20(v252, v253, v251, (unsigned int)&v381, v254, v255);
  }
  sub_E2F80(v217, v213, v388, *(_QWORD *)(v213 + 860), *(_QWORD *)(v213 + 940));
  __asm
  {
    vmovaps ymm0, ymmword ptr cs:18FF360h
    vmovups [rbp+57h+var_B0], ymm0
    vmovdqu xmm0, xmmword ptr cs:17061F0h
    vmovups xmmword ptr [rbp+57h+var_90], xmm0
    vmovss  dword ptr [rbp+57h+var_90+10h], xmm7
    vmovss  dword ptr [rbp+57h+var_90+14h], xmm7
  }
  _RAX = *(_QWORD *)(v259 + 972);
  __asm
  {
    vmovss  xmm0, dword ptr [rax+88h]
    vsubss  xmm1, xmm0, [rbp+57h+var_68]
    vmovss  dword ptr [rbp+57h+var_90+18h], xmm1
    vmovss  xmm2, dword ptr cs:17061B8h
    vmovss  dword ptr [rbp+57h+var_90+1Ch], xmm2
  }
  __asm { vzeroupper }
  sub_DC91C(v264, v259, &v386, *(_QWORD *)(v259 + 940));
  _RAX = sub_E2F80(v265, v266, &v381, *(_QWORD *)(v266 + 860), *(_QWORD *)(v266 + 940));
  __asm
  {
    vmovups xmm6, xmmword ptr [rax]
    vmovsd  xmm0, qword ptr [rax+10h]
    vmovsd  [rbp+57h+var_60], xmm0
  }
  _RAX = *(_QWORD *)(v270 + 940);
  __asm
  {
    vmovups ymm0, ymmword ptr [rax+0A0h]
    vmovups [rbp+57h+var_B0], ymm0
    vmovups ymm1, ymmword ptr [rax+0C0h]
    vmovups [rbp+57h+var_90], ymm1
  }
  _RCX = *(_QWORD *)(v270 + 956);
  __asm
  {
    vmovsd  xmm0, qword ptr [rcx+60h]
    vmovsd  [rsp+130h+var_E0], xmm0
  }
  v382 = *(_DWORD *)(_RCX + 104);
  __asm
  {
    vmovss  xmm0, dword ptr [rcx+78h]
    vmovss  [rbp+57h+var_D4], xmm0
  }
  __asm { vzeroupper }
  sub_DD0D70(v277, v270, &v378, &v381, &v386, *(double *)&_XMM0, v278, v279, v280, v281, v282, *(double *)&_XMM6);
  __asm
  {
    vshufps xmm0, xmm6, xmm6, 0FFh
    vsubss  xmm0, xmm0, xmm6
    vmulss  xmm3, xmm0, dword ptr cs:17060E4h
    vshufps xmm6, xmm6, xmm6, 55h ; 'U'
    vmovss  xmm0, dword ptr [rbp+57h+var_60]
    vsubss  xmm1, xmm0, xmm6
    vmulss  xmm2, xmm1, dword ptr cs:17060E4h
    vmovss  xmm0, [rsp+130h+var_E4]
    vmovss  xmm4, [rsp+130h+var_E8]
  }
  _RAX = *(_QWORD *)(v285 + 844);
  __asm
  {
    vcomiss xmm3, xmm2
    vmulss  xmm2, xmm0, dword ptr cs:17063BCh
  }
  if ( v300 | v301 )
  {
    __asm
    {
      vmovss  xmm3, dword ptr [rsp+130h+var_F0+4]
      vmovss  xmm1, dword ptr [rsp+130h+var_F0]
      vsubss  xmm2, xmm1, xmm2
      vmovss  dword ptr [rax+18h], xmm2
      vmovss  dword ptr [rax+1Ch], xmm3
      vmovss  dword ptr [rax+20h], xmm4
    }
    v309 = *(_QWORD *)(v285 + 844);
    *(_QWORD *)(v309 + 36) = 0;
    *(_DWORD *)(v309 + 44) = 0;
  }
  else
  {
    __asm
    {
      vmovss  xmm1, dword ptr [rsp+130h+var_F0+4]
      vsubss  xmm3, xmm1, xmm2
      vmovss  xmm0, dword ptr [rsp+130h+var_F0]
      vmovss  dword ptr [rax+18h], xmm0
      vmovss  dword ptr [rax+1Ch], xmm3
      vmovss  dword ptr [rax+20h], xmm4
    }
    v305 = *(_QWORD *)(v285 + 844);
    *(_DWORD *)(v305 + 36) = 0;
    *(_QWORD *)(v305 + 40) = 0x4000;
  }
  sub_1184C10(v284, v285, v283, *(_QWORD *)(v285 + 844), v286, v287, v351);
  sub_11869D0(v311, v312, v310, v312);
  sub_1189864(v313, v314, 0xFFFFFFFFLL, v314, 4);
  sub_1189AC0(v315, v316, 0xFFFFFFFFLL, v316);
  sub_1189788(v317, v318, 1, v318);
  sub_11849DC(v320, v321, v319, v321);
  sub_118BFB8(v323, v324, v322, v324);
  __asm
  {
    vmovss  dword ptr [rsp+130h+var_F0], xmm7
    vmovss  dword ptr [rsp+130h+var_F0+4], xmm7
    vmovss  [rsp+130h+var_E8], xmm7
  }
  sub_A2F6FC(v326, v327, &v378, *(_QWORD *)(v327 + 844));
  sub_101BDB0(v329, v330, v328, v330);
  sub_1DDA20(v332, v333, v331, (unsigned int)&v384, v334, v335);
  _R11 = &v390;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
  }
  return v336;
}
