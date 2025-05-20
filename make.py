import subprocess
import os

def printCmakeError(result):
    print("output:")
    print(result.stdout)
    if result.returncode != 0:
        print("Command failed.")
        print("error:")
        print(result.returncode)
        print(result.stderr)
        exit(1)

source_dir = os.getcwd()
base_dir = os.path.join(source_dir, "base")
base_build_dir = os.path.join(source_dir, "base", "build")

# Create the build directory if it doesn't exist
if not os.path.exists(base_build_dir):
    os.makedirs(base_build_dir)

# Configure the project using CMake
configure_command = ["cmake", "-S", base_dir, "-B", base_build_dir]# -S选项用于指定源代码目录。
configure_result = subprocess.run(configure_command, capture_output=True, text=True)
printCmakeError(configure_result)
# Build the project using CMake
build_command = ["cmake", "--build", base_build_dir]
build_result = subprocess.run(build_command, capture_output=True, text=True)
printCmakeError(build_result)
# install
install_command = ["sudo", "cmake", "--install", base_build_dir]
install_result = subprocess.run(install_command, capture_output=True, text=True)
printCmakeError(install_result)

base_test_dir = os.path.join(source_dir, "base", "test")
base_test_build_dir = os.path.join(base_test_dir, "build")
# Configure the project using CMake
configure_command = ["cmake", "-S", base_test_dir, "-B", base_test_build_dir]# -S选项用于指定源代码目录。
configure_result = subprocess.run(configure_command, capture_output=True, text=True)
printCmakeError(configure_result)
# Build the project using CMake
build_command = ["cmake", "--build", base_test_build_dir]
build_result = subprocess.run(build_command, capture_output=True, text=True)
printCmakeError(build_result)
