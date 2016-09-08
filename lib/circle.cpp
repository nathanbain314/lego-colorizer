#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <Magick++.h>
#include <tclap/CmdLine.h>

using namespace TCLAP;
using namespace Magick;
using namespace std;

int closest_color( PixelPacket *pixels, int width, int height, int (*color_data)[256][256]) //{return 0;};
{
  map< int, int > counter;

  for( int i = 0; i < height; ++i)
  {
    for( int j = 0; j < width; ++j)
    {
      Color color = pixels[i*width+j];

      int r = color.redQuantum()/256;
      int g = color.greenQuantum()/256;
      int b = color.blueQuantum()/256;
      counter[color_data[r][g][b]]+=(65535-color.alphaQuantum());
    }
  }
  auto x = max_element( counter.begin(), counter.end(),
    [](const pair<int, int>& p1, const pair<int, int>& p2) {
        return p1.second < p2.second; });
  return x->first;
}

bool valid_point( double x, double y, double inner_radius, double outer_radius)
{
  double offset = ( x > 0 ) ? -.5 : .5;
  bool valid_point = ( (x-offset) * (x-offset) + (y-.5) * (y-.5) >= inner_radius * inner_radius ) &&
                     ( (x+offset) * (x+offset) + (y+.5) * (y+.5) <= outer_radius * outer_radius );
  return valid_point;
}

string create_circle( double inner_radius, double outer_radius, Image image, int (*color_data)[256][256] )
{
  stringstream output;

  double width = image.columns();

  Image image2 = image;

  double height = image.rows();

  int size = width / ( outer_radius - inner_radius + 1 );

  double mid = height / 2;
  int count = 0;

  for( double x = .5-(double)inner_radius / sqrt(2); x < 0; ++x)
  {
    for( double y = outer_radius*2; y > -1; --y)
    {
      double angle = atan2( y/2, x);
      if( valid_point( x, y/2, inner_radius - .5*sin( angle ), outer_radius + .5*sin( angle ) ) )
      {
        double start = int(sqrt(x*x+y*y/4)-inner_radius+ .5*sin( angle ));

        Pixels view( image );
        double w = (size*(start+1) < width) ? size : width - size*start;
        PixelPacket *pixels = view.get( (size * start<width-size/2) ? size*start : width-size/2 , 0, w, height);

        int color = closest_color( pixels, w, height, color_data);

        if( color != 0)
        {
          output << " 1 " << color << " " << (int)x*20 << " -4 " << (int)y*10 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << (int)x*20 << " -4 " << -(int)y*10 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << (int)y*10 << " -4 " << (int)x*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << -(int)y*10 << " -4 " << (int)x*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          if( x < -1)
          {
            output << " 1 " << color << " " << (int)x*-20 << " -4 " << (int)y*10 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
            output << " 1 " << color << " " << (int)x*-20 << " -4 " << -(int)y*10 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
            output << " 1 " << color << " " << (int)y*10 << " -4 " << (int)x*-20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
            output << " 1 " << color << " " << -(int)y*10 << " -4 " << (int)x*-20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          }
        }
        --y;
      }
    }
  }

  for( double x = inner_radius / sqrt(2)+.5; x < outer_radius+2; ++x)
  {
    for( double y = inner_radius / sqrt(2)+.5; y < outer_radius+2; ++y)
    {
      double angle = atan2( y, x);
      if( valid_point( x, y, inner_radius- .5*sin( angle ), outer_radius+ .5*sin( angle )) )
      {
        double start = int(sqrt(x*x+y*y)-inner_radius+.5*sin( angle ));

        Pixels view( image );
        double w = (size*(start+1) < width) ? size : width - size*start;
        PixelPacket *pixels = view.get( (size * start<width-size/2) ? size*start : width-size/2 , 0, w, height);

        int color = closest_color( pixels, w, height, color_data);

        if( color != 0)
        {
          output << " 1 " << color << " " << (int)x*20 << " -4 " << (int)y*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << (int)x*20 << " -4 " << -(int)y*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << -(int)x*20 << " -4 " << (int)y*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
          output << " 1 " << color << " " << -(int)x*20 << " -4 " << -(int)y*20 << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
        }
      }
    }
  }
  
  return output.str();
}

int main( int argc, char** argv)
{
  try
  {
    CmdLine cmd("Reads an imput image file and changes the colors to the most similar valid lego color. Can output to a comma separated value file or text file of color codes, generate a lego mosaic in LDraw format, or render to an image file.", ' ', "1.0");

    ValueArg<string> colorArg( "c", "color", "Color data", true, "data/colors.dat", "string");
    cmd.add( colorArg );

    ValueArg<double> inner_radiusArg( "s", "inner_radius", "Inner circle radius", true, 10.0, "double");
    cmd.add( inner_radiusArg );

    ValueArg<double> outer_radiusArg( "l", "outer_radius", "Outer circle radius", true, 10.0, "double");
    cmd.add( outer_radiusArg );

    ValueArg<string> outputArg( "o", "output", "Output file (.ldr, .csv, .png, ect.)", true, "out.png", "string");
    cmd.add( outputArg );

    ValueArg<string> pictureArg( "p", "picture", "Input image file (.png, .jpg, ect.)", true, "in.png", "string");
    cmd.add( pictureArg );

    cmd.parse( argc, argv );

    string picture_file = pictureArg.getValue();
    string output_file = outputArg.getValue();
    double inner_radius = inner_radiusArg.getValue();
    double outer_radius = outer_radiusArg.getValue();
    string color_file = colorArg.getValue();

    Image image;
    image.read(picture_file);

    int (*color_data)[256][256] = new int[256][256][256];
    ifstream input_data( color_file, ios::binary );
    input_data.read( (char *)color_data, 1<<26 );
    input_data.close();

    ofstream output_circle( output_file );

    output_circle << create_circle( inner_radius, outer_radius, image, color_data );
    //output_circle << " 1 110 0 0 0 1 0 0 0 1 0 0 0 1 30039.dat" << endl;

    output_circle.close();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

  return 0;
}