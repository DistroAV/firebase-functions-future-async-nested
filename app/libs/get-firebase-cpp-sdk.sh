#!/bin/bash

# https://api.github.com/repos/firebase/firebase-cpp-sdk/releases/latest
# https://github.com/firebase/firebase-cpp-sdk/releases/latest

SDK_VERSION="12.1.0"
ZIP_FILE="firebase_cpp_sdk_${SDK_VERSION}.zip"
URL="https://dl.google.com/firebase/sdk/cpp/${ZIP_FILE}"
FIREBASE_CPP_SDK_DIR="./firebase_cpp_sdk"

rm -rf ${FIREBASE_CPP_SDK_DIR}
mkdir -p ${FIREBASE_CPP_SDK_DIR}

pushd ${FIREBASE_CPP_SDK_DIR} >/dev/null

echo "Downloading Firebase C++ SDK version ${SDK_VERSION}..."
curl -L ${URL} -o ${ZIP_FILE}

echo "Extracting ${ZIP_FILE}..."
extraction_inclusions=(
  "include/*"
  "libs/linux/x86_64/cxx11/*"
  "libs/darwin/universal/*"
  "libs/windows/*/MD/x64/*"
  "CMakeLists.txt"
  "NOTICES"
  "readme.md"
)
BASE_DIR_NAME=$(basename ${FIREBASE_CPP_SDK_DIR})
prefixed_paths=()
for path in "${extraction_inclusions[@]}"; do
  prefixed_paths+=("${BASE_DIR_NAME}/${path}")
done
unzip ${ZIP_FILE} "${prefixed_paths[@]}" -d ..

echo "Cleaning up..."
rm ${ZIP_FILE}

popd >/dev/null

echo "Firebase C++ SDK setup complete."
