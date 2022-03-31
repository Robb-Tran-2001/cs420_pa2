Subject 	: CSCI420 - Computer Graphics 
Assignment 2: Simulating a Roller Coaster
Author		: Robb Tran
USC ID 		: 9095109299

Description: In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

Core Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)

======================

1. Uses OpenGL core profile, version 3.2 or higher - Y

2. Completed all Levels:
  Level 1 : - Y
  level 2 : - Y
  Level 3 : - Y
  Level 4 : - Y
  Level 5 : - Y

3. Rendered the camera at a reasonable speed in a continuous path/orientation - Y

4. Run at interactive frame rate (>15fps at 1280 x 720) - Y

5. Understandably written, well commented code - Y

6. Attached an Animation folder containing not more than 1000 screenshots - Y

7. Attached this ReadMe File - Y

Extra Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Render a T-shaped rail cross section - N

2. Render a Double Rail - Y

3. Made the track circular and closed it with C1 continuity - N

4. Any Additional Scene Elements? (list them here) - N

5. Render a sky-box - Y

6. Create tracks that mimic real world roller coaster - N

7. Generate track from several sequences of splines - Y

8. Draw splines using recursive subdivision - Y

9. Render environment in a better manner - Y

10. Improved coaster normals - Y

11. Modify velocity with which the camera moves - N

12. Derive the steps that lead to the physically realistic equation of updating u - N

Additional Features: (Please document any additional features you may have implemented other than the ones described above)
1. Add option to shuffle the splines to randomly generate track if only 3 arguments provided
usage: ./hw2 [track_list_file] [anything_here] (e.g. ./hw2 alltracks.txt 123)
2. 
3.

Open-Ended Problems: (Please document approaches to any open-ended problems that you have tackled)
1. Adding texture to rails in addition to Phong shading
2.
3.

Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
1. Rotate x, y via the left mouse button. Rotate z via middle mouse button.
2. 
3.

Names of the .cpp files you made changes to:
1. hw2.cpp
2.
3.

Comments : (If any)
Developed on MacOS
Note that when ./hw2 alltracks.txt is used, some splines overlap.
Some nice tracks: ./hw2 track.txt or ./hw2 track2.txt
Modified Makefile to compile with c++11. Run make to compile, make clean to clean.