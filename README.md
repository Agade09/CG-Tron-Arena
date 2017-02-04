# CG-Tron-Arena
Referee program to play games between AIs for the Tron game on Codingame. Linux only.
The code only works for 1v1 games but a lot was written with N player games in mind so adaptation to 1v2,1v3 games should be minor.

Usage:
* Compile the Arena program
* Compile two of your AIs
* Run the Arena program with the names of the AI binaries as command line parameters. e.g: Arena V13 V12
* Optional: Specify the number of threads as a command line parameter. e.g: Arena V13 V12 2
* Optional: Adjust fairness of the spawns via the "constexpr double asymetry_limit" variable. 0.50 corresponds to random spawns and 0.01 corresponds to, at worst, starting with 49% of the map according to a basic Voronoi evaluation
* Optional: Set timeout behavior on or off via the "constexpr bool Timeout" variable. This can be useful as I've noticed timeouts if the computer is being used for something else.


