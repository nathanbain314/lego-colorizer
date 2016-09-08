#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <cfloat>
#include <Magick++.h>
#include <tclap/CmdLine.h>

#include "progressbar.h"

using namespace TCLAP;
using namespace std;
using namespace Magick;

// Takes x, y, and z coordinates of point in sphere, and the width and height of the image.
// Returns the cooresponding point for an equirectangular image.
Coordinate equi_project( double x, double y, double z, double width, double height)
{
  double r = sqrt( x * x + y * y + z * z );
  double lat = acos( z / r )-M_PI/2;
  double lng = atan2( y, x );

  return Coordinate( ((lng + M_PI) * width / (2*M_PI)), ((M_PI/2 - lat) * height / M_PI) );
}

// Takes array of pixels, width and height of the array, and array of color best matching colors for all rgb values.
// Returns the most common matching color for that group of pixels.
int closest_color( PixelPacket *pixels, int width, int height, int (*color_data)[256][256])
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

// Checks if the shortest line between two coordinates goes across a border
bool across_border( Coordinate &c1, Coordinate &c2, double &width )
{
  double x1 = c1.x();
  double y1 = c1.y();
  double x2 = c2.x();
  double y2 = c2.y();

  if( x2 < x1)
  {
    double temp = x2;
    x2 = x1;
    x1 = temp;
    temp = y2;
    y2 = y1;
    y1 = temp;
  }
  bool across = (x1-x2+width) * (x1-x2+width) < (x1-x2) * (x1-x2);
  return across;
}

// Returns the y value of the point at which the lines bewteen c1 and c2 crosses the border of an image with the input width
double cross_height( Coordinate c1, Coordinate c2, double width )
{
  double x1 = c1.x();
  double y1 = c1.y();
  double x2 = c2.x();
  double y2 = c2.y();
  double m = (y2-y1) / (x2-x1);
  return m*(width-x2)+y2;
}

pair< vector< double >, string > parse_line( string s)
{
  vector< double > data;
  stringstream line;
  string l;

  if( s.find_first_of("134",0) != string::npos ) 
  {
    line << s;
    do
    {
      getline( line, l, ' ');
    }
    while( l.empty() );
    data.push_back( atof(l.c_str()) );
  }
  else
  {
    data.push_back( 0 );
  }

  switch( int(data[0]) )
  {
    case 1:
      for( int i = 0; i < 13; ++i)
      {
        do{ getline( line, l, ' '); }while( l.empty() );
        data.push_back( atof(l.c_str()) );
      }
      do{ getline( line, l, ' '); }while( l.empty() );
      break;

    case 3:
      for( int i = 0; i < 10; ++i)
      {
        do{ getline( line, l, ' '); }while( l.empty() );
        data.push_back( atof(l.c_str()) );
      }
      break;

    case 4:
      for( int i = 0; i < 13; ++i)
      {
        do{ getline( line, l, ' '); }while( l.empty() );
        data.push_back( atof(l.c_str()) );
      }
      break;
  }

  return pair< vector< double >, string >( data, l );
}

vector< double > draw_on_mask( vector< double > data, vector< double > rotation, vector< double > pos, Image &mask)
{
  //for (vector<double>::const_iterator i = data.begin(); i != data.end(); ++i)
  //  cout << *i << ' ';
  //cout << endl;

  double width = mask.columns();
  double height = mask.rows();

  int size = int(data.size()/3);

  double x_avg = 0;
  vector<Coordinate> points;
  for(int i = 0; i < size; ++i)
  {
    double _x = pos[0] + data[3*i+0] * rotation[0] + data[3*i+1] * rotation[1] + data[3*i+2] * rotation[2];
    double _y = pos[1] + data[3*i+0] * rotation[3] + data[3*i+1] * rotation[4] + data[3*i+2] * rotation[5];
    double _z = pos[2] + data[3*i+0] * rotation[6] + data[3*i+1] * rotation[7] + data[3*i+2] * rotation[8];

    points.push_back(equi_project( _x, _z, _y, width, height));
    x_avg += ( points[i].x() > width/2 );
  }

  x_avg/=(size);

  vector< double > bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
  for(int i = 0; i < size; ++i)
  {
    if( abs( points[i].x() - width ) < .001 && x_avg < .5)
    {
      points[i] = Coordinate( 0, points[i].y());
    }
    if( points[i].x() < bounds[0] ) bounds[0] = points[i].x();
    if( points[i].y() < bounds[1] ) bounds[1] = points[i].y();
    if( points[i].x() > bounds[2] ) bounds[2] = points[i].x();
    if( points[i].y() > bounds[3] ) bounds[3] = points[i].y();
  }

  bool across = false;
  for( int i = 0; i < size; ++i)
  {
    if( across_border( points[i], points[(i+1)%size], width ) )
    {
      across = true;
      break;
    }
  }
  if( across )
  {
    list< Coordinate > left, right;
    for( int i = 0; i < size; ++i)
    {
      if( points[i].x() < width/2 )
      {
        left.push_back( points[i] );
      }
      else
      {
        right.push_back( points[i] );
      }
      if( across_border( points[i], points[(i+1)%size], width ))
      {
        double y3 = cross_height( points[i], points[(i+1)%size], width );
        left.push_back( Coordinate( 0, y3 ) );
        right.push_back( Coordinate( width-1, y3 ) );
      }
    }
    mask.draw( DrawablePolygon( left ) );
    mask.draw( DrawablePolygon( right ) );
  }
  else if( abs( pos[0] ) < .001 && abs( pos[2] ) < .001 )
  {
    bounds[0] = 0;
    bounds[2] = width;
    double y3 = points[0].y();
    double y4 = height * round( y3 / height );
    bounds[0] = 0;
    bounds[2] = width;

    if( y4 < bounds[1] ) bounds[1] = y4;
    if( y4 > bounds[3] ) bounds[3] = y4;

    mask.draw( DrawablePolygon( { Coordinate( 0, y3), Coordinate( width, y3), Coordinate( width, y4 ), Coordinate( 0, y4 ) } ) );
  }
  else
  {
    list<Coordinate> coord;
    for( int i = 0; i < size; ++i) coord.push_back( points[i] );
    mask.draw( DrawablePolygon( coord ) );
  }
  return bounds;
}

