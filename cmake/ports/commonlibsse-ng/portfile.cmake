# CommonLibSSE-NG v7.5.4. This includes the corrected Skyrim 1.7.x
# PlayerCharacter runtime-data accessors introduced in v7.5.3.
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/alandtse/CommonLibSSE-NG.git
    REF c5424463bba9af0d75cde8640ba7ddd4cacb9e39
)

# This CNO update targets Skyrim SE/AE 1.7.104.0 only. Building CommonLib
# without VR avoids exporting conflicting ENABLE_SKYRIM_VR definitions and
# removes the OpenVR dependency from the consumer build.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DSKSE_SUPPORT_XBYAK=ON
        -DSKSE_SUPPORT_PATCH_SAFETY=OFF
        -DENABLE_SKYRIM_SE=ON
        -DENABLE_SKYRIM_AE=ON
        -DENABLE_SKYRIM_VR=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake/CommonLibSSE)
vcpkg_copy_pdbs()

# add_commonlibsse_plugin() lives in this helper and is referenced by the
# installed CommonLibSSEConfig.cmake.
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/COPYING.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(INSTALL "${SOURCE_PATH}/EXCEPTIONS.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
