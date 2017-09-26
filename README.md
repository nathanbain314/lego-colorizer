# lego-colorizer
![Earth.gif](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/Earth.gif)
This is a tool to color lego objects. It is mainly meant to map spherical equirectanuglar images to objects but can also build rings systems for planets and lego mosaics from photos.
# colorize
This will map an equirectangular spherical image to an LDraw object.
## options
- -i,  --input : The input LDraw file that will be used.
- -o, --output : The name of the output LDraw file to be created.
- -c, --color : A 64 megabyte binary file of the lego color id to use for each of the 2^24 RGB values. Two files are already provided as well as a tool to create more.
- -p, --picture : An equirectangular spherical image to color the lego object with.
- -l, --ldraw : LDraw directory to use for parts definitions.
## Tips
Use colors.dat in the data directory for the **-c** option. This was created using the [Delta E 2000 formula](https://en.wikipedia.org/wiki/Color_difference# CIEDE2000) and gives the most similar colors to RGB colors. rgb.dat uses the [Euclidean distance](https://en.wikipedia.org/wiki/Euclidean_distance) and produces much worse results, but I included it anyway.
The program colors around the origin, so you may need to move your object in an draw editor.
# color_image
Since there are only 68 colors in the lego palette that are valid colors for the sphere, some results can look much different from the input image. This will replace the colors of an image with the closest color from the lego palette. Can also be used to generate lego mosaics, both LDraw files and files of comma separated colors for building your own.
![Normal Color](https://color.adobe.com/build2.0.0-buildNo/resource/img/kuler/color_wheel_730.png)
![Lego Color](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/lego_color.png)
## options
- -i,  --input : The input image
- -o, --output : The output file. If the file ends in *.ldr* then a mosaic will be build in LDraw format composed of 1x1 plates in the matching color. If the file ends in *.txt* or *.csv* then that file will be filled with comma separated values representing the colors in the mosaic. Otherwise it will save it as an image with the pixels changed to the most similar color.
- -c, --color : A 64 megabyte binary file of the lego color id to use for each of the 2^24 RGB values.
- s, --shrink : The factor to shrink shrink the input image by.
# build_circle
Constructs and colors a ring made out of 1x1 plates. This is mostly useful for generating ring systems for planets.
## options
- -p,  --picture : A texture to map to the ring. This image should be oriented such that the left side is on the innermost side of the ring and the right side is on the outermost side of the ring.
- o, --output : The output ldraw file
- -l, --outer_radius : The outer radius of the circle, in blocks
- -s, --inner_radius : The inner radius of the circle, in blocks
- -c, --color : Binary file of color data
# better_sphere
[This webpage](http://lego.bldesign.org/sphere/) generates a high quality lego sphere made of 6 identical parts. However it is not build of valid parts nor even similar parts and can't be used in many editors. **better_sphere** will take this file as input and generate a hollow sphere made of valid parts. The sphere from the webpage must be generated with the *sphere* option turned off, and better quality spheres will be obtained with jumpers turned on.
## options
- -i,  --input : The input LDraw file.
- -o, --output : The output LDraw file.
- -s, --side : A number between 0 and 5 inclusive corresponding to which side of the sphere to use. 0 is top, 1 is bottom, 2-5 are the sides. Can be used multiple times. If no sides are specified all will be used by default.
# build_data
This program is mostly unnecessary but can be used if you wish to generate your own color data file. Just change the two arrays at the top or the innermost loop of the **build_color_data()** function. It is currently set up to use 68 colors and the Delta E 2000 formula for color comparison.
Run with ./build_data output.dat
# examples
![Large Earth](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/large_earth.png)
![Block Earth](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/block_earth.png)
![Death Star](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/death_star.png)
![Cube](https://raw.githubusercontent.com/nathanbain314/lego-colorizer/master/examples/cube_earth.png)
