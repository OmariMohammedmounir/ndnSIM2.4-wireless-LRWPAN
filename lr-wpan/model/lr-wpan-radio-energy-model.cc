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
 *  Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu> (wifi-radio-energy-model)
 *  Philip Hönnecke <p.hoennecke@tu-braunschweig.de> (this code)
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "lr-wpan-radio-energy-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (LrWpanRadioEnergyModel);

TypeId
LrWpanRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::LrWpanRadioEnergyModel")
          .SetParent<DeviceEnergyModel> ()
          .SetGroupName ("Energy")
          .AddConstructor<LrWpanRadioEnergyModel> ()
          .AddAttribute ("IdleCurrentA", "The default radio Idle current in Ampere.",
                         DoubleValue (0.0013),
                         MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetIdleCurrentA,
                                             &LrWpanRadioEnergyModel::GetIdleCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("TxCurrentA", "The default radio Tx current in Ampere.",
                         DoubleValue (0.03),
                         MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTxCurrentA,
                                             &LrWpanRadioEnergyModel::GetTxCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("RxCurrentA", "The default radio Rx current in Ampere.",
                         DoubleValue (0.038),
                         MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetRxCurrentA,
                                             &LrWpanRadioEnergyModel::GetRxCurrentA),
                         MakeDoubleChecker<double> ())
          .AddTraceSource (
              "TotalEnergyConsumption", "Total energy consumption of the radio device.",
              MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_totalEnergyConsumption),
              "ns3::TracedValueCallback::Double")
          .AddTraceSource (
              "CurrentState", "This DeviceEnergyModel's current LrWpanRadioEnergyState.",
              MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_currentState),
              "ns3::TracedValueCallback::LrWpanRadioEnergyState")
          .AddTraceSource (
              "CurrentA", "This DeviceEnergyModel's current in A.",
              MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_currentA),
              "ns3::TracedValueCallback::Double");
  return tid;
}

LrWpanRadioEnergyModel::LrWpanRadioEnergyModel ()
    : m_source (0),
      m_currentState (LrWpanRadioEnergyState::Idle),
      m_lastUpdateTime (Seconds (0.0)),
      m_nPendingChangeState (0)
{
  NS_LOG_FUNCTION (this);
  m_energyDepleted = false;
  m_energyDepletionCallback.Nullify ();
}

LrWpanRadioEnergyModel::~LrWpanRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
}

void
LrWpanRadioEnergyModel::SetEnergySource (const Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
  UpdateSwitchToOffEvent (m_currentState);
}

void
LrWpanRadioEnergyModel::SetLrWpanPhy (Ptr<LrWpanPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  NS_ASSERT (phy != NULL);
  m_phy = phy;

  // Energy depletion and recharged callbacks
  SetEnergyDepletionCallback (MakeCallback (&LrWpanPhy::SetEnergyOff, phy));
  SetEnergyRechargedCallback (MakeCallback (&LrWpanPhy::SetEnergyOn, phy));

  phy->TraceConnectWithoutContext ("TrxStateValue",
                                   MakeCallback (&LrWpanRadioEnergyModel::TrxStateChanged, this));
}

double
LrWpanRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = duration.GetSeconds () * GetStateA (m_currentState) * supplyVoltage;

  // notify energy source
  m_source->UpdateEnergySource ();

  return m_totalEnergyConsumption + energyToDecrease;
}

LrWpanRadioEnergyModel::LrWpanRadioEnergyState
LrWpanRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
LrWpanRadioEnergyModel::SetEnergyDepletionCallback (LrWpanRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
LrWpanRadioEnergyModel::SetEnergyRechargedCallback (LrWpanRadioEnergyRechargedCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Setting NULL energy recharged callback!");
    }
  m_energyRechargedCallback = callback;
}

// void
// LrWpanRadioEnergyModel::SetTxCurrentModel (const Ptr<LrWpanTxCurrentModel> model)
// {
//   NS_LOG_FUNCTION (this);
//   m_txCurrentModel = model;
// }

