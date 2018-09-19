/*
  Copyright (C) 2018 by the authors of the World Builder code.

  This file is part of the World Builder.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <world_builder/types/segment.h>
#include <world_builder/assert.h>
#include <world_builder/utilities.h>

namespace WorldBuilder
{
  namespace Types
  {
    Segment::Segment(double default_value_length,
                     WorldBuilder::Point<2> default_value_thickness,
                     WorldBuilder::Point<2> default_value_angle,
                     std::string description)
      :
      value_length(default_value_length),
      default_value_length(default_value_length),
      value_thickness(default_value_thickness),
      default_value_thickness(default_value_thickness),
      value_angle(default_value_angle),
      default_value_angle(default_value_angle),
      description(description)
    {
      this->type_name = Types::type::Segment;
    }

    Segment::Segment(double   value_length,
                     double   default_value_length,
                     WorldBuilder::Point<2> value_thickness,
                     WorldBuilder::Point<2> default_value_thickness,
                     WorldBuilder::Point<2> value_angle,
                     WorldBuilder::Point<2> default_value_angle,
                     std::string description)
      :
      value_length(value_length),
      default_value_length(default_value_length),
      value_thickness(value_thickness),
      default_value_thickness(default_value_thickness),
      value_angle(value_angle),
      default_value_angle(default_value_angle),
      description(description)
    {
      this->type_name = Types::type::Segment;

    }

    Segment::~Segment ()
    {}

    std::unique_ptr<Interface>
    Segment::clone() const
    {
      return std::unique_ptr<Interface>(new Segment(value_length, default_value_length,
                                                    value_thickness, default_value_thickness,
                                                    value_angle, default_value_angle,
                                                    description));
    }

  }
}

