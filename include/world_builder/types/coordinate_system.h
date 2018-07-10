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

#ifndef _world_feature_types_coordinate_systems_h
#define _world_feature_types_coordinate_systems_h

#include <world_builder/types/interface.h>
#include <world_builder/coordinate_systems/interface.h>


namespace WorldBuilder
{
  namespace Types
  {

    /**
     * This class represents a continental plate and can implement submodules
     * for temperature and composition. These submodules determine what
     * the returned temperature or composition of the temperature and composition
     * functions of this class will be.
     */
    class CoordinateSystem : public Interface
    {
      public:
        /**
         * constructor
         */
    	CoordinateSystem(std::string default_value, std::string description);

        /**
         * Destructor
         */
        ~CoordinateSystem();

        /**
         * clone
         */
        virtual
        std::shared_ptr<Interface> clone() const;

        /**
         * Set value
         */
        virtual
        void set_value(std::string value);

        //std::string value;
        std::shared_ptr<::WorldBuilder::CoordinateSystems::Interface> value;
        std::string default_value;
        std::string description;

      private:

    };
  }
}

#endif
