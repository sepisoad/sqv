# wtf is posenum

- ok i am dealing with mdl files
- it stores character animations as a series of frames
- each frame contains a series of vertices
- quake 1 mdl file does not support skeletal animation
- so everything is described as a series of vertices (i think)
- also (i think) each mdl file contains a bunch of poses
- the concept of pose is a bit confusing to me
- i thought it referred to the number of distict animations for a character
- for example standing animation, or running or jumping, or dying ...
- quake uses a lot of global variables all over the place
- during model loading it sets 'posenum' global variabl to '0'
- also there are two frame types: single, group
