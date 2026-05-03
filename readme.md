how to run program :

For ubantu
1. Update packages and install dependencies: 
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libopencv-dev

2. clone repo :
    git clone https://github.com/shauryaaamulik/Opencv_rockpaperscissor.git

3. Navigate into the project folder:
    cd Opencv_rockpaperscissor

4. Create a build directory and navigate into it:
    mkdir build && cd build

5. generate build files:
    cmake ..

6. Compile the code:
    make

7. Run code:
    ./HandGesture