Time
LrWpanRadioEnergyModel::GetMaximumTimeInState (int state) const
{
  NS_LOG_FUNCTION (this << state);
  if (state == LrWpanRadioEnergyState::Off)
    {
      NS_FATAL_ERROR ("Requested maximum remaining time for OFF state");
    }
  double remainingEnergy = m_source->GetRemainingEnergy ();
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double current = GetStateA (state);
  if (current == 0 || supplyVoltage == 0)
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel: The supply voltage or the current in Ampere for state "
                    << state << " is 0.");
      return Time::Max ();
    }
  double secsLeft = remainingEnergy / (current * supplyVoltage);
  // Return Time::Max () if the amount of seconds it too high
  // If we wouldn't do this, the returned Time value could end up being negative
  if (secsLeft > Time::Max ().GetSeconds ())
    {
      return Time::Max ();
    }
  else
    {
      Time ret = Seconds (secsLeft);
      NS_ASSERT (ret.IsPositive ());
      return ret;
    }
}

void
LrWpanRadioEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  m_nPendingChangeState++;

  if (m_nPendingChangeState > 1 && (newState == LrWpanRadioEnergyState::Off))
    {
      SetLrWpanRadioEnergyState ((LrWpanRadioEnergyState) newState);
      m_nPendingChangeState--;
      return;
    }

  if (newState != LrWpanRadioEnergyState::Off)
    {
      UpdateSwitchToOffEvent ((LrWpanRadioEnergyState) newState);
    }

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = (duration * GetStateA (m_currentState) * supplyVoltage).GetSeconds ();

  // update total energy consumption
  m_totalEnergyConsumption += energyToDecrease;

  // update last update time stamp
  m_lastUpdateTime = Simulator::Now ();

  // notify energy source
  m_source->UpdateEnergySource ();

  // in case the energy source is found to be depleted during the last update, a callback might be
  // invoked that might cause a change in the LR-WPAN PHY state.
  // This in turn causes a new call to this member function, with the consequence that the previous
  // instance is resumed after the termination of the new instance. In particular, the state set
  // by the previous instance is erroneously the final state stored in m_currentState. The check below
  // ensures that previous instances do not change m_currentState.

  if (m_nPendingChangeState <= 1 && (!m_energyDepleted))
    {
      // update current state & last update time stamp
      SetLrWpanRadioEnergyState ((LrWpanRadioEnergyState) newState);

      // some debug message
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Total energy consumption is "
                    << m_totalEnergyConsumption << "J");
    }

  m_nPendingChangeState--;
}

void
LrWpanRadioEnergyModel::TrxStateChanged (LrWpanPhyEnumeration oldState,
                                         LrWpanPhyEnumeration newState)
{
  NS_LOG_FUNCTION (this << oldState << newState);
  switch (newState)
    {
    case IEEE_802_15_4_PHY_BUSY_TX:
      ChangeState (Tx);
      break;

    case IEEE_802_15_4_PHY_BUSY_RX:
      ChangeState (Rx);
      break;

    case IEEE_802_15_4_PHY_RX_ON:
      NS_LOG_LOGIC ("LrWpanRadioEnergyModel: The connected LrWpanPhy is getting ready to receive. "
                    "LrWpanRadioEnergyState is Idle.");
      ChangeState (Idle);
      break;

    case IEEE_802_15_4_PHY_TX_ON:
      NS_LOG_LOGIC ("LrWpanRadioEnergyModel: The connected LrWpanPhy is getting ready to "
                    "transmit. LrWpanRadioEnergyState is Idle.");
      ChangeState (Idle);
      break;

    case IEEE_802_15_4_PHY_IDLE:
      ChangeState (Idle);
      break;

    case IEEE_802_15_4_PHY_TRX_OFF:
    case IEEE_802_15_4_PHY_FORCE_TRX_OFF:
      ChangeState (Off);
      break;

    default:
      NS_LOG_LOGIC ("LrWpanRadioEnergyModel: The connected LrWpanPhy is changing its state to "
                    << newState << ". Doing nothing.");
      break;
    }
}

void
LrWpanRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Energy is depleted!");
  m_energyDepleted = true;
  ChangeState (Off);
  // invoke energy depletion callback, if set.
  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
}

void
LrWpanRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Energy is recharged!");
  m_energyDepleted = false;
  ChangeState (Idle);
  // invoke energy recharged callback, if set.
  if (!m_energyRechargedCallback.IsNull ())
    {
      m_energyRechargedCallback ();
    }
}

