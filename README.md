# CPSC 453 HW3

Benjamin Thomas, 10125343, T04

## Building/Running

In order to build and run the model viewer, follow these steps:

1. Make a new directory in which to run the build
2. Run CMake in the build directory to generate the Makefile: `cmake <project root>`
    - You should strongly consider adding `-DCMAKE_CXX_FLAGS="-O2"` to your call to CMake for
      performance reasons, especially if you'll be rendering a complex scene
3. Initiate the build process by running `make` in the build directory
4. If you'll be using any of the included scenes, download and extract all models from D2L in
   subdirectories under the `scenes/models` directory (create this directory if it does not exist)
5. Run the executable `hw3` in the build directory with the path to the scene file to display as its
   only command line argument (e.g. `./hw3 ../scenes/chessboard.scn`)

## Controls

The model viewer supports the following viewing controls:

- Click and drag to orbit the camera around the current focus point
- Right-click and drag to pan the focus point and camera
- Use the mouse wheel to move the camera closer to or further from the focus point
- Press R to switch render mode between standard, full brightness, and normals
- Press B to show/hide bounding boxes
    - Red bounding boxes are aligned to object axes
    - Blue bounding boxes are aligned to world axes
    - Green bounding box encompasses the entire scene, aligned to world axes
- Press L to show/hide lights
- Press T to enable/disable textures
- Press O to enable/disable ambient occlusion
- Press C to reset the camera to show the entire scene
- Press H to show/hide help text

Additionally, the model viewer can be used to perform simple scene editing. Pressing Tab and
Shift+Tab will select an object. The currently selected object is marked by a white bounding box.
This object can then be manipulated using the following controls:

- Use WASDQE to translate the object around the scene
- Hold shift and use WASDQE to modify the object's orientation (yaw-pitch-roll) about its origin
- Use Z and X to scale the object about its origin
- Use I and K to adjust the editing speed
- Press P to print object positioning information to standard output (useful for modifying scene
  files)

## Scene Files

In order to render a scene, the model viewer must be given a scene file. This is a simple text file
describing the layout of the scene. A non-blank line with no indentation invokes a command; each
command can have any number of extra attributes associated with it, so long as each is on its own
line and all attributes belonging to a given command have the same indentation.

All RGB values range from 0 (no illumination) to 1 (full illumination). All paths are relative to
the directory the scene file is in. Most attributes have sane defaults that will be used if the
attribute is not specified.

The following commands are available:

- The `mdl <name> <obj file>` command loads the given OBJ file into a model with the given name
- The `mtl <name>` command defines a new material with the given name.
    - The `ambient <r> <g> <b>` attribute defines the ambient reflectivity
    - The `diffuse <r> <g> <b>` attribute defines the diffuse reflectivity
    - The `specular <r> <g> <b>` attribute defines the specular reflectivity
    - The `shininess <value>` attribute defines the shininess exponent
    - The `diffuse_map <image>` attribute defines the diffuse reflectivity map
    - The `specular_map <image>` attribute defines the specular reflectivity map
    - The `ao_map <image>` attribute defines the ambient occlusion map
- The `alight <r> <g> <b>` command defines the ambient lighting of the scene
- The `plight` attribute defines a new point light (max 16 per scene)
    - The `pos <x> <y> <z>` attribute defines the position of the light within the scene
    - The `ambient <r> <g> <b>` attribute defines the ambient light intensity/colour
    - The `diffuse <r> <g> <b>` attribute defines the diffuse light intensity/colour
    - The `specular <r> <g> <b>` attribute defines the specular light intensity/colour
    - The `atten <a0> <a1> <a2>` attribute defines the attenuation coefficients
        - Light intensity is multiplied by `1 / (a0 + a1 * d + a2 * d * d)` where `d` is the
          distance to the light
- The `obj` command defines a new object in the scene+
    - The `mdl <model name>` attribute defines what model this object will use (required)
    - The `mtl <material name>` attribute defines what material this object will use
    - The `pos <x> <y> <z>` attribute defines the position of the object within the scene
    - The `rot <yaw> <pitch> <roll>` attribute defines the orientation of the object (rotations are
      performed about the object's origin)
    - The `scale <scale factor>` attribute defines the object's scale factor (scaling is performed
      about the object's origin)

When editing an object in a scene in the model viewer, pressing P will print the object's `pos`,
`rot`, and `scale` attributes so that they can be easily copied into the scene.

### Included Scenes

The following example scene files have been included in the `scenes` directory:

- `coffee_cup.scn` has a single coffee cup with a single light to demonstrate specular reflection
- `knight.scn` has a single chess knight with two lights of different colours on opposite sides of
  the knight to demonstrate multiple point lights
- `chessboard.scn` has a chessboard and full chess set placed atop an oak table with two oak chairs,
  with more complex lighting to show off the abilities of the model viewer

As noted previously, these example scene files require all of the models from D2L to be downloaded
and extracted in subdirectories of the `scenes/models` directory.

## Limitations

The model viewer has several limitations:

- At most 16 point lights can be present in a scene
- All OBJ models must use only traingular faces, and each face is required to have a 3d position,
  2d texture coordinates, and a properly normalized 3d normal
- Each OBJ model can only use one material

## Resources Used

- [OBJ file specification](http://paulbourke.net/dataformats/obj/)
- [Basic Phong Shading](http://www.opengl-tutorial.org/beginners-tutorials/tutorial-8-basic-shading/)
- [Arcball Rotation](https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball)
- [OpenGL Reference](https://www.khronos.org/registry/OpenGL-Refpages/gl4/)
- [GLFW3 Documentation](http://www.glfw.org/docs/latest/)
- [GLM Manual](http://glm.g-truc.net/glm.pdf)
- [GLM API Reference](https://glm.g-truc.net/0.9.4/api/index.html)
- [FreeType Documentation](https://www.freetype.org/freetype2/docs/documentation.html)
- [Fontconfig Documentation](https://www.freedesktop.org/wiki/Software/fontconfig/)
- Lots of code copied and adapted from HW1 and HW2
