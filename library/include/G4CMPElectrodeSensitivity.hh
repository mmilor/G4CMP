/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef G4CMPElectrodeSensitivity_h
#define G4CMPElectrodeSensitivity_h 1

#include "G4VSensitiveDetector.hh"
#include "G4CMPElectrodeHit.hh"

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

class G4CMPElectrodeSensitivity : public G4VSensitiveDetector {
public:
  G4CMPElectrodeSensitivity(G4String);
  virtual ~G4CMPElectrodeSensitivity();
  
  virtual void Initialize(G4HCofThisEvent*);
  virtual G4bool ProcessHits(G4Step*,G4TouchableHistory*);
  virtual void EndOfEvent(G4HCofThisEvent*);
  
  G4CMPElectrodeHitsCollection* getHitsCollection();
  static G4CMPElectrodeHitsCollection* hitsCollection;

protected:
  inline G4int GetHCID() { return HCID; }
  
private:
  G4int HCID;
};

#endif
