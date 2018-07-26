#include <exception>
#include <iostream>
#include <array>
#include <fstream>
#include <cmath>

#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <world_builder/assert.h>
#include <world_builder/nan.h>
#include <world_builder/utilities.h>
#include <world_builder/world.h>
#include <world_builder/coordinate_system.h>

namespace po = boost::program_options;
using namespace WorldBuilder;
using namespace WorldBuilder::Utilities;

void project_on_sphere(double radius, double &x_, double &y_, double &z_)
{
  double x = x_;
  double y = y_;
  double z = z_;
  const WorldBuilder::Point<3> in_point(std::array<double,3> {x,y,z}, WorldBuilder::CoordinateSystem::cartesian);
  WorldBuilder::Point<3> output_point(std::array<double,3> {0,0,0}, WorldBuilder::CoordinateSystem::cartesian);
  double r = in_point.norm();
  double theta = std::atan2(in_point[1],in_point[0]);
  double phi = std::acos(in_point[2]/r);

  x_ = radius * std::cos(theta) * std::sin(phi);
  y_ = radius * std::sin(theta) * std::sin(phi);
  z_ = radius * std::cos(phi);

}

void lay_points(double x1, double y1, double z1,
                double x2, double y2, double z2,
                double x3, double y3, double z3,
                double x4, double y4, double z4,
                std::vector<double> &x, std::vector<double> &y, std::vector<double> &z,
                std::vector<bool> &hull, unsigned int level)
{
  // TODO: Assert that the vectors have the correct size;
  unsigned int counter = 0;
  for (unsigned int j = 0; j < level+1; ++j)
    {
      for (unsigned int i = 0; i < level+1; ++i)
        {
          // equidistant (is this irrelevant?)
          double r = -1.0 + (2.0 / level) * i;
          double s = -1.0 + (2.0 / level) * j;

          // equiangular
          const double pi4 = M_PI*0.25;
          double x0 = -pi4 + i * 2.0 * (double)pi4/(double)level;
          double y0 = -pi4 + j * 2.0 * (double)pi4/(double)level;
          r = std::tan(x0);
          s = std::tan(y0);


          double N1 = 0.25 * (1.0 - r) * (1.0 - s);
          double N2 = 0.25 * (1.0 + r) * (1.0 - s);
          double N3 = 0.25 * (1.0 + r) * (1.0 + s);
          double N4 = 0.25 * (1.0 - r) * (1.0 + s);

          x[counter] = x1 * N1 + x2 * N2 + x3 * N3 + x4 * N4;
          y[counter] = y1 * N1 + y2 * N2 + y3 * N3 + y4 * N4;
          z[counter] = z1 * N1 + z2 * N2 + z3 * N3 + z4 * N4;

          if (i == 0) hull[counter] = true;
          if (j == 0) hull[counter] = true;
          if (i == level) hull[counter] = true;
          if (j == level) hull[counter] = true;
          counter++;
        }
    }
}

