#include "LegoColorizer.h"

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Reads an input LDraw file and maps equirectangular image to sphere using lego color pallette.", ' ', "1.0");


    ValueArg<double> inner_radiusArg( "s", "inner_radius", "Inner circle radius", false, 5.0, "double", cmd);

    ValueArg<double> outer_radiusArg( "r", "outer_radius", "Outer circle radius", false, 10.0, "double", cmd);

    ValueArg<double> gammaArg( "g", "gamma", "Gamma exponent", false, 1.0, "double", cmd );

    ValueArg<string> colorNameArg( "c", "colors", "Colors to use", false, "legoData/allColors.txt", "string", cmd);

    ValueArg<string> ldrawArg( "l", "ldraw_directory", "LDraw directory for parts", false, "none", "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Picture to map", false, "none", "string", cmd);

    ValueArg<string> outputArg( "o", "output", "Output LDraw file (.ldr)", false, "out.ldr", "string", cmd);

    ValueArg<string> inputArg( "i", "input", "Input LDraw file (.ldr)", false, "", "string", cmd);

    cmd.parse( argc, argv );

    string input_sphere     = inputArg.getValue();
    string output_sphere    = outputArg.getValue();
    string picture_file     = pictureArg.getValue();
    string colorName        = colorNameArg.getValue();
    string ldraw_directory  = ldrawArg.getValue();
    double gamma            = gammaArg.getValue();
    double inner_radius = inner_radiusArg.getValue();
    double outer_radius = outer_radiusArg.getValue();

    LegoColorizer( input_sphere, output_sphere, picture_file, colorName, ldraw_directory, gamma, inner_radius, outer_radius );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

  return 0;
}