void
LrWpanRadioEnergyModel::HandleEnergyChanged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Energy is changed!");
  // No update if there is currently a state change happening (state is going to change anyway)
  if (m_nPendingChangeState == 0 && m_currentState != Off)
    {
      UpdateSwitchToOffEvent (m_currentState);
    }
}

double
LrWpanRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxCurrentA;
}

void
LrWpanRadioEnergyModel::SetRxCurrentA (double currentA)
{
  NS_LOG_FUNCTION (this);
  m_rxCurrentA = currentA;
}

double
LrWpanRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txCurrentA;
}

void
LrWpanRadioEnergyModel::SetTxCurrentA (double currentA)
{
  NS_LOG_FUNCTION (this);
  m_txCurrentA = currentA;
}

double
LrWpanRadioEnergyModel::GetIdleCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_idleCurrentA;
}

void
LrWpanRadioEnergyModel::SetIdleCurrentA (double currentA)
{
  NS_LOG_FUNCTION (this);
  m_idleCurrentA = currentA;
}

/*
 * Private functions start here.
 */

void
LrWpanRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}

double
LrWpanRadioEnergyModel::GetStateA (int state) const
{
  NS_LOG_FUNCTION (this << state);
  switch (state)
    {
    case LrWpanRadioEnergyState::Idle:
      return m_idleCurrentA;
    case LrWpanRadioEnergyState::Tx:
      return m_txCurrentA;
    case LrWpanRadioEnergyState::Rx:
      return m_rxCurrentA;
    case LrWpanRadioEnergyState::Off:
      return 0.0;
    }
  NS_FATAL_ERROR ("LrWpanRadioEnergyModel: Undefined radio state " << state);
}

double
LrWpanRadioEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return GetStateA (m_currentState);
}

void
LrWpanRadioEnergyModel::SetLrWpanRadioEnergyState (LrWpanRadioEnergyState state)
{
  NS_LOG_FUNCTION (this << state);
  std::string oldStateName, newStateName;
  switch (m_currentState)
    {
    case Tx:
      oldStateName = "TX";
      break;
    case Rx:
      oldStateName = "RX";
      break;
    case Idle:
      oldStateName = "IDLE";
      break;
    case Off:
      oldStateName = "OFF";
      break;
    case Undefined:
      NS_FATAL_ERROR ("LrWpanRadioEnergyModel: Switching from state Undefined is not allowed!");
    }
  switch (state)
    {
    case Tx:
      newStateName = "TX";
      break;
    case Rx:
      newStateName = "RX";
      break;
    case Idle:
      newStateName = "IDLE";
      break;
    case Off:
      newStateName = "OFF";
      m_switchToOffEvent.Cancel ();
      break;
    case Undefined:
      NS_FATAL_ERROR ("LrWpanRadioEnergyModel: Switching state to Undefined is not allowed!");
    }
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Switching from state "
                << oldStateName << " to " << newStateName << " at time = " << Simulator::Now ());
  m_currentState = state;
  m_currentA = DoGetCurrentA ();
}

void
LrWpanRadioEnergyModel::UpdateSwitchToOffEvent (LrWpanRadioEnergyState state)
{
  NS_LOG_FUNCTION (this);

  Time durationToOff = GetMaximumTimeInState (state);

  // Scheduling the event directly with GetMaximumTimeInState as the delay often resulted in errors.
  // That might be due to rounding errors or something else.
  // Accordingly, a small duration is subtracted from the delay so that the event is scheduled shortly before the calculated time
  if (durationToOff > MilliSeconds (1))
    {
      durationToOff -= MilliSeconds (1);
    }
  else
    {
      durationToOff -= durationToOff * .01;
    }

  // Create new switchToOffEvent if event is expired or new event time is different to current one
  if (m_switchToOffEvent.IsExpired () ||
      Time (m_switchToOffEvent.GetTs ()) != Simulator::Now () + durationToOff)
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel: Scheduling state change to Off in " << durationToOff);
      m_switchToOffEvent.Cancel ();
      m_switchToOffEvent =
          Simulator::Schedule (durationToOff, &EnergySource::UpdateEnergySource, m_source);
    }
}

} // namespace ns

