/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4PhononDownconversion.hh
/// \brief Definition of the G4PhononDownconversion class
//
// $Id$
//
#ifndef G4PhononDownconversion_h
#define G4PhononDownconversion_h 1

#include "G4VPhononProcess.hh"

class G4PhononDownconversion : public G4VPhononProcess {
public:
  G4PhononDownconversion(const G4String& processName ="phononDownconversion");
  virtual ~G4PhononDownconversion();

  virtual G4VParticleChange* PostStepDoIt(const G4Track&, const G4Step& );
  
  virtual G4bool IsApplicable(const G4ParticleDefinition&);
  
protected:
  virtual G4double GetMeanFreePath(const G4Track&, G4double, G4ForceCondition*);
  
private:
  // relative probability that anharmonic decay occurs L->L'+T'
  inline double GetLTDecayProb(G4double, G4double) const;
  inline double GetTTDecayProb(G4double, G4double) const;
  inline double MakeLDeviation(G4double, G4double) const;
  inline double MakeTTDeviation(G4double, G4double) const;
  inline double MakeTDeviation(G4double, G4double) const;

  void MakeTTSecondaries(const G4Track&);
  void MakeLTSecondaries(const G4Track&);

private:
  double fBeta, fGamma, fLambda, fMu;	// Local buffers for calculations

  // hide assignment operator as private 
  G4PhononDownconversion(G4PhononDownconversion&);
  G4PhononDownconversion& operator=(const G4PhononDownconversion& right);
};

#endif