int main(int argc, char **argv)
{
  /**
   * First parse the command line options
   */
  std::string wb_file;
  std::string data_file;

  unsigned int dim = 3;
  unsigned int compositions = 0;
  double gravity = 10;

  //commmon
  std::string grid_type = "chunk";

  unsigned int n_cell_x = NaN::ISNAN; // x or long
  unsigned int n_cell_y = NaN::ISNAN; // y or lat
  unsigned int n_cell_z = NaN::ISNAN; // z or depth


  // spherical
  double x_min = NaN::DSNAN;  // x or long
  double x_max = NaN::DSNAN;  // x or long
  double y_min = NaN::DSNAN; // y or lat
  double y_max = NaN::DSNAN; // y or lat
  double z_min = NaN::DSNAN; // z or inner_radius
  double z_max = NaN::DSNAN; // z or outer_radius

  try
    {
      po::options_description desc("Allowed options");
      desc.add_options()
      ("help", "produce help message")
      ("files", po::value<std::vector<std::string> >(), "list of files, starting with the World Builder "
       "file and data file(s) after it.");

      po::positional_options_description p;
      p.add("files", -1);

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
      po::notify(vm);

      if (vm.count("help"))
        {
          std::cout << std::endl << "TODO: Write description how to use this." << std::endl << std::endl;
          std::cout << desc << "\n";
          //return 0;
        }

      if (!vm.count("files"))
        {
          std::cout << "Error: There where no files passed to the World Builder, use --help for more " << std::endl
                    << "information on how  to use the World Builder app." << std::endl;
          //return 0;
        }

      std::vector<std::string> file_names = vm["files"].as<std::vector<std::string> >();

      if (file_names.size() < 2)
        {
          std::cout << "Error:  The World Builder app requires at least two files, a World Builder file " << std::endl
                    << "and a data file to convert." << std::endl;
          //return 0;
        }

      wb_file = file_names[0];
      // Todo: Is it useful to check whether the string is empty?


      data_file = file_names[1];
      // Todo: Is it useful to check whether the string is empty?

    }
  catch (std::exception &e)
    {
      std::cerr << "error: " << e.what() << "\n";
      return 1;
    }
  catch (...)
    {
      std::cerr << "Exception of unknown type!\n";
      return 1;
    }

  /**
   * Try to start the world builder
   */
  std::unique_ptr<WorldBuilder::World> world;
  try
    {
      world = std::make_unique<WorldBuilder::World>(wb_file);
    }
  catch (std::exception &e)
    {
      std::cerr << "Could not start the World builder, error: " << e.what() << "\n";
      return 1;
    }
  catch (...)
    {
      std::cerr << "Exception of unknown type!\n";
      return 1;
    }


  /**
   * Read the data from the data files
   */
  // if config file is available, parse it
  /*  if(config_file != "")
      {
        // Get world builder file and check wether it exists
        WBAssertThrow(access( config_file.c_str(), F_OK ) != -1,
            "Could not find the provided convig file at the specified location: " + config_file);


        // Now read in the world builder file into a file stream and
        // put it into a boost property tree.
        //std::ifstream json_input_stream(config_file.c_str());
        ptree tree;
        tree.read_json(config_file, tree);

        if(boost::optional<unsigned int> value = tree.get_optional<unsigned int>("dim"))
            dim = value.get();

        if(boost::optional<unsigned int> value = tree.get_optional<unsigned int>("compositions"))
            compositions = value.get();

      }*/
  std::string line;
  std::ifstream data_stream(data_file);

  // move the data into a vector of strings
  std::vector<std::vector<std::string> > data;
  std::string temp;

  while (std::getline(data_stream, temp))
    {
      std::istringstream buffer(temp);
      std::vector<std::string> line((std::istream_iterator<std::string>(buffer)),
                                    std::istream_iterator<std::string>());

      // remove the comma's in case it is a comma separated file.
      // TODO: make it split for comma's and/or spaces
      for (unsigned int i = 0; i < line.size(); ++i)
        line[i].erase(std::remove(line[i].begin(), line[i].end(), ','), line[i].end());

      data.push_back(line);
    }

  // Read config from data if pressent
  for (unsigned int i = 0; i < data.size(); ++i)
    {
      if (data[i].size() == 0)
        continue;

      if (data[i][0] == "#")
        continue;

      if (data[i][0] == "grid_type" && data[i][1] == "=")
        {
          grid_type = data[i][2];
        }

      if (data[i][0] == "dim" && data[i][1] == "=")
        {
          dim = string_to_unsigned_int(data[i][2]);
        }

      if (data[i][0] == "compositions" && data[i][1] == "=")
        compositions = string_to_unsigned_int(data[i][2]);

      if (data[i][0] == "x_min" && data[i][1] == "=")
        x_min = string_to_double(data[i][2]);
      if (data[i][0] == "x_max" && data[i][1] == "=")
        x_max = string_to_double(data[i][2]);
      if (data[i][0] == "y_min" && data[i][1] == "=")
        y_min = string_to_double(data[i][2]);
      if (data[i][0] == "y_max" && data[i][1] == "=")
        y_max = string_to_double(data[i][2]);
      if (data[i][0] == "z_min" && data[i][1] == "=")
        z_min = string_to_double(data[i][2]);
      if (data[i][0] == "z_max" && data[i][1] == "=")
        z_max = string_to_double(data[i][2]);

      if (data[i][0] == "n_cell_x" && data[i][1] == "=")
        n_cell_x = string_to_double(data[i][2]);
      if (data[i][0] == "n_cell_y" && data[i][1] == "=")
        n_cell_y = string_to_double(data[i][2]);
      if (data[i][0] == "n_cell_z" && data[i][1] == "=")
        n_cell_z = string_to_double(data[i][2]);
    }





  /**
   * All variables set by the user
   */



  /*// cartesian
  double x_min = 0e3;  // x or long
  double x_max = 100e3;  // x or long
  double y_min = 0e3; // y or lat
  double y_max = 110e3; // y or lat
  double z_min = 40e3; // z or inner_radius
  double z_max = 120e3; // z or outer_radius
  // */

  if (grid_type == "sphere")
    WBAssert(n_cell_x == n_cell_y, "For the sphere grid the amount of cells in the x (long) and y (lat) direction have to be the same.");

  if (grid_type == "spherical" ||
      grid_type == "chunk" ||
      grid_type == "anullus")
    {
      x_min *= (M_PI/180);
      x_max *= (M_PI/180);
      y_min *= (M_PI/180);
      y_max *= (M_PI/180);
    }



  /**
   * All variables needed for the visualization
   */
  unsigned int n_cell = NaN::ISNAN;
  unsigned int n_p = NaN::ISNAN;

  std::vector<double> grid_x(0);
  std::vector<double> grid_y(0);
  std::vector<double> grid_z(0);
  std::vector<double> grid_depth(0);

  std::vector<std::vector<unsigned int> > grid_connectivity(0);




  /**
   * Begin making the grid
   */
  WBAssertThrow(dim == 2 || dim == 3, "Dimension should be 2d or 3d.");
  bool compress_size = false;
  if (grid_type == "cartesian")
    {
      n_cell = n_cell_x * n_cell_z * (dim == 3 ? n_cell_y : 1.0);
      if(compress_size == false && dim == 3)
    	  n_p = n_cell * 8 ; // it shouldn't matter for 2d in the output, so just do 3d.
      else
          n_p = (n_cell_x + 1) * (n_cell_z + 1) * (dim == 3 ? (n_cell_y + 1) : 1.0);


      double dx = (x_max - x_min) / n_cell_x;
      double dy = (y_max - y_min) / n_cell_y;
      double dz = (z_max - z_min) / n_cell_z;

      // todo: determine wheter a input variable is desirable for this.
      double surface = z_max;

      grid_x.resize(n_p);
      grid_z.resize(n_p);

      if (dim == 3)
        grid_y.resize(n_p);

      grid_depth.resize(n_p);

      // compute positions
      unsigned int counter = 0;
      if (dim == 2)
        {
          for (unsigned int j = 0; j <= n_cell_z; ++j)
            {
              for (unsigned int i = 0; i <= n_cell_x; ++i)
                {
                  grid_x[counter] = x_min + i * dx;
                  grid_z[counter] = z_min + j * dz;
                  grid_depth[counter] = (surface - z_min) - j * dz;
                  counter++;
                }
            }
        }
      else
        {
    	  if(compress_size == true)
    	  {
          for (unsigned int i = 0; i <= n_cell_x; ++i)
            {
              for (unsigned int j = 0; j <= n_cell_y; ++j)
                {
                  for (unsigned int k = 0; k <= n_cell_z; ++k)
                    {
                      grid_x[counter] = x_min + i * dx;
                      grid_y[counter] = y_min + j * dy;
                      grid_z[counter] = z_min + k * dz;
                      grid_depth[counter] = (surface - z_min) - k * dz;
                      counter++;
                    }
                }
            }
    	  }
    	  else
    	  {
              for (unsigned int i = 0; i < n_cell_x; ++i)
                {
                  for (unsigned int j = 0; j < n_cell_y; ++j)
                    {
                      for (unsigned int k = 0; k < n_cell_z; ++k)
                        {
                    	  // position is defined by the vtk file format
                    	  // position 0 of this cell
                          grid_x[counter] = x_min + i * dx;
                          grid_y[counter] = y_min + j * dy;
                          grid_z[counter] = z_min + k * dz;
                          grid_depth[counter] = (surface - z_min) - k * dz;
                          counter++;
                    	  // position 1 of this cell
                          grid_x[counter] = x_min + (i + 1) * dx;
                          grid_y[counter] = y_min + j * dy;
                          grid_z[counter] = z_min + k * dz;
                          grid_depth[counter] = (surface - z_min) - k * dz;
                          counter++;
                    	  // position 2 of this cell
                          grid_x[counter] = x_min + (i + 1) * dx;
                          grid_y[counter] = y_min + (j + 1) * dy;
                          grid_z[counter] = z_min + k * dz;
                          grid_depth[counter] = (surface - z_min) - k * dz;
                          counter++;
                    	  // position 3 of this cell
                          grid_x[counter] = x_min + i * dx;
                          grid_y[counter] = y_min + (j + 1) * dy;
                          grid_z[counter] = z_min + k * dz;
                          grid_depth[counter] = (surface - z_min) - k * dz;
                          counter++;
                          // position 0 of this cell
                          grid_x[counter] = x_min + i * dx;
                          grid_y[counter] = y_min + j * dy;
                          grid_z[counter] = z_min + (k + 1) * dz;
                          grid_depth[counter] = (surface - z_min) - (k + 1) * dz;
                          counter++;
                          // position 1 of this cell
                          grid_x[counter] = x_min + (i + 1) * dx;
                          grid_y[counter] = y_min + j * dy;
                          grid_z[counter] = z_min + (k + 1) * dz;
                          grid_depth[counter] = (surface - z_min) - (k + 1) * dz;
                          counter++;
                          // position 2 of this cell
                          grid_x[counter] = x_min + (i + 1) * dx;
                          grid_y[counter] = y_min + (j + 1) * dy;
                          grid_z[counter] = z_min + (k + 1) * dz;
                          grid_depth[counter] = (surface - z_min) - (k + 1) * dz;
                          counter++;
                          // position 3 of this cell
                          grid_x[counter] = x_min + i * dx;
                          grid_y[counter] = y_min + (j + 1) * dy;
                          grid_z[counter] = z_min + (k + 1) * dz;
                          grid_depth[counter] = (surface - z_min) - (k + 1) * dz;
                          WBAssert(counter < n_p, "Assert counter smaller then n_P: counter = " << counter << ", n_p = " << n_p);
                          counter++;
                        }
                    }
                }
    	  }
        }

      // compute connectivity. Local to global mapping.
      grid_connectivity.resize(n_cell,std::vector<unsigned int>((dim-1)*4));

      counter = 0;
      if (dim == 2)
        {
          for (unsigned int j = 1; j <= n_cell_z; ++j)
            {
              for (unsigned int i = 1; i <= n_cell_x; ++i)
                {
                  grid_connectivity[counter][0] = i + (j - 1) * (n_cell_x + 1) - 1;
                  grid_connectivity[counter][1] = i + 1 + (j - 1) * (n_cell_x + 1) - 1;
                  grid_connectivity[counter][2] = i + 1  + j * (n_cell_x + 1) - 1;
                  grid_connectivity[counter][3] = i + j * (n_cell_x + 1) - 1;
                  counter++;
                }
            }
        }
      else
        {
    	  if(compress_size == true)
    	  {
          for (unsigned int i = 1; i <= n_cell_x; ++i)
            {
              for (unsigned int j = 1; j <= n_cell_y; ++j)
                {
                  for (unsigned int k = 1; k <= n_cell_z; ++k)
                    {
                      grid_connectivity[counter][0] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j - 1) + k - 1;
                      grid_connectivity[counter][1] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j - 1) + k - 1;
                      grid_connectivity[counter][2] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j    ) + k - 1;
                      grid_connectivity[counter][3] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j    ) + k - 1;
                      grid_connectivity[counter][4] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j - 1) + k;
                      grid_connectivity[counter][5] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j - 1) + k;
                      grid_connectivity[counter][6] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j    ) + k;
                      grid_connectivity[counter][7] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j    ) + k;
                      counter++;
                    }
                }
            }
        }
      else
      {
    	  for(unsigned int i = 0; i < n_cell; ++i)
    	  {
    		  grid_connectivity[i][0] = counter;
    		  grid_connectivity[i][1] = counter + 1;
    		  grid_connectivity[i][2] = counter + 2;
    		  grid_connectivity[i][3] = counter + 3;
    		  grid_connectivity[i][4] = counter + 4;
    		  grid_connectivity[i][5] = counter + 5;
    		  grid_connectivity[i][6] = counter + 6;
    		  grid_connectivity[i][7] = counter + 7;
    		  counter = counter + 8;
    	  }
      }
        }
    }
  else if (grid_type == "annulus")
    {
      /**
       * An annulus which is a 2d hollow sphere.
       * TODO: make it so you can determine your own cross section.
       */
      WBAssertThrow(dim == 2, "The annulus only works in 2d.");


      double inner_radius = z_min;
      double outer_radius = z_max;

      double l_outer = 2.0 * M_PI * outer_radius;

      double lr = outer_radius - inner_radius;
      double dr = lr/n_cell_z;

      unsigned int n_cell_t = (2 * M_PI * outer_radius)/dr;

      // compute the ammount of cells
      n_cell = n_cell_t *n_cell_z;
      n_p = n_cell_t *(n_cell_z + 1);  // one less then cartesian because two cells overlap.

      double sx = l_outer / n_cell_t;
      double sz = dr;

      grid_x.resize(n_p);
      grid_z.resize(n_p);
      grid_depth.resize(n_p);

      unsigned int counter = 0;
      for (unsigned int j = 0; j <= n_cell_z; ++j)
        {
          for (unsigned int i = 1; i <= n_cell_t; ++i)
            {
              grid_x[counter] = (i-1)*sx;
              grid_z[counter] = (j)*sz;
              counter++;
            }
        }

      counter = 0;
      for (unsigned int j = 1; j <= n_cell_z+1; ++j)
        {
          for (unsigned int i = 1; i <= n_cell_t; ++i)
            {
              double xi = grid_x[counter];
              double zi = grid_z[counter];
              double theta = xi / l_outer * 2.0 * M_PI;
              grid_x[counter] = std::cos(theta) * (inner_radius + zi);
              grid_z[counter] = std::sin(theta) * (inner_radius + zi);
              grid_depth[counter] = outer_radius - std::sqrt(grid_x[counter] * grid_x[counter] + grid_z[counter] * grid_z [counter]);
              counter++;
            }
        }

      grid_connectivity.resize(n_cell,std::vector<unsigned int>(4));
      counter = 0;
      for (unsigned int j = 1; j <= n_cell_z; ++j)
        {
          for (unsigned int i = 1; i <= n_cell_t; ++i)
            {
              std::vector<double> cell_connectivity(4);
              cell_connectivity[0] = counter + 1;
              cell_connectivity[1] = counter + 1 + 1;
              cell_connectivity[2] = i + j * n_cell_t + 1;
              cell_connectivity[3] = i + j * n_cell_t;
              if (i == n_cell_t)
                {
                  cell_connectivity[1] = cell_connectivity[1] - n_cell_t;
                  cell_connectivity[2] = cell_connectivity[2] - n_cell_t;
                }
              grid_connectivity[counter][0] = cell_connectivity[1] - 1;
              grid_connectivity[counter][1] = cell_connectivity[0] - 1;
              grid_connectivity[counter][2] = cell_connectivity[3] - 1;
              grid_connectivity[counter][3] = cell_connectivity[2] - 1;
              counter++;
            }
        }
    }
  else if (grid_type == "chunk")
    {


      WBAssertThrow(dim == 3, "The chunk only works in 3d.");

      double inner_radius = z_min;
      double outer_radius = z_max;

      WBAssertThrow(x_min < x_max, "The minimum longitude must be less than the maximum longitude.");
      WBAssertThrow(x_min < x_max, "The minimum latitude must be less than the maximum latitude.");
      WBAssertThrow(inner_radius < outer_radius, "The inner radius must be less than the outer radius.");

      WBAssertThrow(x_min - x_max <= 2.0 * M_PI, "The difference between the minimum and maximum longitude "
                    " must be less than or equal to 360 degree.");

      WBAssertThrow(y_min >= - 0.5 * M_PI, "The minimum latitude must be larger then or equal to -90 degree.");
      WBAssertThrow(y_min <= 0.5 * M_PI, "The maximum latitude must be smaller then or equal to 90 degree.");

      double opening_angle_long_rad = (x_max - x_min);
      double opening_angle_lat_rad =  (y_max - y_min);

      n_cell = n_cell_x * n_cell_y * (dim == 3 ? n_cell_z : 1.0);
      n_p = (n_cell_x + 1) * (n_cell_y + 1) * (dim == 3 ? (n_cell_z + 1) : 1.0);

      double dlong = opening_angle_long_rad / n_cell_x;
      double dlat = opening_angle_lat_rad / n_cell_y;
      double lr = outer_radius - inner_radius;
      double dr = lr / n_cell_z;

      grid_x.resize(n_p);
      grid_y.resize(n_p);
      grid_z.resize(n_p);
      grid_depth.resize(n_p);


      unsigned int counter = 0;
      for (unsigned int i = 1; i <= n_cell_x + 1; ++i)
        for (unsigned int j = 1; j <= n_cell_y + 1; ++j)
          for (unsigned int k = 1; k <= n_cell_z + 1; ++k)
            {
              grid_x[counter] = x_min + (i-1) * dlong;
              grid_y[counter] = y_min + (j-1) * dlat;
              grid_z[counter] = inner_radius + (k-1) * dr;
              grid_depth[counter] = lr - (k-1) * dr;
              counter++;
            }

      for (unsigned int i = 0; i < n_p; ++i)
        {

          double longitude = grid_x[i];
          double latitutde = grid_y[i];
          double radius = grid_z[i];

          grid_x[i] = radius * std::cos(latitutde) * std::cos(longitude);
          grid_y[i] = radius * std::cos(latitutde) * std::sin(longitude);
          grid_z[i] = radius * std::sin(latitutde);
        }


      // compute connectivity. Local to global mapping.
      grid_connectivity.resize(n_cell,std::vector<unsigned int>(8));

      counter = 0;
      for (unsigned int i = 1; i <= n_cell_x; ++i)
        {
          for (unsigned int j = 1; j <= n_cell_y; ++j)
            {
              for (unsigned int k = 1; k <= n_cell_z; ++k)
                {
                  grid_connectivity[counter][0] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j - 1) + k - 1;
                  grid_connectivity[counter][1] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j - 1) + k - 1;
                  grid_connectivity[counter][2] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j    ) + k - 1;
                  grid_connectivity[counter][3] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j    ) + k - 1;
                  grid_connectivity[counter][4] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j - 1) + k;
                  grid_connectivity[counter][5] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j - 1) + k;
                  grid_connectivity[counter][6] = (n_cell_y + 1) * (n_cell_z + 1) * (i    ) + (n_cell_z + 1) * (j    ) + k;
                  grid_connectivity[counter][7] = (n_cell_y + 1) * (n_cell_z + 1) * (i - 1) + (n_cell_z + 1) * (j    ) + k;
                  counter++;
                }
            }
        }
    }
  else if (grid_type == "sphere")
    {

      WBAssertThrow(dim == 3, "The sphere only works in 3d.");


      double inner_radius = z_min;
      double outer_radius = z_max;

      unsigned int n_block = 12;

      unsigned int block_n_cell = n_cell_x*n_cell_x;
      unsigned int block_n_p = (n_cell_x + 1) * (n_cell_x + 1);
      unsigned int block_n_v = 4;


      std::vector<std::vector<double> > block_grid_x(n_block,std::vector<double>(block_n_p));
      std::vector<std::vector<double> > block_grid_y(n_block,std::vector<double>(block_n_p));
      std::vector<std::vector<double> > block_grid_z(n_block,std::vector<double>(block_n_p));
      std::vector<std::vector<std::vector<unsigned int> > > block_grid_connectivity(n_block,std::vector<std::vector<unsigned int> >(block_n_cell,std::vector<unsigned int>(block_n_v)));
      std::vector<std::vector<bool> > block_grid_hull(n_block,std::vector<bool>(block_n_p));

      /**
       * block node layout
       */
      for (unsigned int i_block = 0; i_block < n_block; ++i_block)
        {
          unsigned int block_n_cell_x = n_cell_x;
          unsigned int block_n_cell_y = n_cell_x;
          double Lx = 1.0;
          double Ly = 1.0;

          unsigned int counter = 0;
          for (unsigned int j = 0; j <= block_n_cell_y; ++j)
            {
              for (unsigned int i = 0; i <= block_n_cell_y; ++i)
                {
                  block_grid_x[i_block][counter] = (double) i * Lx / (double) block_n_cell_x;
                  block_grid_y[i_block][counter] = (double) j * Ly / (double) block_n_cell_y;
                  block_grid_z[i_block][counter] = 0.0;
                  counter++;
                }
            }

          counter = 0;
          // using i=1 and j=1 here because i an j are not used in lookup and storage
          // so the code can remain very similar to ghost and the cartesian code.
          for (unsigned int j = 1; j <= block_n_cell_y; ++j)
            {
              for (unsigned int i = 1; i <= block_n_cell_x; ++i)
                {
                  block_grid_connectivity[i_block][counter][0] = i + (j - 1) * (block_n_cell_x + 1) - 1;
                  block_grid_connectivity[i_block][counter][1] = i + 1 + (j - 1) * (block_n_cell_x + 1) - 1;
                  block_grid_connectivity[i_block][counter][2] = i + 1  + j * (block_n_cell_x + 1) - 1;
                  block_grid_connectivity[i_block][counter][3] = i + j * (block_n_cell_x + 1) - 1;
                  counter++;
                }
            }
        }

      /**
       * map blocks
       */
      double radius = 1;

      // four corners
      double xA = -1.0;
      double yA = 0.0;
      double zA = -1.0 / std::sqrt(2.0);

      double xB = 1.0;
      double yB = 0.0;
      double zB = -1.0 / std::sqrt(2.0);

      double xC = 0.0;
      double yC = -1.0;
      double zC = 1.0 / std::sqrt(2.0);

      double xD = 0.0;
      double yD = 1.0;
      double zD = 1.0 / std::sqrt(2.0);

      // middles of faces
      double xM = (xA+xB+xC)/3.0;
      double yM = (yA+yB+yC)/3.0;
      double zM = (zA+zB+zC)/3.0;

      double xN = (xA+xD+xC)/3.0;
      double yN = (yA+yD+yC)/3.0;
      double zN = (zA+zD+zC)/3.0;

      double xP = (xA+xD+xB)/3.0;
      double yP = (yA+yD+yB)/3.0;
      double zP = (zA+zD+zB)/3.0;

      double xQ = (xC+xD+xB)/3.0;
      double yQ = (yC+yD+yB)/3.0;
      double zQ = (zC+zD+zB)/3.0;

      // middle of edges
      double xF = (xB+xC)/2.0;
      double yF = (yB+yC)/2.0;
      double zF = (zB+zC)/2.0;

      double xG = (xA+xC)/2.0;
      double yG = (yA+yC)/2.0;
      double zG = (zA+zC)/2.0;

      double xE = (xB+xA)/2.0;
      double yE = (yB+yA)/2.0;
      double zE = (zB+zA)/2.0;

      double xH = (xD+xC)/2.0;
      double yH = (yD+yC)/2.0;
      double zH = (zD+zC)/2.0;

      double xJ = (xD+xA)/2.0;
      double yJ = (yD+yA)/2.0;
      double zJ = (zD+zA)/2.0;

      double xK = (xD+xB)/2.0;
      double yK = (yD+yB)/2.0;
      double zK = (zD+zB)/2.0;

      // Making sure points A..Q are on a sphere
      project_on_sphere(radius,xA,yA,zA);
      project_on_sphere(radius,xB,yB,zB);
      project_on_sphere(radius,xC,yC,zC);
      project_on_sphere(radius,xD,yD,zD);
      project_on_sphere(radius,xE,yE,zE);
      project_on_sphere(radius,xF,yF,zF);
      project_on_sphere(radius,xG,yG,zG);
      project_on_sphere(radius,xH,yH,zH);
      project_on_sphere(radius,xJ,yJ,zJ);
      project_on_sphere(radius,xK,yK,zK);
      project_on_sphere(radius,xM,yM,zM);
      project_on_sphere(radius,xN,yN,zN);
      project_on_sphere(radius,xP,yP,zP);
      project_on_sphere(radius,xQ,yQ,zQ);

      lay_points(xM,yM,zM,xG,yG,zG,xA,yA,zA,xE,yE,zE,block_grid_x[0], block_grid_y[0], block_grid_z[0],block_grid_hull[0], n_cell_x);
      lay_points(xF,yF,zF,xM,yM,zM,xE,yE,zE,xB,yB,zB,block_grid_x[1], block_grid_y[1], block_grid_z[1],block_grid_hull[1], n_cell_x);
      lay_points(xC,yC,zC,xG,yG,zG,xM,yM,zM,xF,yF,zF,block_grid_x[2], block_grid_y[2], block_grid_z[2],block_grid_hull[2], n_cell_x);
      lay_points(xG,yG,zG,xN,yN,zN,xJ,yJ,zJ,xA,yA,zA,block_grid_x[3], block_grid_y[3], block_grid_z[3],block_grid_hull[3], n_cell_x);
      lay_points(xC,yC,zC,xH,yH,zH,xN,yN,zN,xG,yG,zG,block_grid_x[4], block_grid_y[4], block_grid_z[4],block_grid_hull[4], n_cell_x);
      lay_points(xH,yH,zH,xD,yD,zD,xJ,yJ,zJ,xN,yN,zN,block_grid_x[5], block_grid_y[5], block_grid_z[5],block_grid_hull[5], n_cell_x);
      lay_points(xA,yA,zA,xJ,yJ,zJ,xP,yP,zP,xE,yE,zE,block_grid_x[6], block_grid_y[6], block_grid_z[6],block_grid_hull[6], n_cell_x);
      lay_points(xJ,yJ,zJ,xD,yD,zD,xK,yK,zK,xP,yP,zP,block_grid_x[7], block_grid_y[7], block_grid_z[7],block_grid_hull[7], n_cell_x);
      lay_points(xP,yP,zP,xK,yK,zK,xB,yB,zB,xE,yE,zE,block_grid_x[8], block_grid_y[8], block_grid_z[8],block_grid_hull[8], n_cell_x);
      lay_points(xQ,yQ,zQ,xK,yK,zK,xD,yD,zD,xH,yH,zH,block_grid_x[9], block_grid_y[9], block_grid_z[9],block_grid_hull[9], n_cell_x);
      lay_points(xQ,yQ,zQ,xH,yH,zH,xC,yC,zC,xF,yF,zF,block_grid_x[10], block_grid_y[10], block_grid_z[10],block_grid_hull[10], n_cell_x);
      lay_points(xQ,yQ,zQ,xF,yF,zF,xB,yB,zB,xK,yK,zK,block_grid_x[11], block_grid_y[11], block_grid_z[11],block_grid_hull[11], n_cell_x);

      // make sure all points end up on a sphere
      for (unsigned int i_block = 0; i_block < n_block; ++i_block)
        {
          for (unsigned int i_point = 0; i_point < block_n_p; ++i_point)
            {
              project_on_sphere(radius,block_grid_x[i_block][i_point],block_grid_y[i_block][i_point],block_grid_z[i_block][i_point]);
            }
        }

      /**
       * merge blocks
       */
      std::vector<double> temp_x(n_block * block_n_p);
      std::vector<double> temp_y(n_block * block_n_p);
      std::vector<double> temp_z(n_block * block_n_p);
      std::vector<bool> sides(n_block * block_n_p);

      for (unsigned int i = 0; i < n_block; ++i)
        {
          unsigned int counter = 0;
          for (unsigned int j = i * block_n_p; j < i * block_n_p + block_n_p; ++j)
            {
              WBAssert(j < temp_x.size(), "j should be smaller then the size of the array temp_x.");
              WBAssert(j < temp_y.size(), "j should be smaller then the size of the array temp_y.");
              WBAssert(j < temp_z.size(), "j should be smaller then the size of the array temp_z.");
              temp_x[j] = block_grid_x[i][counter];
              temp_y[j] = block_grid_y[i][counter];
              temp_z[j] = block_grid_z[i][counter];
              sides[j] = block_grid_hull[i][counter];
              counter++;
            }
        }


      std::vector<bool> double_points(n_block * block_n_p,false);
      std::vector<unsigned int> point_to(n_block * block_n_p);

      for (unsigned int i = 0; i < n_block * block_n_p; ++i)
        point_to[i] = i;

      // TODO: This becomes problematic with too large values of outer radius. Find a better way, maybe through an epsilon.
      double distance = 1e-12*outer_radius;

      unsigned int counter = 0;
      unsigned int amount_of_double_points = 0;
      for (unsigned int i = 1; i < n_block * block_n_p; ++i)
        {
          if (sides[i])
            {
              double gxip = temp_x[i];
              double gyip = temp_y[i];
              double gzip = temp_z[i];
              for (unsigned int j = 0; j < i-1; ++j)
                {
                  if (sides[j])
                    {
                      if (std::fabs(gxip-temp_x[j]) < distance &&
                          std::fabs(gyip-temp_y[j]) < distance &&
                          std::fabs(gzip-temp_z[j]) < distance)
                        {
                          double_points[i] = true;
                          point_to[i] = j;
                          amount_of_double_points++;
                          break;
                        }
                    }
                }
            }
        }


      unsigned int shell_n_p = n_block * block_n_p - amount_of_double_points;
      unsigned int shell_n_cell = n_block * block_n_cell;
      unsigned int shell_n_v = block_n_v;

      std::vector<double> shell_grid_x(shell_n_p);
      std::vector<double> shell_grid_y(shell_n_p);
      std::vector<double> shell_grid_z(shell_n_p);
      std::vector<std::vector<unsigned int> > shell_grid_connectivity(shell_n_cell,std::vector<unsigned int>(shell_n_v));

      counter = 0;
      for (unsigned int i = 0; i < n_block * block_n_p; ++i)
        {
          if (!double_points[i])
            {
              shell_grid_x[counter] = temp_x[i];
              shell_grid_y[counter] = temp_y[i];
              shell_grid_z[counter] = temp_z[i];

              counter++;
            }
        }

      for (unsigned int i = 0; i < n_block; ++i)
        {
          counter = 0;
          for (unsigned int j = i * block_n_cell; j < i * block_n_cell + block_n_cell; ++j)
            {
              for (unsigned int k = 0; k < shell_n_v; ++k)
                {
                  shell_grid_connectivity[j][k] = block_grid_connectivity[i][counter][k] + i * block_n_p;
                }
              counter++;
            }
        }

      for (unsigned int i = 0; i < shell_n_cell; ++i)
        {
          for (unsigned int j = 0; j < shell_n_v; ++j)
            {
              shell_grid_connectivity[i][j] = point_to[shell_grid_connectivity[i][j]];
            }
        }

      std::vector<unsigned int> compact(n_block * block_n_p);

      counter = 0;
      for (unsigned int i = 0; i < n_block * block_n_p; ++i)
        {
          if (!double_points[i])
            {
              compact[i] = counter;
              counter++;
            }
        }


      for (unsigned int i = 0; i < shell_n_cell; ++i)
        {
          for (unsigned int j = 0; j < shell_n_v; ++j)
            {
              shell_grid_connectivity[i][j] = compact[shell_grid_connectivity[i][j]];
            }
        }


      /**
       * build hollow sphere
       */

      std::vector<double> temp_shell_grid_x(shell_n_p);
      std::vector<double> temp_shell_grid_y(shell_n_p);
      std::vector<double> temp_shell_grid_z(shell_n_p);

      unsigned int n_v = shell_n_v * 2;
      n_p = (n_cell_z + 1) * shell_n_p;
      n_cell = (n_cell_z) * shell_n_cell;

      grid_x.resize(n_p);
      grid_y.resize(n_p);
      grid_z.resize(n_p);
      grid_depth.resize(n_p);
      grid_connectivity.resize(n_cell,std::vector<unsigned int>(n_v));


      for (unsigned int i = 0; i < n_cell_z + 1; ++i)
        {
          temp_shell_grid_x = shell_grid_x;
          temp_shell_grid_y = shell_grid_y;
          temp_shell_grid_z = shell_grid_z;

          // We do not need to copy the shell_grid_connectivity into a
          // temperorary variable, because we do not change it. We can
          // directly use it.

          radius = inner_radius + ((outer_radius - inner_radius) / n_cell_z) * i;
          for (unsigned int j = 0; j < shell_n_p; ++j)
            {
              WBAssert(j < temp_shell_grid_x.size(), "ERROR: j = " << j << ", temp_shell_grid_x.size() = " << temp_shell_grid_x.size());
              WBAssert(j < temp_shell_grid_y.size(), "ERROR: j = " << j << ", temp_shell_grid_y.size() = " << temp_shell_grid_y.size());
              WBAssert(j < temp_shell_grid_z.size(), "ERROR: j = " << j << ", temp_shell_grid_z.size() = " << temp_shell_grid_z.size());
              project_on_sphere(radius, temp_shell_grid_x[j], temp_shell_grid_y[j], temp_shell_grid_z[j]);
            }

          unsigned int i_beg =  i * shell_n_p;
          unsigned int i_end = (i+1) * shell_n_p;
          counter = 0;
          for (unsigned int j = i_beg; j < i_end; ++j)
            {
              WBAssert(j < grid_x.size(), "ERROR: j = " << j << ", grid_x.size() = " << grid_x.size());
              WBAssert(j < grid_y.size(), "ERROR: j = " << j << ", grid_y.size() = " << grid_y.size());
              WBAssert(j < grid_z.size(), "ERROR: j = " << j << ", grid_z.size() = " << grid_z.size());
              grid_x[j] = temp_shell_grid_x[counter];
              grid_y[j] = temp_shell_grid_y[counter];
              grid_z[j] = temp_shell_grid_z[counter];
              grid_depth[j] = outer_radius - std::sqrt(grid_x[j] * grid_x[j] + grid_y[j] * grid_y[j] + grid_z[j] * grid_z[j]);

              counter++;
            }
        }

      for (unsigned int i = 0; i < n_cell_z; ++i)
        {
          unsigned int i_beg = i * shell_n_cell;
          unsigned int i_end = (i+1) * shell_n_cell;
          counter = 0;
          for (unsigned int j = i_beg; j < i_end; ++j)
            {
              for (unsigned int k = 0; k < shell_n_v; ++k)
                {
                  grid_connectivity[j][k] = shell_grid_connectivity[counter][k] + i * shell_n_p;
                }
              counter++;
            }


          counter = 0;
          for (unsigned int j = i_beg; j < i_end; ++j)
            {
              for (unsigned int k = shell_n_v ; k < 2 * shell_n_v; ++k)
                {
                  WBAssert(k-shell_n_v < shell_grid_connectivity[counter].size(), "k - shell_n_v is larger then shell_grid_connectivity[counter]: k= " << k << ", shell_grid_connectivity[counter].size() = " << shell_grid_connectivity[counter].size());
                  grid_connectivity[j][k] = shell_grid_connectivity[counter][k-shell_n_v] + (i+1) * shell_n_p;
                }
              counter++;
            }
        }
    }

  // create paraview file.
  std::ofstream myfile;
  myfile.open ("example.vtu");
  myfile << "<?xml version=\"1.0\" ?> " << std::endl;
  myfile << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl;
  myfile << "<UnstructuredGrid>" << std::endl;
  myfile << "<FieldData>" << std::endl;
  myfile << "<DataArray type=\"Float32\" Name=\"TIME\" NumberOfTuples=\"1\" format=\"ascii\">0</DataArray>" << std::endl;
  myfile << "</FieldData>" << std::endl;
  myfile << "<Piece NumberOfPoints=\""<< n_p << "\" NumberOfCells=\"" << n_cell << "\">" << std::endl;
  myfile << "  <Points>" << std::endl;
  myfile << "    <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" << std::endl;
  if (dim == 2)
    for (unsigned int i = 0; i < n_p; ++i)
      myfile << grid_x[i] << " " << grid_z[i] << " " << "0.0" << std::endl;
  else
    for (unsigned int i = 0; i < n_p; ++i)
      myfile << grid_x[i] << " " << grid_y[i] << " " << grid_z[i] << std::endl;
  myfile << "    </DataArray>" << std::endl;
  myfile << "  </Points>" << std::endl;
  myfile << std::endl;
  myfile << "  <Cells>" << std::endl;
  myfile << "    <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" << std::endl;
  if (dim == 2)
    for (unsigned int i = 0; i < n_cell; ++i)
      myfile << grid_connectivity[i][0] << " " <<grid_connectivity[i][1] << " " << grid_connectivity[i][2] << " " << grid_connectivity[i][3] << std::endl;
  else
    for (unsigned int i = 0; i < n_cell; ++i)
      myfile << grid_connectivity[i][0] << " " <<grid_connectivity[i][1] << " " << grid_connectivity[i][2] << " " << grid_connectivity[i][3]  << " "
             << grid_connectivity[i][4] << " " <<grid_connectivity[i][5] << " " << grid_connectivity[i][6] << " " << grid_connectivity[i][7]<< std::endl;
  myfile << "    </DataArray>" << std::endl;
  myfile << "    <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" << std::endl;
  if (dim == 2)
    for (unsigned int i = 1; i <= n_cell; ++i)
      myfile << i * 4 << " ";
  else
    for (unsigned int i = 1; i <= n_cell; ++i)
      myfile << i * 8 << " ";
  myfile << std::endl << "    </DataArray>" << std::endl;
  myfile << "    <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" << std::endl;
  if (dim == 2)
    for (unsigned int i = 0; i < n_cell; ++i)
      myfile << "9" << " ";
  else
    for (unsigned int i = 0; i < n_cell; ++i)
      myfile << "12" << " ";
  myfile <<  std::endl <<"    </DataArray>" << std::endl;
  myfile << "  </Cells>" << std::endl;

  myfile << "  <PointData Scalars=\"scalars\">" << std::endl;

  /*myfile << "<DataArray type=\"Float32\" Name=\"Depth\" format=\"ascii\">" << std::endl;

  for (unsigned int i = 0; i < n_p; ++i)
    {
      myfile <<  grid_depth[i] << std::endl;
    }
  myfile << "</DataArray>" << std::endl;*/

  myfile << "    <DataArray type=\"Float32\" Name=\"T\" format=\"ascii\">" << std::endl;
  if (dim == 2)
    {
      for (unsigned int i = 0; i < n_p; ++i)
        {
          std::array<double,2> coords = {grid_x[i], grid_z[i]};
          myfile <<  world->temperature(coords, grid_depth[i], gravity) << std::endl;
        }
    }
  else
    {
      for (unsigned int i = 0; i < n_p; ++i)
        {
          std::array<double,3> coords = {grid_x[i], grid_y[i], grid_z[i]};
          myfile <<  world->temperature(coords, grid_depth[i], gravity) << std::endl;
        }
    }
  myfile << "    </DataArray>" << std::endl;

  /*for (unsigned int c = 0; c < compositions; ++c)
    {
      myfile << "<DataArray type=\"Float32\" Name=\"Composition " << c << "\" Format=\"ascii\">" << std::endl;
      if (dim == 2)
        {
          for (unsigned int i = 0; i < n_p; ++i)
            {
              std::array<double,2> coords = {grid_x[i], grid_z[i]};
              myfile <<  world->composition(coords, grid_depth[i], c) << std::endl;
            }
        }
      else
        {
          for (unsigned int i = 0; i < n_p; ++i)
            {
              std::array<double,3> coords = {grid_x[i], grid_y[i], grid_z[i]};
              myfile <<  world->composition(coords, grid_depth[i], c) << std::endl;
            }
        }
      myfile << "</DataArray>" << std::endl;
    }*/

  myfile << "  </PointData>" << std::endl;


  myfile << " </Piece>" << std::endl;
  myfile << " </UnstructuredGrid>" << std::endl;
  myfile << "</VTKFile>" << std::endl;
  /*
  switch(dim)
  {
  case 2:
    // set the header
    std::cout << "# x z d T ";

    for(unsigned int c = 0; c < compositions; ++c)
      std::cout << "c" << c << " ";

    std::cout <<std::endl;

    // set the values
    for(unsigned int i = 0; i < data.size(); ++i)
      if(data[i][0] != "#")
      {

        WBAssertThrow(data[i].size() == dim + 2, "The file needs to contain dim + 2 entries, but contains " << data[i].size() << " entries "
            " on line " << i+1 << " of the data file.  Dim is " << dim << ".");

        std::array<double,2> coords = {string_to_double(data[i][0]),
            string_to_double(data[i][1])};
        std::cout << data[i][0] << " " << data[i][1] << " " << data[i][2] << " " << data[i][3] << " ";
        //std::cout << world->temperature(coords, string_to_double(data[i][2]), string_to_double(data[i][3]))  << " ";

        for(unsigned int c = 0; c < compositions; ++c)
        {
          //std::cout << world->composition(coords, string_to_double(data[i][2]), c)  << " ";
        }
        std::cout << std::endl;

      }
    break;
  case 3:
    // set the header
    std::cout << "# x y z d T ";

    for(unsigned int c = 0; c < compositions; ++c)
      std::cout << "c" << c << " ";

    std::cout <<std::endl;

    // set the values
    for(unsigned int i = 0; i < data.size(); ++i)
      if(data[i][0] != "#")
      {
        WBAssertThrow(data[i].size() == dim + 2, "The file needs to contain dim + 2 entries, but contains " << data[i].size() << " entries "
            " on line " << i+1 << " of the data file. Dim is " << dim << ".");
        std::array<double,3> coords = {string_to_double(data[i][0]),
            string_to_double(data[i][1]),
            string_to_double(data[i][3])};

        std::cout << data[i][0] << " " << data[i][1] << " " << data[i][2] << " " << data[i][3] << " " << data[i][4] << " ";
        //std::cout << world->temperature(coords, string_to_double(data[i][3]), string_to_double(data[i][4]))  << " ";

        for(unsigned int c = 0; c < compositions; ++c)
        {
          //std::cout << world->composition(coords, string_to_double(data[i][3]), c)  << " ";
        }
        std::cout << std::endl;

      }
    break;
  default:
    std::cout << "The World Builder can only be run in 2d and 3d but a different space dimension " << std::endl
    << "is given: dim = " << dim << ".";
    return 0;
  }
   */

  return 0;
}
