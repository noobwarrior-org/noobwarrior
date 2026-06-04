if (TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    set(OpenSSL_FOUND TRUE)
    set(OPENSSL_FOUND TRUE)
    if (NOT TARGET OpenSSL::applink)
        add_library(OpenSSL::applink INTERFACE IMPORTED)
    endif()
    return()
endif()

if (OpenSSL_FIND_REQUIRED)
    message(FATAL_ERROR
        "FindOpenSSL shim: OpenSSL::SSL / OpenSSL::Crypto targets weren't created before find_package(OpenSSL) was called."
    )
endif()
set(OpenSSL_FOUND FALSE)
set(OPENSSL_FOUND FALSE)
