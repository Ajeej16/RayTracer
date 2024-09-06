# Ray Tracing Project

## Introduction
The is a simple implementation of a multi-hit ray tracer, utilizing c and glsl.
This ray tracer is able to render spheres and meshes with specified material
properties. The renderer supports diffuse and specular lighting.

## Author
**Alejandro Fernandez**
- Contact: alejandr.fernand@ufl.edu
- GitHub: ajeejuf

## How to Build
Follow these steps to build and set up the ray tracing project.

### Prerequisites
- [List any prerequisites users need to install before building the project.]

### Build Instructions
1. Clone the repository: `git clone [repository URL]`
2. Navigate to the project directory: `cd RayTracer`
3. Navigate to the project scripts: `cd scripts`
4. Run the batch script `.\build.bat`
5. Navigate to build directory and run exexutable or run debug script to
open in Visual Studios.

Note: Read through the build batch file and make sure that the path the your Visual Studios directory is the same. If you do not have Visual Studios, then install it. If
the path is different, please modify the path in the batch file in order for you to run the vcvars64.bat script.

## Controls
Understand how to interact with the ray tracing application using the following controls.

### Keyboard Controls
- **W, A, S, D:** Move forward, left, backward, and right.
- **M:** Render the current frame (cannot move in this state).
- **R:** Start and end recording frames.
- **P:** Start and end playing frames.
- **V:** Start uploading recording to video file.

### Mouse Controls
- **Movement:** Rotates the camera (recommend pressing M to stop movement and centering the mouse on the screen and then press M again to initiate movement).