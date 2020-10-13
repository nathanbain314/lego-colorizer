#include "LegoColorizer.h"

void rgbToLab( int r, int g, int b, float &l1, float &a1, float &b1 )
{
  vips_col_sRGB2scRGB_8( r, g, b, &l1,&a1,&b1 );
  vips_col_scRGB2XYZ( l1, a1, b1, &l1, &a1, &b1 );
  vips_col_XYZ2Lab( l1, a1, b1, &l1, &a1, &b1 );
}

void changeColorspace( unsigned char * c, float * c2, int start, int end, int width, double gamma, ProgressBar * changingColorspace )
{
  bool show = !(start);

  for( int height = start, index = start*width; height < end; ++height )
  {
    for( int w = 0; w < width; ++w, ++index )
    {
      for( int k = 0; k < 3; ++k )
      {
        int n10 = c[3*index+k];
        
        float v = (float)n10;

        v /= 256.0;

        v = pow( v, gamma );
        c2[3*index+k] = 256 * v;
      }
      
      rgbToLab( c2[3*index+0], c2[3*index+1], c2[3*index+2], c2[3*index+0], c2[3*index+1], c2[3*index+2] );      
    }

    if( show ) changingColorspace->Increment();
  }
}


double ccw( Vertex &v1, Vertex &v2, Vertex &v3 )
{
  return (v2.x - v1.x)*(v3.y - v1.y) - (v2.y - v1.y)*(v3.x - v1.x);
}

int windingNumber( Vertex p, vector< Vertex > &windingVertices )
{
  int n = 0;

  int s = windingVertices.size();

  for( int i = 0; i < s; ++i )
  {
    Vertex &v1 = windingVertices[i];
    Vertex &v2 = windingVertices[(i+1)%s];

    if( v1.y <= p.y )
    {
      if( v2.y > p.y && ccw( v1, p, v2 ) < 0 )
      {
        ++n;
      }
    }
    else
    {
      if( v2.y <= p.y && ccw( v1, p, v2 ) > 0 )
      {
        --n;
      }
    }
  }

  return n;
}

// Takes x, y, and z coordinates of point in sphere, and the width and height of the image.
// Returns the cooresponding point for an equirectangular image.
Vertex equi_project( double x, double y, double z, double width, double height)
{
  double r = sqrt( x * x + y * y + z * z );
  double lat = acos( z / r )-M_PI/2;
  double lng = atan2( y, x );

  return Vertex( ((lng + M_PI) * width / (2*M_PI)), ((M_PI/2 - lat) * height / M_PI) );
}

// Takes array of pixels, width and height of the array, and array of color best matching colors for all rgb values.
// Returns the most common matching color for that group of pixels.
int closest_color( float *data, float *colors, vector< Vertex > &mask, int width, int height, vector< int > &colorsToUse )
{
  float avgColor[3] = {0.0,0.0,0.0};
  for( int i = 0; i < mask.size(); ++i )
  {
    int y = mask[i].y;
    int x = mask[i].x;

    if( x < 0 || x >= width ) continue;
    if( y < 0 || y >= height ) continue;

    for( int k = 0; k < 3; ++k )
    {
      avgColor[k] += data[3*(y*width+x)+k];
    }
  }

  for( int k = 0; k < 3; ++k )
  {
    avgColor[k] /= mask.size();
  }

  int best = 0;
  float bestDifference = 1000000000.0;

  for( int i1 = 0; i1 < colorsToUse.size(); ++i1 )
  {
    int i = colorsToUse[i1];

    float diff = 0.0;

    for( int k = 0; k < 3; ++k )
    {
      diff += (avgColor[k]-colors[i*3+k])*(avgColor[k]-colors[i*3+k]);
    }

    if( diff < bestDifference )
    {
      bestDifference = diff;
      best = i;
    }
  }

  return color_codes[best];
}

