#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <cfloat>
#include <tclap/CmdLine.h>

using namespace TCLAP;
using namespace std;

// Functions for finding max x, y, and z bounds

bool max_x( vector< int > i, vector< int > j )
{
  return abs(i[0]) < abs(j[0]);
}

bool max_y( vector< int > i, vector< int > j )
{
  return abs(i[1]) < abs(j[1]);
}

bool max_z( vector< int > i, vector< int > j )
{
  return abs(i[2]) < abs(j[2]);
}

// Takes line of LDraw file and returns vector with x, y, and z points 
vector< int > string_to_point( string s )
{
  vector< int > point;
  stringstream line;
  string l;

  line << s;
  getline( line, l, ' ');
  getline( line, l, ' ');
  getline( line, l, ' ');

  getline( line, l, ' ');
  point.push_back( stoi(l) );
  getline( line, l, ' ');
  point.push_back( stoi(l) );
  getline( line, l, ' ');
  point.push_back( stoi(l) );

  return point;
}

// Takes line of LDraw file and replaces rectangle with lego blocks
string boxes_to_blocks( string s )
{
  stringstream line, output;
  string l;
  vector< int > data;

  line << s;
  getline( line, l, ' ');

  // Parses all data to vector
  while ( getline( line, l, ' ') && l.find(".dat")==string::npos )
  {
    data.push_back( stoi(l) );
  }

  // Fill box with 1x1 plates
  if( l == "box.dat" )
  {
    int size_x = data[5];
    int size_z = data[13];
    for(int i = data[2] + 10 - size_x; i < data[2]+size_x; i+=20)
    {
      for(int j = data[4] + 10 - size_z; j < data[4]+size_z; j+=20)
      {
        output << " " << data[0] << " " << data[1] << " " << i << " " << data[3] - 4 << " " << j << " 1 0 0 0 1 0 0 0 1\n";
      }
    }
  }

  return output.str();
}

// Hollows side of sphere
// Takes a stream of ldraw lines representing a side of a sphere and removes all of the blocks on the inside
stringstream hollow_blocks( stringstream &blocks )
{
  stringstream output;
  vector< vector< int > > points;
  vector< string > lines;
  string s;

  // Create vector of lines and points in those lines
  while ( getline( blocks, s) && s.size() > 1 )
  {
    lines.push_back( s );
    points.push_back( string_to_point(s) );
  }

  // Used for size of exists array. width and lenght are multiplied by four to compomsate for other size
  // and for the ability to have half spaced blocks
  int point_size = points.size();
  int width = 6+abs(max_element( points.begin(), points.end(), max_x)->at(0))/5;
  int height = 2+abs(max_element( points.begin(), points.end(), max_y)->at(1))/8;
  int length = 6+abs(max_element( points.begin(), points.end(), max_z)->at(2))/5;

  // Create array of data on whether a block exists at that point
  bool ***exists;
  exists = new bool**[width];
  for( int w = 0; w < width; ++w)
  {
    exists[w] = new bool*[height];
    for( int h = 0; h < height; ++h)
    {
      exists[w][h] = new bool[length];
      for( int l = 0; l < length; ++l)
      {
        exists[w][h][l] = false;
      }
    }
  }

  for( int i = 0; i < point_size; ++i)
  {
    exists[points[i][0]/10+width/2][abs(points[i][1]+8)/8][points[i][2]/10+length/2] = true;
  }

  // Determines if a block should be removed from the model
  for( int i = 0; i < point_size; ++i)
  {
    int x = points[i][0]/10+width/2;
    int y = abs(points[i][1]+8)/8;
    int z = points[i][2]/10+length/2;

    // If the block is hidden, remove it
    bool d = ( exists[x-2][y][z] || ( exists[x-2][y][z-1] && exists[x-2][y][z+1] ) ) &&
             ( exists[x+2][y][z] || ( exists[x+2][y][z-1] && exists[x+2][y][z+1] ) ) &&
             ( exists[x][y][z-2] || ( exists[x-1][y][z-2] && exists[x+1][y][z-2] ) ) &&
             ( exists[x][y][z+2] || ( exists[x-1][y][z+2] && exists[x+1][y][z+2] ) ) &&
             ( exists[x][y+1][z] || ( exists[x-1][y+1][z] && exists[x+1][y+1][z] ) || ( exists[x][y+1][z-1] && exists[x][y+1][z+1] ) ||
              ( exists[x-1][y+1][z-1] && exists[x+1][y+1][z-1] && exists[x-1][y+1][z+1] && exists[x+1][y+1][z+1] ) );

    // It if is on the uncomplete side and lower than two blocks from the surface, remove it
    d = d ||  ( ( x == width-3 || x == 3 ) 
          && ( exists[x][y][z-1] || exists[x][y][z-2] ) 
          && ( exists[x][y][z+1] || exists[x][y][z+2] )
          && ( exists[x][y+3][z] || ( exists[x][y+2][z-1] && exists[x][y+3][z+1] ) ) );

    // If the block overlaps another block, remove it 
    d = d || ( ( exists[x][y+1][z-1] || exists[x][y+1][z+1] )
          && ( exists[x][y][ z + ( ( z < length/2 ) ? -2 : 2 ) ] ) );

    if( !d )
    {
      if( exists[x-1][y+1][z] || exists[x+1][y+1][z]  ||  exists[x][y+1][z-1] || exists[x][y+1][z+1] )
      {
        // 1x2 knob
        output << " 1 1 " << points[i][0] << " " << points[i][1] << " " << points[i][2] + ((points[i][2] < 0) ? 10 : -10);
        output << " 1 0 0 0 1 0 0 0 1 3794b.dat\n";
        // 1x1 tile
        //output << " 1 1 " << points[i][0] << " " << points[i][1] << " " << points[i][2];
        //output << " 30039.dat\n";
      }
      else
      {
        // 1x1 plate
        output << " 1 1 " << points[i][0] << " " << points[i][1] << " " << points[i][2];
        output << " 1 0 0 0 1 0 0 0 1 3024.dat\n";
      }
    }

  }
  return output;

}

