## CG-Tron-Arena
Referee program to play games between AIs for the Tron game on Codingame. **Linux only**.

# Usage:
* Compile the Arena program
* Compile two of your AIs
* Run the Arena program with the names of the AI binaries as command line parameters. e.g: Arena V13 V12

#Optional:
* Change constexpr int N{2} to any number for 1v2/1v3 games. The Arena will then pit the first AI against N-1 copies of the second.
* Specify the number of threads as a command line parameter. e.g: Arena V13 V12 2
* Adjust fairness of the spawns via the "constexpr double asymetry_limit" variable. 0.50 corresponds to random spawns and 0.01 corresponds to, at worst, starting with 49% of the map according to a basic Voronoi evaluation
* Set timeout behavior on or off via the "constexpr bool Timeout" variable. This can be useful as I've noticed timeouts if the computer is being used for something else.