// Checks if the shortest line between two coordinates goes across a border
bool across_border( Vertex &c1, Vertex &c2, double &width )
{
  double x1 = c1.x;
  double y1 = c1.y;
  double x2 = c2.x;
  double y2 = c2.y;

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
double cross_height( Vertex c1, Vertex c2, double width )
{
  double x1 = c1.x;
  double y1 = c1.y;
  double x2 = c2.x;
  double y2 = c2.y;
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

void addMask( vector< Vertex > &points, vector< Vertex > &mask )
{
  vector< double > bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };

  for(int i = 0; i < points.size(); ++i)
  {
    if( points[i].x < bounds[0] ) bounds[0] = points[i].x;
    if( points[i].y < bounds[1] ) bounds[1] = points[i].y;
    if( points[i].x > bounds[2] ) bounds[2] = points[i].x;
    if( points[i].y > bounds[3] ) bounds[3] = points[i].y;

    mask.push_back( points[i] );
  }

  for( double i = bounds[1]; i < bounds[3]+1.0; ++i )
  {
    for( double j = bounds[0]; j < bounds[2]+1.0; ++j )
    {
      int w = abs(windingNumber( Vertex(j,i), points ));
      if( w == 1 )
      {
        mask.push_back( Vertex(j,i) );
      }
    }
  }
}

vector< double > draw_on_mask( vector< double > data, vector< double > rotation, vector< double > pos, vector< Vertex > &mask, double width, double height )
{
  int size = int(data.size()/3);

  double x_avg = 0;
  vector<Vertex> points;
  for(int i = 0; i < size; ++i)
  {
    double _x = pos[0] + data[3*i+0] * rotation[0] + data[3*i+1] * rotation[1] + data[3*i+2] * rotation[2];
    double _y = pos[1] + data[3*i+0] * rotation[3] + data[3*i+1] * rotation[4] + data[3*i+2] * rotation[5];
    double _z = pos[2] + data[3*i+0] * rotation[6] + data[3*i+1] * rotation[7] + data[3*i+2] * rotation[8];

    points.push_back(equi_project( _x, _z, _y, width, height));
    x_avg += ( points[i].x > width/2 );
  }

  x_avg/=(size);

  vector< double > bounds = { FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN };
  for(int i = 0; i < size; ++i)
  {
    if( abs( points[i].x - width ) < .001 && x_avg < .5)
    {
      points[i] = Vertex( 0, points[i].y);
    }
    if( points[i].x < bounds[0] ) bounds[0] = points[i].x;
    if( points[i].y < bounds[1] ) bounds[1] = points[i].y;
    if( points[i].x > bounds[2] ) bounds[2] = points[i].x;
    if( points[i].y > bounds[3] ) bounds[3] = points[i].y;
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
    vector< Vertex > left, right;
    for( int i = 0; i < size; ++i)
    {
      if( points[i].x < width/2 )
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
        left.push_back( Vertex( 0, y3 ) );
        right.push_back( Vertex( width-1, y3 ) );
      }
    }
    
    addMask( left, mask );
    addMask( right, mask );
  }
  else if( abs( pos[0] ) < .001 && abs( pos[2] ) < .001 )
  {
    bounds[0] = 0;
    bounds[2] = width;
    double y3 = points[0].y;
    double y4 = height * round( y3 / height );
    bounds[0] = 0;
    bounds[2] = width;

    if( y4 < bounds[1] ) bounds[1] = y4;
    if( y4 > bounds[3] ) bounds[3] = y4;

    vector< Vertex > p2{ Vertex( 0, y3), Vertex( width, y3), Vertex( width, y4 ), Vertex( 0, y4 ) };

    addMask( p2, mask );
  }
  else
  {
    addMask( points, mask );
  }
  return bounds;
}

