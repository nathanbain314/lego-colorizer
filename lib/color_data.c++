#include <string>
#include <fstream>
#include <vips/vips8>

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

void build_color_data( string output_file)
{
  int *rgb = new int[1<<24];
  for( int r = 0, p = 0; r < 256; ++r)
  {
    for( int g = 0; g < 256; ++g)
    {
      for( int b = 0; b < 256; ++b, ++p)
      {
        float low = FLT_MAX;
        //int low = INT_MAX;
        int low_value = 0;
        for( int i = 0; i < 68; ++i)
        {
          
          float x1, y1, z1, l1, a1, b1;
          float x2, y2, z2, l2, a2, b2;
          vips_col_scRGB2XYZ( r, g, b, &x1, &y1, &z1);
          vips_col_scRGB2XYZ( color_values[i][0], color_values[i][1], color_values[i][2], &x2, &y2, &z2);
          vips_col_XYZ2Lab( x1, y1, z1, &l1, &a1, &b1);
          vips_col_XYZ2Lab( x2, y2, z2, &l2, &a2, &b2);
          float difference = vips_col_dE00( l1, a1, b1, l2, a2, b2);
          
          //int difference = ( r - color_values[i][0] ) * ( r - color_values[i][0] ) + ( g - color_values[i][1] ) * ( g - color_values[i][1] ) + ( b - color_values[i][2] ) * ( b - color_values[i][2] );
          if( difference < low )
          {
            low = difference;
            low_value = i;
          }
        }
        rgb[p] = low_value;
      }
    }
  }
  ofstream output( output_file, ios::binary );
  output.write( (char *)rgb, 1<<26 );
  output.close();
}

int main( int argc, char **argv)
{
  build_color_data( argv[1] );
  return 0;
}