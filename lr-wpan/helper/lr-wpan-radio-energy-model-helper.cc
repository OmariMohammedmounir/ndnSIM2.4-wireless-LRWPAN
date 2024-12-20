/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Institute of Operating Systems and Computer Networks, TU Braunschweig
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *  Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu> (wifi-radio-energy-model-helper)
 *  Philip Hönnecke <p.hoennecke@tu-braunschweig.de> (this code)
 */

#include "lr-wpan-radio-energy-model-helper.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/lr-wpan-phy.h"

namespace ns3 {

LrWpanRadioEnergyModelHelper::LrWpanRadioEnergyModelHelper ()
{
  m_radioEnergy.SetTypeId ("ns3::LrWpanRadioEnergyModel");
  m_depletionCallback.Nullify ();
  m_rechargedCallback.Nullify ();
}

void
LrWpanRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_radioEnergy.Set (name, v);
}

void
LrWpanRadioEnergyModelHelper::SetDepletionCallback (
    LrWpanRadioEnergyModel::LrWpanRadioEnergyDepletionCallback callback)
{
  m_depletionCallback = callback;
}

void
LrWpanRadioEnergyModelHelper::SetRechargedCallback (
    LrWpanRadioEnergyModel::LrWpanRadioEnergyRechargedCallback callback)
{
  m_rechargedCallback = callback;
}

/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
LrWpanRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device, Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);
  // check if device is LrWpanNetDevice
  std::string deviceName = device->GetInstanceTypeId ().GetName ();
  if (deviceName.compare ("ns3::LrWpanNetDevice") != 0)
    {
      NS_FATAL_ERROR ("NetDevice type is not LrWpanNetDevice!");
    }
  Ptr<Node> node = device->GetNode ();
  Ptr<LrWpanRadioEnergyModel> model = m_radioEnergy.Create ()->GetObject<LrWpanRadioEnergyModel> ();
  NS_ASSERT (model != NULL);

  // set energy depletion callback
  // if none is specified, make a callback to LrWpanPhy::SetEnergyOff
  Ptr<LrWpanNetDevice> lrWpanDevice = DynamicCast<LrWpanNetDevice> (device);
  Ptr<LrWpanPhy> lrWpanPhy = lrWpanDevice->GetPhy ();

  // add model to device model list in energy source
  source->AppendDeviceEnergyModel (model);
  // set energy source pointer
  model->SetEnergySource (source);

  // Set the phy
  model->SetLrWpanPhy (lrWpanPhy);

  return model;
}

} // namespace ns3