vector< double > generate_mask( string part, string ldraw_directory, vector< double > rotation, vector< double > pos, vector< Vertex > &mask, double width, double height )
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

      temp = generate_mask( l_parsed.second, ldraw_directory, rotation1, pos2, mask, width, height );
    }
    else if( type == 3 || type == 4 )
    {
      data.erase(data.begin(), data.begin() + 2);
      temp = draw_on_mask( data, rotation, pos, mask, width, height );
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
string draw_on_image( string s, float *c, float *colors, int width, int height, string ldraw_directory, vector< int > colorsToUse )
{
  bool full = true;

  stringstream line, output, buffer;
  string l;
  vector< int > data;

  line << s;
  getline( line, l, ' ');

  if(full)
  {
    vector< Vertex > mask;

    pair< vector< double >, string > l_parsed = parse_line( s );
    vector< double > rotation = l_parsed.first;
    int type = rotation[0];
    if( type == 1 )
    {
      vector< double > pos = { rotation[2], rotation[3], rotation[4] };
      rotation.erase(rotation.begin(), rotation.begin() + 5);

      vector< double > bounds = generate_mask( l_parsed.second, ldraw_directory, rotation, pos, mask, width, height);
      if( bounds[0] < FLT_MAX )
      {
        buffer << s;

        do{ getline( buffer, l, ' '); }while( l.empty() );
        do{ getline( buffer, l, ' '); }while( l.empty() );

        output << "1 " << closest_color( c, colors, mask, width, height, colorsToUse );

        for( int i = 0; i < 13; ++i)
        {
          do{ getline( buffer, l, ' '); }while( l.empty() );
          output << " " << l; 
        }

        output << endl;
      }
    }
  }

  return output.str();
}

string drawThread( int start, int end, float *c, float *colors, int width, int height, string ldraw_directory, vector< int > &colorsToUse, vector< string > &lines, ProgressBar *color_bar )
{
  stringstream output;
  string s;

  for( int i = start; i < end; ++i )
  {
    s = lines[i];
    s.erase( std::remove(s.begin(), s.end(), '\r'), s.end() );
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    output << draw_on_image( s, c, colors, width, height, ldraw_directory, colorsToUse );
    if( start == 0 ) color_bar->Increment();
  }

  return output.str();
}





bool valid_point( double x, double y, double inner_radius, double outer_radius)
{
  double offset = ( x > 0 ) ? -.5 : .5;
  bool valid_point = ( (x-offset) * (x-offset) + (y-.5) * (y-.5) >= inner_radius * inner_radius ) &&
                     ( (x+offset) * (x+offset) + (y+.5) * (y+.5) <= outer_radius * outer_radius );
  return valid_point;
}

bool valid_point2( double x, double y, double inner_radius, double outer_radius)
{
  bool valid_point = true;

  for( int i = -1; i <= 1; i += 2 )
  {
    for( int j = -1; j <= 1; j += 2 )
    {
      double x1 = x + (double)j/2.0;
      double y1 = y + (double)i/2.0;
      bool good = (x1*x1 + y1*y1 > inner_radius*inner_radius) && (x1*x1 + y1*y1 < outer_radius*outer_radius);
      valid_point = valid_point && good;
    }
  }

  return valid_point;
}

bool valid_point3( double x, double y, double inner_radius, double outer_radius)
{
  bool inside = true;
  bool outside = true;

  for( int i = -1; i <= 1; i += 2 )
  {
    for( int j = -1; j <= 1; j += 2 )
    {
      double x1 = x + (double)j/2.0;
      double y1 = y + (double)i/2.0;
      outside = outside && (x1*x1 + y1*y1 < outer_radius*outer_radius);
      inside = inside && (x1*x1 + y1*y1 > inner_radius*inner_radius);
    }
  }

  return inside && outside;
}

string create_circle( double inner_radius, double outer_radius, float *data, float *colors, int width, int height, vector< int > &colorsToUse )
{
  stringstream output;

  int size = width / ( outer_radius - inner_radius + 1 );

  double mid = height / 2;
  int count = 0;

  int startRadius = 2*outer_radius; 

  for( double x = .5-(double)inner_radius / sqrt(2); x < 0; ++x)
  {
    bool began = true;
    for( double y = startRadius; y > -1; --y,--y)
    {
      double angle = atan2( y/2, x);

      if( valid_point2( x, y/2, inner_radius, outer_radius ) )
      {
        if(began)
        {
          began = false;
        }

        double start = int(sqrt(x*x+y*y/4)-inner_radius+ .5*sin( angle ));

        double w = (size*(start+1) < width) ? size : width - size*start;

        vector< Vertex > mask;

        int s = (size * start<width-size/2) ? size*start : width-size/2;

        for( int i = 0; i < height; ++i )
        {
          for( int j = s; j < s + w; ++j )
          {
            mask.push_back( Vertex( j, i ) );
          }
        }

        int color = closest_color( data, colors, mask, width, height, colorsToUse );

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
      }
    }
  }

  for( double x = inner_radius / sqrt(2)+.5; x < outer_radius+2; ++x)
  {
    for( double y = inner_radius / sqrt(2)+.5; y < outer_radius+2; ++y)
    {
      double angle = atan2( y, x);
      if( valid_point2( x, y, inner_radius, outer_radius ) )
      {
        double start = int(sqrt(x*x+y*y)-inner_radius+.5*sin( angle ));

        double w = (size*(start+1) < width) ? size : width - size*start;

        vector< Vertex > mask;

        int s = (size * start<width-size/2) ? size*start : width-size/2;

        for( int i = 0; i < height; ++i )
        {
          for( int j = s; j < s + w; ++j )
          {
            mask.push_back( Vertex( j, i ) );
          }
        }

        int color = closest_color( data, colors, mask, width, height, colorsToUse );

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




void LegoColorizer( string input_sphere, string output_sphere, string picture_file, string colorName, string ldraw_directory, double gamma, double inner_radius, double outer_radius )
{
  string s;

  ifstream sphere( input_sphere );

  ofstream output_file( output_sphere );

  ifstream count_stream( input_sphere ); 
  int line_count = count(istreambuf_iterator<char>(count_stream), istreambuf_iterator<char>(), '\n');


  VImage image = VImage::vipsload( (char *)picture_file.c_str() ).autorot();

  // Convert to a three band image
  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  int width = image.width();
  int height = image.height();




  vector< int > colorsToUse;

  ifstream file(colorName);

  string str; 
  while (std::getline(file, str))
  {
    int n = stoi(str);

    int index = -1;

    for( int i = 0; i < 68; ++i )
    {
      if( color_codes[i] == n )
      {
        index = i;
        break;
      }
    }

    colorsToUse.push_back(index);
  }

  // Get image data
  unsigned char * inputData = ( unsigned char * )image.data();
  float * c2 = new float[3*width*height];

  int threads = sysconf(_SC_NPROCESSORS_ONLN);

  ProgressBar *changingColorspace = new ProgressBar(height/threads, "Changing colorspace");

  float colors[68*3];

  for( int k1 = 0; k1 < colorsToUse.size(); ++k1 )
  {
    int k = colorsToUse[k1];

    rgbToLab(color_values[k][0],color_values[k][1],color_values[k][2],colors[3*k+0],colors[3*k+1],colors[3*k+2]);
  }

  future< void > ret2[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret2[k] = async( launch::async, &changeColorspace, inputData, c2, k*height/threads, (k+1)*height/threads, width, gamma, changingColorspace );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret2[k].get();
  }

  changingColorspace->Finish();



  if( input_sphere == "" )
  {
    output_file << create_circle( inner_radius, outer_radius, c2, colors, width, height, colorsToUse );
  }
  else
  {
    ProgressBar *color_bar = new ProgressBar(line_count/threads, "Coloring Sphere");

    vector< string > lines;

    while ( getline( sphere, s) )
    {
      lines.push_back(s);
    }

    future< string > ret3[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret3[k] = async( launch::async, &drawThread, k*lines.size()/threads, (k+1)*lines.size()/threads, c2, colors, width, height, ldraw_directory, ref(colorsToUse), ref(lines), color_bar );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      output_file << ret3[k].get();
    }

    color_bar->Finish();
  }

  output_file.close();
}
