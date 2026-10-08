option(PROJ_GNUTLS_DEBUG "Enable GnuTLS debug mode" OFF)

option(PROJ_ADMIN_PRIV_ELEVATION "Admin privileges elevation; don't turn it off in production!! (UAC on Windows) " ON)

if(MINGW)
    set(DEFAULT_VPNC_SCRIPT "vpnc-script.js")
elseif(APPLE)
    set(DEFAULT_VPNC_SCRIPT "../Resources/vpnc-script")
else()
    # Prefer the script we ship with the package; vpninfo falls back to the
    # distro-provided locations (/etc/vpnc/vpnc-script, ...) when it is absent.
    set(DEFAULT_VPNC_SCRIPT "/usr/share/openconnect-gui/vpnc-script")
endif()
option(PROJ_PKCS11 "Enable PKCS11" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-pipe -Wall -Wextra -Wpedantic -Wno-unused-parameter)
#add_compile_options(-Weffc++)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Werror)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
