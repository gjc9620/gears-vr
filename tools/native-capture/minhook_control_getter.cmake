# Build-local, exact scalar getter compatibility; vendor source stays pristine.
file(SHA256 "${MH_SOURCE_DIR}/src/trampoline.c" GEARS_MH_GETTER_VENDOR_SHA)
if(NOT GEARS_MH_GETTER_VENDOR_SHA STREQUAL "824ed887ec9da606960962af8973205d1f97cd03241e75517eba485280fb6377")
    message(FATAL_ERROR "Unexpected MinHook trampoline source; exact getter patch rejected")
endif()
file(READ "${MH_SOURCE_DIR}/src/trampoline.c" GEARS_MH_GETTER_VENDOR)
set(GEARS_MH_GETTER_BEFORE [=[        if (hs.flags & F_ERROR)
            return FALSE;]=])
set(GEARS_MH_GETTER_AFTER [=[        if (hs.flags & F_ERROR)
        {
#if defined(_M_X64) || defined(__x86_64__)
            const UINT8 *code = (const UINT8 *)pOldInst;
            // Exact certified getter only: vmovss xmm0,[rcx+70h]; ret.
            // Non-RIP-relative 5-byte instruction; all other errors stay errors.
            if (oldPos == 0 && code[0] == 0xC5 && code[1] == 0xFA
                && code[2] == 0x10 && code[3] == 0x41
                && code[4] == 0x70 && code[5] == 0xC3)
            {
                ZeroMemory(&hs, sizeof(hs));
                hs.len = 5;
                hs.opcode = 0x10;
                hs.modrm = 0x41;
                hs.modrm_mod = 1;
                hs.modrm_rm = 1;
                hs.disp.disp8 = 0x70;
                hs.flags = F_MODRM | F_DISP8;
                copySize = hs.len;
            }
            else
#endif
                return FALSE;
        }]=])
string(FIND "${GEARS_MH_GETTER_VENDOR}" "\r\n" GEARS_MH_GETTER_CRLF)
if(NOT GEARS_MH_GETTER_CRLF LESS 0)
    string(REPLACE "\n" "\r\n" GEARS_MH_GETTER_BEFORE "${GEARS_MH_GETTER_BEFORE}")
    string(REPLACE "\n" "\r\n" GEARS_MH_GETTER_AFTER "${GEARS_MH_GETTER_AFTER}")
endif()
string(REPLACE "${GEARS_MH_GETTER_BEFORE}" "" GEARS_MH_GETTER_REMOVED "${GEARS_MH_GETTER_VENDOR}")
string(LENGTH "${GEARS_MH_GETTER_VENDOR}" GEARS_MH_GETTER_VENDOR_LENGTH)
string(LENGTH "${GEARS_MH_GETTER_REMOVED}" GEARS_MH_GETTER_REMOVED_LENGTH)
string(LENGTH "${GEARS_MH_GETTER_BEFORE}" GEARS_MH_GETTER_BEFORE_LENGTH)
math(EXPR GEARS_MH_GETTER_MATCHED "${GEARS_MH_GETTER_VENDOR_LENGTH}-${GEARS_MH_GETTER_REMOVED_LENGTH}")
if(NOT GEARS_MH_GETTER_MATCHED EQUAL GEARS_MH_GETTER_BEFORE_LENGTH)
    message(FATAL_ERROR "MinHook exact getter patch requires one unchanged decoder guard")
endif()
string(REPLACE "${GEARS_MH_GETTER_BEFORE}" "${GEARS_MH_GETTER_AFTER}" GEARS_MH_GETTER_DERIVED "${GEARS_MH_GETTER_VENDOR}")
set(GEARS_MH_CONTROL_TRAMPOLINE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/gears_minhook_trampoline.c")
file(WRITE "${GEARS_MH_CONTROL_TRAMPOLINE_SOURCE}" "${GEARS_MH_GETTER_DERIVED}")
file(SHA256 "${GEARS_MH_CONTROL_TRAMPOLINE_SOURCE}" GEARS_MH_GETTER_DERIVED_SHA)
if(NOT GEARS_MH_GETTER_DERIVED_SHA STREQUAL "825edb0f2e75fc4a72d2f1f374040aa672badfd997aed27ded78ae7b86bdd4b3")
    message(FATAL_ERROR "MinHook generated getter source differs from the tested exact patch")
endif()
