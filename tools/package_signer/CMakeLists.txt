add_library(drs_controlled_package_signing STATIC
    src/ControlledPackageSigner.cpp
    include/drs/signing/ControlledPackageSigner.h
)
target_include_directories(drs_controlled_package_signing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(drs_controlled_package_signing PUBLIC cxx_std_17)
target_compile_definitions(drs_controlled_package_signing PRIVATE SODIUM_STATIC=1)
target_link_libraries(drs_controlled_package_signing
    PUBLIC drs_engine_adapter
    PRIVATE drs_libsodium)
set_target_properties(drs_controlled_package_signing PROPERTIES FOLDER "Controlled Signing")

option(DRS_BUILD_CONTROLLED_PACKAGE_SIGNER
    "Build the CI/service-only detached package signing executable" OFF)
if(DRS_BUILD_CONTROLLED_PACKAGE_SIGNER)
    add_executable(drs_controlled_package_signer src/PackageSignerMain.cpp)
    target_compile_features(drs_controlled_package_signer PRIVATE cxx_std_17)
    target_link_libraries(drs_controlled_package_signer
        PRIVATE drs_controlled_package_signing drs_libsodium)
    set_target_properties(drs_controlled_package_signer PROPERTIES FOLDER "Controlled Signing")
endif()
