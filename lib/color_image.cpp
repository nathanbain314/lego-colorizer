#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vips/vips8>
#include <tclap/CmdLine.h>

#include "progressbar.h"

using namespace TCLAP;
using namespace vips;
using namespace std;

const int color_codes[68] =
{
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  13,
  14,
  15,
  17,
  18,
  19,
  20,
  22,
  23,
  25,
  26,
  27,
  28,
  29,
  68,
  69,
  70,
  71,
  72,
  73,
  74,
  77,
  78,
  85,
  86,
  89,
  92,
  100,
  110,
  112,
  115,
  118,
  120,
  125,
  151,
  191,
  212,
  216,
  226,
  232,
  272,
  288,
  308,
  313,
  320,
  321,
  323,
  335,
  351,
  373,
  378,
  379,
  450,
  462,
  484,
  503
};

const int color_values[68][3] =
{
  {5,19,29},
  {0,85,191},
  {37,122,62},
  {0,131,143},
  {201,26,9},
  {200,112,160},
  {88,57,39},
  {155,161,157},
  {109,110,92},
  {180,210,227},
  {75,159,74},
  {85,165,175},
  {252,151,172},
  {242,205,55},
  {255,255,255},
  {194,218,184},
  {251,230,150},
  {228,205,158},
  {201,202,226},
  {129,0,123},
  {32,50,176},
  {254,138,24},
  {146,57,120},
  {187,233,11},
  {149,138,115},
  {228,173,200},
  {243,207,155},
  {205,98,152},
  {88,42,18},
  {160,165,169},
  {108,110,104},
  {92,157,209},
  {115,220,161},
  {254,204,207},
  {246,215,179},
  {63,54,145},
  {124,80,58},
  {76,97,219},
  {208,145,104},
  {254,186,189},
  {67,84,163},
  {104,116,202},
  {199,210,60},
  {179,215,209},
  {217,228,167},
  {249,186,97},
  {230,227,224},
  {248,187,61},
  {134,193,225},
  {179,16,4},
  {255,240,58},
  {86,190,214},
  {13,50,91},
  {24,70,50},
  {53,33,0},
  {84,169,200},
  {114,14,15},
  {20,152,215},
  {189,220,216},
  {214,117,114},
  {247,133,177},
  {132,94,132},
  {160,188,172},
  {89,113,132},
  {182,123,80},
  {255,167,11},
  {169,85,0},
  {230,227,218}
};

void color_image( VImage image, int (*color_data)[256][256], string output_file )
{
  unsigned char * data = (unsigned char *)image.data();

  int width = image.width();
  int height = image.height();

  progressbar *color_bar = progressbar_new("Processing image", height);

  string filetype = output_file.substr( output_file.find_last_of(".")+1 );

  if( filetype == "ldr" )
  {
    ofstream output( output_file );
    for( int i = 0, p = 0; i < height; ++i)
    {
      for( int j = 0; j < width; ++j, p+=3)
      {
        output << " 1 ";
        output << color_data[data[p]][data[p+1]][data[p+2]];
        output << " " << j * 20 << " 0 " << i * -20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
      }
      progressbar_inc( color_bar );
    }
    output.close();
  }
  else if( filetype == "csv" || filetype == "txt" )
  {
    ofstream output( output_file );
    for( int i = 0, p = 0; i < height; ++i)
    {
      for( int j = 0; j < width; ++j, p+=3)
      {
        output << color_data[data[p]][data[p+1]][data[p+2]] << ",";
      }
      output << endl;
      progressbar_inc( color_bar );
    }
    output.close();
  }
  else
  {
    for( int i = 0, p = 0; i < height; ++i)
    {
      for( int j = 0; j < width; ++j, p+=3)
      {
        const int* values = color_values[ find( color_codes, color_codes+68, color_data[data[p]][data[p+1]][data[p+2]]) - color_codes];
        data[p]   = (unsigned char)values[0];
        data[p+1] = (unsigned char)values[1];
        data[p+2] = (unsigned char)values[2];
      }
      progressbar_inc( color_bar );
    }
    image.vipssave( (char *)output_file.c_str() );
  }

  progressbar_finish(color_bar);
}

int main( int argc, char** argv)
{
  try
  {
    CmdLine cmd("Reads an imput image file and changes the colors to the most similar valid lego color. Can output to a comma separated value file or text file of color codes, generate a lego mosaic in LDraw format, or render to an image file.", ' ', "1.0");

    ValueArg<int> shrinkArg( "s", "shrink", "Shrink amount", false, 1, "int");
    cmd.add( shrinkArg );

    ValueArg<string> colorArg( "c", "color", "Color data", true, "data/colors.dat", "string");
    cmd.add( colorArg );

    ValueArg<string> outputArg( "o", "output", "Output file (.ldr, .csv, .png, ect.)", true, "out.png", "string");
    cmd.add( outputArg );

    ValueArg<string> inputArg( "i", "input", "Input image file (.png, .jpg, ect.)", true, "in.png", "string");
    cmd.add( inputArg );

    cmd.parse( argc, argv );

    string input_file = inputArg.getValue();
    string output_file = outputArg.getValue();
    string color_file = colorArg.getValue();
    int shrink = shrinkArg.getValue();

    if( vips_init( argv[0] ) )
      vips_error_exit( NULL ); 
    vips_cache_set_max(0);

    VImage input = VImage::vipsload( (char *)input_file.c_str() ).shrink( shrink, shrink);

    int (*color_data)[256][256] = new int[256][256][256];
    ifstream input_data( color_file, ios::binary );
    input_data.read( (char *)color_data, 1<<26 );
    input_data.close();

    cout << input_file << " " << output_file << " " << color_file << " " << shrink << endl;

    color_image( input, color_data, output_file);

    vips_shutdown();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

}