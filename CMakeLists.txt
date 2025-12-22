cmake_minimum_required(VERSION 3.17)

# Proje adı ve versiyonu
project(FaceAuth_Cpp VERSION 3.31.10 LANGUAGES CXX)

# --- 1. GENEL AYARLAR ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Portabilite için standart dışı uzantıları kapat

# --- 2. BAĞIMLILIKLARIN BULUNMASI ---
find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# ONNX Runtime'ı bulma
find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATHS /usr/include/onnxruntime /usr/local/include/onnxruntime
        DOC "ONNX Runtime include directory"
)

if(NOT ONNXRUNTIME_INCLUDE_DIR)
    message(FATAL_ERROR "ONNX Runtime headers not found! Please install onnxruntime-devel.")
endif()

# PAM Kütüphanesini Bulma (YENİ)
find_library(PAM_LIBRARY NAMES pam DOC "PAM library")
find_path(PAM_INCLUDE_DIR NAMES security/pam_modules.h DOC "PAM include dir")

if (NOT PAM_LIBRARY OR NOT PAM_INCLUDE_DIR)
    message(FATAL_ERROR "PAM development files not found! Please run: sudo apt-get install libpam0g-dev")
endif()

# --- 3. UYGULAMA 1: DAEMON (SERVİS) ---
# Arka planda çalışan, yapay zeka işlemlerini yapan ana servis.

set(DAEMON_SOURCES
        src/daemon/main.cpp
        src/daemon/SocketServer.cpp
        src/modules/Detector.cpp
        src/modules/SpoofDetector.cpp
        src/modules/FaceMemory.cpp
        include/Detector.h
        include/SocketServer.h
        include/SpoofDetector.h
        include/FaceMemory.h
)

add_executable(faceauthd ${DAEMON_SOURCES})

target_include_directories(faceauthd PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${OpenCV_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${ONNXRUNTIME_INCLUDE_DIR}
)

target_link_libraries(faceauthd PRIVATE
        ${OpenCV_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        onnxruntime
        Threads::Threads
)

# --- 4. UYGULAMA 2: CLI (İSTEMCİ) ---
# Kullanıcının terminalden komut göndermesini sağlayan araç.

add_executable(faceauth-cli src/cli/main.cpp)

target_include_directories(faceauth-cli PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# --- 5. UYGULAMA 3: PAM MODÜLÜ (YENİ) ---
# Linux login süreçlerine entegre olan kütüphane (.so dosyası).
# MODULE tipi, bu dosyanın çalıştırılabilir olmadığını, yüklenebilir bir modül olduğunu belirtir.

add_library(pam_faceauth MODULE src/pam/pam_faceauth.cpp)

target_include_directories(pam_faceauth PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include" # Config.h'ye erişim için
        ${PAM_INCLUDE_DIR}
)

target_link_libraries(pam_faceauth PRIVATE
        ${PAM_LIBRARY}
)

# Linux standardı gereği PAM modüllerinin başında "lib" öneki olmaz.
# CMake varsayılan olarak "libpam_faceauth.so" yapar, bunu engelliyoruz:
set_target_properties(pam_faceauth PROPERTIES PREFIX "")

# --- 6. DERLEYİCİ SEÇENEKLERİ ---
# Tüm hedefler için uyarıları açıyoruz
if(MSVC)
    target_compile_options(faceauthd PRIVATE /W4)
    target_compile_options(faceauth-cli PRIVATE /W4)
    target_compile_options(pam_faceauth PRIVATE /W4)
else()
    target_compile_options(faceauthd PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(faceauth-cli PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pam_faceauth PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- 7. DOSYA KOPYALAMA ---
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/models" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# --- BİLGİLENDİRME ---
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "ONNX Runtime: ${ONNXRUNTIME_INCLUDE_DIR}")
message(STATUS "PAM Library: ${PAM_LIBRARY}")
message(STATUS "Targets: faceauthd (Daemon), faceauth-cli (Client), pam_faceauth.so (PAM Module)")
