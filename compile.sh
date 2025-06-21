cd $HOME/workspace/c++

# Clean and create build directory
rm -rf build
mkdir build && cd build

# Configure with CMake
cmake -DENABLE_LOGGING=OFF ..

# Build the project
make -j16

## Set Up Model Environment

# Set the model path environment variable
echo 'export RTADP_MODEL_PATH="/home/worker/workspace/test/ml_models/float_16.tflite"' >> ~/.bashrc

# Reload the bash configuration
source ~/.bashrc

# Navigate to the test directory
cd $HOME/workspace/test
