# Dice Face Optimizer

This app calculates the optimal positions of the faces on a non-standard die. It only works for dice with an even number of sides, and the faces will always land facing up.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Compilation Instructions](#compilation-instructions)
    - [Linux](#linux)
    - [Windows](#windows)
    - [macOS](#macos)
- [Usage](#usage)
- [Visualization](#visualization)
- [Recommendations](#recommendations)
- [Saving Progress](#saving-progress)
- [License](#license)
- [Notes](#notes)
- [Contact](#contact)

## Prerequisites

- **C++ Compiler** supporting C++17
- **CMake** (version 3.25 or higher)
- **Qt5** (Core, Gui, Widgets modules)

    - **Linux:** Install via package manager or [Qt Installer](https://www.qt.io/download)
    - **Windows:** Install via [Qt Installer](https://www.qt.io/download)
    - **macOS:** Install via [Qt Installer](https://www.qt.io/download) or [Homebrew](https://brew.sh/) (with [Homebrew Cask](https://formulae.brew.sh/cask/qt))

## Compilation Instructions

### Linux

1. **Install Dependencies**

   Ensure you have `build-essential`, `cmake`, and Qt5 installed.

   ```bash
   sudo apt-get update
   sudo apt-get install build-essential cmake qt5-default
   ```

2. **Clone the Repository**

   ```bash
   git clone https://github.com/mctrivia/dice.git
   cd dice
   ```

3. **Create Build Directory**

   ```bash
   mkdir build
   cd build
   ```

4. **Generate Makefiles with CMake**

   ```bash
   cmake ..
   ```

    - If Qt is installed in a non-standard location, specify the Qt prefix path:

      ```bash
      cmake -DCMAKE_PREFIX_PATH=/path/to/Qt5 ..
      ```

5. **Build the Project**

   ```bash
   make
   mv dice ..
   cd ..
   ```

6. **Run the Program**

   ```bash
   ./dice
   ```

### Windows

1. **Install Dependencies**

    - **CMake:**
        - Download and install from the [CMake Official Website](https://cmake.org/download/).
        - Ensure CMake is added to your system `PATH` during installation.

    - **Qt5:**
        - Download the Qt installer from the [Qt Official Website](https://www.qt.io/download).
        - During installation, select the components matching your compiler (e.g., MSVC 2019, MinGW).

    - **Visual Studio:**
        - Install [Visual Studio](https://visualstudio.microsoft.com/) with **Desktop development with C++** workload.

2. **Clone the Repository**

   ```powershell
   git clone https://github.com/mctrivia/dice.git
   cd dice
   ```

3. **Create Build Directory**

   Open **Command Prompt** or **PowerShell** and navigate to the project directory:

   ```powershell
   mkdir build
   cd build
   ```

4. **Generate Visual Studio Solution with CMake**

   ```powershell
   cmake .. -G "Visual Studio 16 2019" -A x64
   ```

    - **Notes:**
        - Replace `"Visual Studio 16 2019"` with your installed version (e.g., `"Visual Studio 17 2022"`).
        - Use `-A x64` for 64-bit builds or `-A Win32` for 32-bit.

    - If Qt is installed in a non-standard location, specify the Qt prefix path:

      ```powershell
      cmake -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64" .. -G "Visual Studio 16 2019" -A x64
      ```

5. **Build the Project**

    - Open the generated `.sln` file in **Visual Studio**.
    - Select the desired build configuration (**Debug** or **Release**).
    - Build the solution:
        - Go to `Build` > `Build Solution` or press `Ctrl + Shift + B`.

6. **Run the Program**

    - After a successful build, run the executable directly from Visual Studio:
        - `Debug` builds are typically located in `build/Debug/`.
        - `Release` builds are typically located in `build/Release/`.

    - Alternatively, navigate to the build directory and execute `dice.exe`.

### macOS

1. **Install Dependencies**

    - **Homebrew:** If you don't have Homebrew installed, install it from [https://brew.sh/](https://brew.sh/).

    - **CMake and Qt5:**

      ```bash
      brew update
      brew install cmake qt5
      ```

      > **Note:** After installation, you might need to link Qt5 if CMake doesn't find it automatically.

      ```bash
      brew link qt5 --force
      ```

        - Alternatively, specify the Qt prefix path when configuring with CMake.

2. **Clone the Repository**

   ```bash
   git clone https://github.com/mctrivia/dice.git
   cd dice
   ```

3. **Create Build Directory**

   ```bash
   mkdir build
   cd build
   ```

4. **Generate Makefiles with CMake**

   ```bash
   cmake ..
   ```

    - If Qt is installed via Homebrew and not linked, specify the Qt prefix path:

      ```bash
      cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..
      ```

5. **Build the Project**

   ```bash
   make
   mv dice ..
   cd ..
   ```

6. **Run the Program**

   ```bash
   ./dice
   ```
   
## Usage

When you run the program, it will prompt you to enter the number of sides you want to design.

After entering the desired number of sides, the program will start generating the optimal positions for the faces.

## Visualization

- **Three Spheres:** The program displays three spheres representing the die from three different angles, each 90 degrees apart. This helps you visualize the face placements from multiple perspectives.
- **Stress Highlighted:** The most stressed face is red, and the least stressed is green.  The color will fade between these depending on how stressed they are.  A good final result should look like random coloring it should not be green in one area and red in another.
- **Timer Display:** A timer shows how long it has been since a better arrangement was found.

## Recommendations

- **Run Time:** For optimal results, it is recommended to run the app until the timer reaches **86,400 seconds (1 day)**.
- **Uniform Spacing:** Do not use the die until the face spacing appears uniformly distributed.
- **Processing Time:** The optimization process takes longer with an increasing number of sides. Please be patient.
- **Threading:** The program uses five separate threads to optimize the shape. Visualizations may update asynchronously as different threads find better configurations.

## Saving Progress

- The best arrangement is automatically saved every 10 seconds.
- You can safely shut down your computer if needed; progress will be retained.

## License

You are free to use this app for personal or business purposes. If you make any profit from products designed with the help of this app, you are required to send **5% of the profits** to **Matthew Cornelisse**.

- **Payment Methods:** PayPal or Interac e-Transfer
- **Email:** [squarerootofnegativeone@gmail.com](mailto:squarerootofnegativeone@gmail.com)

## Notes

- The visualization may appear to jump or update irregularly due to the multi-threaded optimization process.
- The three displayed views are sufficient because the die's symmetry makes the other three views identical.
- Ensure that the face positions look reasonable before finalizing your die design.

## Contact

For questions, support, or more information, please contact **Matthew Cornelisse**.

- **Email:** [squarerootofnegativeone@gmail.com](mailto:squarerootofnegativeone@gmail.com)

---

Feel free to contribute or suggest improvements!
