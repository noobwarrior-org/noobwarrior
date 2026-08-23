# Copyright (C) 2026 Hattozo
#
# This file is part of noobWarrior.
#
# noobWarrior is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# noobWarrior is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with noobWarrior; if not, see
# <https://www.gnu.org/licenses/>.
# === noobWarrior ===
# File: FindMbedTLS.cmake
# Description: Exposes LocalRcc's in-tree static Mbed TLS to libdatachannel and libSRTP.

if (NOT TARGET MbedTLS::mbedtls OR
    NOT TARGET MbedTLS::mbedx509 OR
    NOT TARGET MbedTLS::mbedcrypto OR
    NOT NOOBWARRIOR_LOCALRCC_MBEDTLS_INCLUDE_DIR)
    set(MbedTLS_FOUND FALSE)
    set(MBEDTLS_FOUND FALSE)
    if (MbedTLS_FIND_REQUIRED)
        message(FATAL_ERROR
            "LocalRcc's static Mbed TLS targets are unavailable")
    endif()
    return()
endif()

if (NOT TARGET MbedTLS::MbedTLS)
    add_library(MbedTLS::MbedTLS INTERFACE IMPORTED GLOBAL)
    set_property(TARGET MbedTLS::MbedTLS PROPERTY
        INTERFACE_LINK_LIBRARIES
        "MbedTLS::mbedtls;MbedTLS::mbedx509;MbedTLS::mbedcrypto")
    set_property(TARGET MbedTLS::MbedTLS PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES
        "${NOOBWARRIOR_LOCALRCC_MBEDTLS_INCLUDE_DIR}")
endif()

set(MbedTLS_VERSION "3.6.4")
set(MbedTLS_INCLUDE_DIR "${NOOBWARRIOR_LOCALRCC_MBEDTLS_INCLUDE_DIR}")
set(MBEDTLS_INCLUDE_DIRS "${NOOBWARRIOR_LOCALRCC_MBEDTLS_INCLUDE_DIR}")
set(MbedTLS_LIBRARIES MbedTLS::MbedTLS)
set(MBEDTLS_LIBRARIES MbedTLS::MbedTLS)
set(MbedTLS_FOUND TRUE)
set(MBEDTLS_FOUND TRUE)