/*
 * Copies blocks to other sides
 * 
 * s     - a string from an LDraw file
 * size  - the offset from the origin to move sides to
 * sides - a vector of int values representing sides to copy to
 */
string sides_to_sphere( string s, int size, vector<int> sides )
{
  stringstream line, output;
  string l;
  vector< int > data;
  int offset = size*10;

  // Position matrices for offset position
  int position[6][3][3] = {
    {
      {0,0,1},
      {0,1,0},
      {-1,0,0}
    },
    {
      {0,0,-1},
      {0,-1,0},
      {1,0,0}
    },
    {
      {1,0,0},
      {0,0,1},
      {0,-1,0}
    },
    {
      {1,0,0},
      {0,0,-1},
      {0,1,0}
    },
    {
      {0,1,0},
      {-1,0,0},
      {0,0,1}
    },
    {
      {0,-1,0},
      {1,0,0},
      {0,0,1}
    }
  };

  // Rotation matrices for rotation
  int rotation[6][3][3] = {
    {
      {1,0,0},
      {0,1,0},
      {0,0,1}
    },
    {
      {-1,0,0},
      {0,-1,0},
      {0,0,-1}
    },
    {
      {0,0,1},
      {-1,0,0},
      {0,-1,0}
    },
    {
      {0,0,1},
      {1,0,0},
      {0,1,0}
    },
    {
      {0,1,0},
      {0,0,-1},
      {-1,0,0}
    },
    {
      {0,-1,0},
      {0,0,-1},
      {1,0,0}
    }
  };

  line << s;
  getline( line, l, ' ');

  while ( getline( line, l, ' ') && l.find(".dat")==string::npos )
  {
    data.push_back( stoi(l) );
  }

  int x = data[2];
  int y = data[3]-offset;
  int z = data[4];

  // Rotate and position block on all relevant sides
  for( int &i : sides)
  {
    int _x = x * position[i][0][0] + y * position[i][0][1] + z * position[i][0][2];
    int _y = x * position[i][1][0] + y * position[i][1][1] + z * position[i][1][2];
    int _z = x * position[i][2][0] + y * position[i][2][1] + z * position[i][2][2];
    output << " " << data[0] << " " << data[1] << " " << _x << " " << _y << " " << _z << " ";
    for( int j = 0; j < 3; ++j)
      for( int k = 0; k < 3; ++k)
        output << rotation[i][j][k] <<  " " ;
    output << l << endl;
  }
  return output.str();
}

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Reads an input LDraw file corresponding to one side of a Bram/Lowell lego sphere. Converts to hollow shell made of lego pieces.", ' ', "1.0");

    MultiArg<int> sideArg( "s", "side", "Specific side to create", false, "int");
    cmd.add( sideArg );

    ValueArg<string> outputArg( "o", "output", "Output LDraw file (.ldr)", true, "out.ldr", "string");
    cmd.add( outputArg );

    ValueArg<string> inputArg( "i", "input", "Input LDraw file (.ldr)", true, "in.ldr", "string");
    cmd.add( inputArg );

    cmd.parse( argc, argv );

    string input_sphere     = inputArg.getValue();
    string output_sphere    = outputArg.getValue();
    vector<int> sides       = sideArg.getValue();

    sort( sides.begin(), sides.end() );
    sides.erase( unique( sides.begin(), sides.end() ), sides.end() );

    if( sides.size() == 0 )
    {
      sides = { 0, 1, 2, 3, 4, 5 };
    }

    string s;
    stringstream blocks, hollow;

    int p1 = input_sphere.find("with_") + 5;
    int p2 = input_sphere.find("x");

    int width  = stoi( input_sphere.substr( p1, p2-p1 ) );

    ifstream input_file( input_sphere );

    for( int i = 0; i < 6; ++i)
    {
      getline( input_file, s);
    }
    while ( getline( input_file, s) && s.size() > 1 )
    {
      blocks << boxes_to_blocks( s );
    }
    input_file.close();

    hollow = hollow_blocks( blocks );

    ofstream output_file( output_sphere );

    while ( getline( hollow, s) && s.size() > 1 )
    {
      output_file << sides_to_sphere( s, width, sides );
    }

    output_file.close();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
}