vector< double > generate_mask( string part, string ldraw_directory, vector< double > rotation, vector< double > pos, Image &mask )
{
  ifstream file( string( ldraw_directory ).append( "parts/" ).append( part ) );
  if( !file.good() )
  {
    file.close();
    file.clear();
    file.open( string( ldraw_directory ).append( "p/" ).append( part ) );
  }
  if( !file.good() )
  {
    file.close();
    file.clear();
    file.open( string( ldraw_directory ).append( "parts/s/" ).append( part ) );
  }
  if( !file.good() )
  {
    file.close();
    file.clear();
    file.open( string( ldraw_directory ).append( "p/48/" ).append( part ) );
  }
  //cout << part << endl << file.good() << endl;

  string line;

  vector< double > bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
  vector< double > temp;

  while( getline( file, line ) )
  {
    line.erase( std::remove(line.begin(), line.end(), '\r'), line.end() );
    transform(line.begin(), line.end(), line.begin(), ::tolower);
    pair< vector< double >, string > l_parsed = parse_line( line );
    vector< double > data = l_parsed.first;
    int type = int(data[0]);

    if( type == 1 )
    {
      vector< double > pos2;

      pos2.push_back( pos[0] + data[2]*rotation[0] + data[3]*rotation[1] + data[4]*rotation[2] );
      pos2.push_back( pos[1] + data[2]*rotation[3] + data[3]*rotation[4] + data[4]*rotation[5] );
      pos2.push_back( pos[2] + data[2]*rotation[6] + data[3]*rotation[5] + data[4]*rotation[8] );

      data.erase(data.begin(), data.begin() + 5);

      vector< double > rotation1;
      for( int i = 0; i < 3; ++i)
        for( int j = 0; j < 3; ++j)
          rotation1.push_back( rotation[i*3+0]*data[j] + rotation[i*3+1]*data[3+j] + rotation[i*3+2]*data[6+j] );

      temp = generate_mask( l_parsed.second, ldraw_directory, rotation1, pos2, mask);
    }
    else if( type == 3 || type == 4 )
    {
      //pos[0] += data[2]*rotation[0] + data[2]*rotation[1] + data[2]*rotation[2];
      //pos[1] += data[3]*rotation[3] + data[3]*rotation[4] + data[3]*rotation[5];
      //pos[2] += data[4]*rotation[6] + data[4]*rotation[7] + data[4]*rotation[8];
      data.erase(data.begin(), data.begin() + 2);
      temp = draw_on_mask( data, rotation, pos, mask );
    }

    if( type == 1 || type == 3 || type == 4 )
    {
      for(int i = 0; i < 4; ++i)
      {
        if( temp[0] < bounds[0] ) bounds[0] = temp[0];
        if( temp[1] < bounds[1] ) bounds[1] = temp[1];
        if( temp[2] > bounds[2] ) bounds[2] = temp[2];
        if( temp[3] > bounds[3] ) bounds[3] = temp[3];
      }
    }
  }

  return bounds;
}

