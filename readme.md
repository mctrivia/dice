# Dice Face Optimizer

This app calculates the optimal positions of the faces on a non-standard die. It only works for dice with an even number of sides, and the faces will always land facing up.

## Table of Contents

- [Usage](#usage)
- [Visualization](#visualization)
- [Recommendations](#recommendations)
- [Saving Progress](#saving-progress)
- [License](#license)
- [Notes](#notes)
- [Contact](#contact)

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
