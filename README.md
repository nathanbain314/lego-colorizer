
# lego-colorizer
![Earth.gif](http://nathanbain.com/wikiImages/RunLegoColorizer/Earth.gif)

This is a tool to color lego objects. It is mainly meant to map spherical equirectangular images to objects but can also build rings systems for planets.
# RunLegoColorize
This will map an equirectangular spherical image to an LDraw object.
## options
- -i,  --input : The input LDraw file (.ldr) that will be used.
- -o, --output : The name of the output LDraw file (.ldr) to be created.
- -p, --picture : An equirectangular spherical image to color the lego sphere with, or a texture of a ring system.
- -l, --ldraw : LDraw directory to use for parts definitions.
- -c, --color : Colors to use. There are 68 colors in the lego palette, but you can limit it to any set of colors by creating a text file with the color codes to use.
- -g, --gamma : Gamma exponent for gamma correction
- r, --outer_radius : Outer radius for generating ring system
- s, --inner_radius : Inner radius for generating ring system
## Tips
The program colors around the origin of the object, so you may need to move your object in an editor. Since there are only 68 colors in the lego palette that are valid colors for the sphere, some results can look much different from the input image. If the colors don't look correct, using a gamma value can create deeper colors.
If no input file is added then a ring system made out of 1x1 plates will be generated. Instead of using an equirectangular texture a horizontal ring texture should be provided.
# examples
![Large Earth](http://nathanbain.com/wikiImages/RunLegoColorizer/large_earth.png)
![Outward Earth](http://nathanbain.com/wikiImages/RunLegoColorizer/outward_earth.png)
![Plate Earth](http://nathanbain.com/wikiImages/RunLegoColorizer/plate_earth.png)
