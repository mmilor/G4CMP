#include "ChargeElectrodeSensitivity.hh"
#include "ChargeFETDigitizerModule.hh"
#include "G4CMPElectrodeSensitivity.hh"
#include "G4CMPElectrodeHit.hh"
#include "G4CMPConfigManager.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4TouchableHistory.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4SDManager.hh"
#include "G4Navigator.hh"
#include "G4ios.hh"

#include <fstream>

ChargeElectrodeSensitivity::ChargeElectrodeSensitivity(G4String name)
:G4CMPElectrodeSensitivity(name), FET(new ChargeFETDigitizerModule("FETSim"))
{}

void ChargeElectrodeSensitivity::Initialize(G4HCofThisEvent *HCE)
{
  //Prepare output file.
  if (output.is_open()) output.close();
  output.open(G4CMPConfigManager::GetHitOutput(), std::ios_base::app);
  if (!output.good()) {
    G4ExceptionDescription msg;
    msg << "Error opening output file, " << fileName << ".\n"
        << "Will continue simulation.";
    G4Exception("ChargeElectrodeSensitivity::Initialize", "Charge003",
                JustWarning, msg);
    output.close();
  } else {
    output << "Run ID,Event ID,Track ID,Charge,Start Energy [eV],"
           << "Track Lifetime [ns],Energy Deposit [eV],Start X [m],Start Y [m],"
           << "Start Z [m],End X [m],End Y [m],End Z [m]"
           << G4endl;
  }
  //Call base class initialization.
  G4CMPElectrodeSensitivity::Initialize(HCE);

  //Initialize FETSim.
  if (FET->FETSimIsEnabled())
    FET->Initialize();
}

ChargeElectrodeSensitivity::~ChargeElectrodeSensitivity()
{
  if (output.is_open()) output.close();
  if (!output.good()) {
    G4ExceptionDescription msg;
    msg << "Error closing output file, " << fileName << ".\n"
        << "Expect bad things like loss of data.";
    G4Exception("ChargeElectrodeSensitivity::~ChargeElectrodSensitivity",
                "Charge004", JustWarning, msg);
  }
  delete FET;
}

void ChargeElectrodeSensitivity::EndOfEvent(G4HCofThisEvent* HCE)
{
  G4CMPElectrodeHitsCollection* hitCol =
        static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(GetHCID()));
  std::vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();
  std::vector<G4CMPElectrodeHit*>::iterator itr = hitVec->begin();
  G4RunManager* runMan = G4RunManager::GetRunManager();
  if (output.good()) {
    for (; itr != hitVec->end(); itr++) {
      output << runMan->GetCurrentRun()->GetRunID() << ','
             << runMan->GetCurrentEvent()->GetEventID() << ','
             << (*itr)->GetTrackID() << ','
             << (*itr)->GetCharge() << ','
             << (*itr)->GetStartEnergy()/eV << ','
             << (*itr)->GetFinalTime()/ns << ','
             << (*itr)->GetEnergyDeposit()/eV << ','
             << (*itr)->GetStartPosition().getX()/m << ','
             << (*itr)->GetStartPosition().getY()/m << ','
             << (*itr)->GetStartPosition().getZ()/m << ','
             << (*itr)->GetFinalPosition().getX()/m << ','
             << (*itr)->GetFinalPosition().getY()/m << ','
             << (*itr)->GetFinalPosition().getZ()/m << G4endl;
    }
  }
  FET->Digitize();
}

