/*
 * Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>

 * This program is free software; you can  redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version. This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Ekiga is licensed under the GPL license and as a special exception, you
 * have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination, without
 * applying the requirements of the GNU GPL to the OPAL, OpenH323 and PWLIB
 * programs, as long as you do follow the requirements of the GNU GPL for all
 * the rest of the software thus combined.
 */


/*
 *                         hal-core.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Matthias Schneider
 *   copyright            : (c) 2008 by Matthias Schneider
 *   description          : declaration of the interface of a hal core.
 *                          A hal core manages a HalManager.
 *
 */


#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include "hal-core.h"
#include "hal-manager.h"


using namespace Ekiga;

HalCore::HalCore ()
{
}


HalCore::~HalCore ()
{
#if DEBUG
  std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
#endif
}


void HalCore::add_manager (HalManager &manager)
{
  managers.insert (&manager);
  manager_added (manager);

  manager.audioinput_device_added.connect (boost::bind (&HalCore::on_audioinput_device_added, this, _1, _2, &manager));
  manager.audioinput_device_removed.connect (boost::bind (&HalCore::on_audioinput_device_removed, this, _1, _2, &manager));

  manager.audiooutput_device_added.connect (boost::bind (&HalCore::on_audiooutput_device_added, this, _1, _2, &manager));
  manager.audiooutput_device_removed.connect (boost::bind (&HalCore::on_audiooutput_device_removed, this, _1, _2, &manager));

  manager.network_interface_up.connect (boost::bind (&HalCore::on_network_interface_up, this, _1, _2, &manager));
  manager.network_interface_down.connect (boost::bind (&HalCore::on_network_interface_down, this, _1, _2, &manager));
}


void HalCore::visit_managers (boost::function1<bool, HalManager &> visitor) const
{
  bool go_on = true;

  for (std::set<HalManager *>::const_iterator iter = managers.begin ();
       iter != managers.end () && go_on;
       iter++)
      go_on = visitor (*(*iter));
}

void HalCore::on_audioinput_device_added (std::string source, std::string device, HalManager* manager) {
  audioinput_device_added (source, device, manager);
}

void HalCore::on_audioinput_device_removed (std::string source, std::string device, HalManager* manager) {
  audioinput_device_removed (source, device, manager);
}

void HalCore::on_audiooutput_device_added (std::string sink, std::string device, HalManager* manager) {
  audiooutput_device_added (sink, device, manager);
}

void HalCore::on_audiooutput_device_removed (std::string sink, std::string device, HalManager* manager) {
  audiooutput_device_removed (sink, device, manager);
}

void HalCore::on_network_interface_up (std::string interface_name, std::string ip4_address, HalManager* manager) {
  network_interface_up (interface_name, ip4_address, manager);
}

void HalCore::on_network_interface_down (std::string interface_name, std::string ip4_address, HalManager* manager) {
  network_interface_down (interface_name,ip4_address, manager);
}