// Takes block from LDraw string, ImagemMagick Image, and rgb color data
// Compute the best matching color on equirectangular image and returns strinng with that color
string draw_on_image( string s, Image image, int (*color_data)[256][256], string ldraw_directory )
{
  bool full = true;

  stringstream line, output, buffer;
  string l;
  vector< int > data;

  double width = image.columns();
  double height = image.rows();

  line << s;
  getline( line, l, ' ');

  if(full)
  {
    Image mask( Geometry(width,height), Color("white"));
    mask.fillColor("black");
    pair< vector< double >, string > l_parsed = parse_line( s );
    vector< double > rotation = l_parsed.first;
    int type = rotation[0];
    if( type == 1 )
    {
      vector< double > pos = { rotation[2], rotation[3], rotation[4] };
      rotation.erase(rotation.begin(), rotation.begin() + 5);

      vector< double > bounds = generate_mask( l_parsed.second, ldraw_directory, rotation, pos, mask);
      if( bounds[0] < FLT_MAX )
      {
        Image dest( Geometry(width,height), Color("transparent"));

        dest.clipMask(mask);
        Geometry offset2(0,0,0,0);
        dest.composite( image, offset2, OverCompositeOp );

        Pixels view(dest);

        PixelPacket *pixels = view.get( floor(bounds[0]), floor(bounds[1]), ceil(bounds[2])-floor(bounds[0]), ceil(bounds[3])-floor(bounds[1]));
        buffer << s;

        do{ getline( buffer, l, ' '); }while( l.empty() );
        do{ getline( buffer, l, ' '); }while( l.empty() );

        output << "1 " << closest_color( pixels, ceil(bounds[2])-floor(bounds[0]), ceil(bounds[3])-floor(bounds[1]), color_data);

        for( int i = 0; i < 13; ++i)
        {
          do{ getline( buffer, l, ' '); }while( l.empty() );
          output << " " << l; 
        }

        output << endl;
      }
    }
  }
  else
  {/*
    Coordinate c = equi_project( x, z, y, width-1, height-1);
    Pixels view(image);
    int cx = c.x();
    int cy = c.y();
    PixelPacket *pixels = view.get( cx, cy, 1, 1);

    buffer << s;
    getline( buffer, l, ' ');
    getline( buffer, l, ' ');
    output << " " << l;
    getline( buffer, l, ' ');
    output << " " << closest_color( pixels, 1, 1, color_data);
    while ( getline( buffer, l, ' ') )
    {
      output << " " << l;
    }
    output << endl;*/
  }

  return output.str();
}

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Reads an input LDraw file corresponding to one side of a Bram/Lowell lego sphere. Converts to hollow shell made of lego pieces. Optionally maps equirectangular image to sphere using lego color pallette.", ' ', "1.0");

    ValueArg<string> ldrawArg( "l", "ldraw_directory", "LDraw directory for parts", true, "none", "string");
    cmd.add( ldrawArg );

    ValueArg<string> pictureArg( "p", "picture", "Picture to map to sphere", false, "none", "string");
    cmd.add( pictureArg );

    ValueArg<string> colorArg( "c", "color", "Color data", false, "data/colors.dat", "string");
    cmd.add( colorArg );

    ValueArg<string> outputArg( "o", "output", "Output LDraw file (.ldr)", true, "out.ldr", "string");
    cmd.add( outputArg );

    ValueArg<string> inputArg( "i", "input", "Input LDraw file (.ldr)", true, "in.ldr", "string");
    cmd.add( inputArg );

    cmd.parse( argc, argv );

    string input_sphere     = inputArg.getValue();
    string output_sphere    = outputArg.getValue();
    string picture_file     = pictureArg.getValue();
    string color_file       = colorArg.getValue();
    string ldraw_directory  = ldrawArg.getValue();

    string s;

    ifstream sphere( input_sphere );

    ofstream output_file( output_sphere );

    ifstream count_stream( input_sphere ); 
    int line_count = count(istreambuf_iterator<char>(count_stream), istreambuf_iterator<char>(), '\n');
    progressbar *color_bar = progressbar_new("Coloring Sphere", line_count);

    int (*color_data)[256][256] = new int[256][256][256];
    ifstream input_data( color_file, ios::binary );
    input_data.read( (char *)color_data, 1<<26 );
    input_data.close();

    Image image;
    image.read(picture_file);

    while ( getline( sphere, s) )// && s.size() > 1 )
    {
      s.erase( std::remove(s.begin(), s.end(), '\r'), s.end() );
      transform(s.begin(), s.end(), s.begin(), ::tolower);
      output_file << draw_on_image( s, image, color_data, ldraw_directory);
      progressbar_inc( color_bar );
    }
    progressbar_finish(color_bar);

    output_file.close();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

  return 0;
}