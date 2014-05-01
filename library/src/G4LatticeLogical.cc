//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file materials/src/G4LatticeLogical.cc
/// \brief Implementation of the G4LatticeLogical class
//
// $Id$
//
// 20140218  Add new charge-carrier parameters to output
// 20140306  Allow valley filling using Euler angles directly
// 20140318  Compute electron mass scalar (Herring-Vogt) from tensor
// 20140324  Include inverse mass-ratio tensor
// 20140408  Add valley momentum calculations
// 20140425  Add "effective mass" calculation for electrons

#include "G4LatticeLogical.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include <cmath>
#include <fstream>


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4LatticeLogical::G4LatticeLogical()
  : verboseLevel(0), fVresTheta(0), fVresPhi(0), fDresTheta(0), fDresPhi(0),
    fA(0), fB(0), fLDOS(0), fSTDOS(0), fFTDOS(0),
    fBeta(0), fGamma(0), fLambda(0), fMu(0),
    fVSound(0.), fL0_e(0.), fL0_h(0.), 
    mElectron(electron_mass_c2/c_squared),
    fHoleMass(mElectron), fElectronMass(mElectron),
    fMassTensor(G4Rep3x3(mElectron,0.,0.,0.,mElectron,0.,0.,0.,mElectron)),
    fMassInverse(G4Rep3x3(1/mElectron,0.,0.,0.,1/mElectron,0.,0.,0.,1/mElectron)),
    fIVField(0.), fIVRate(0.), fIVExponent(0.) {
  for (G4int i=0; i<3; i++) {
    for (G4int j=0; j<MAXRES; j++) {
      for (G4int k=0; k<MAXRES; k++) {
	fMap[i][j][k] = 0.;
	fN_map[i][j][k].set(0.,0.,0.);
      }
    }
  }
}

G4LatticeLogical::~G4LatticeLogical() {;}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....


///////////////////////////////////////////
//Load map of group velocity scalars (m/s)
////////////////////////////////////////////
G4bool G4LatticeLogical::LoadMap(G4int tRes, G4int pRes,
				 G4int polarizationState, G4String map) {
  if (tRes>MAXRES || pRes>MAXRES) {
    G4cerr << "G4LatticeLogical::LoadMap exceeds maximum resolution of "
           << MAXRES << " by " << MAXRES << ". terminating." << G4endl;
    return false; 		//terminate if resolution out of bounds.
  }

  std::ifstream fMapFile(map.data());
  if (!fMapFile.is_open()) return false;

  G4double vgrp = 0.;
  for (G4int theta = 0; theta<tRes; theta++) {
    for (G4int phi = 0; phi<pRes; phi++) {
      fMapFile >> vgrp;
      fMap[polarizationState][theta][phi] = vgrp * (m/s);
    }
  }

  if (verboseLevel) {
    G4cout << "\nG4LatticeLogical::LoadMap(" << map << ") successful"
	   << " (Vg scalars " << tRes << " x " << pRes << " for polarization "
	   << polarizationState << ")." << G4endl;
  }

  fVresTheta=tRes; //store map dimensions
  fVresPhi=pRes;
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....


////////////////////////////////////
//Load map of group velocity unit vectors
///////////////////////////////////
G4bool G4LatticeLogical::Load_NMap(G4int tRes, G4int pRes,
				   G4int polarizationState, G4String map) {
  if (tRes>MAXRES || pRes>MAXRES) {
    G4cerr << "G4LatticeLogical::LoadMap exceeds maximum resolution of "
           << MAXRES << " by " << MAXRES << ". terminating." << G4endl;
    return false; 		//terminate if resolution out of bounds.
  }

  std::ifstream fMapFile(map.data());
  if(!fMapFile.is_open()) return false;

  G4double x,y,z;	// Buffers to read coordinates from file
  G4ThreeVector dir;
  for (G4int theta = 0; theta<tRes; theta++) {
    for (G4int phi = 0; phi<pRes; phi++) {
      fMapFile >> x >> y >> z;
      dir.set(x,y,z);
      fN_map[polarizationState][theta][phi] = dir.unit();	// Enforce unity
    }
  }

  if (verboseLevel) {
    G4cout << "\nG4LatticeLogical::Load_NMap(" << map << ") successful"
	   << " (Vdir " << tRes << " x " << pRes << " for polarization "
	   << polarizationState << ")." << G4endl;
  }

  fDresTheta=tRes; //store map dimensions
  fDresPhi=pRes;
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//Given the phonon wave vector k, phonon physical volume Vol 
//and polarizationState(0=LON, 1=FT, 2=ST), 
//returns phonon velocity in m/s

G4double G4LatticeLogical::MapKtoV(G4int polarizationState,
				   const G4ThreeVector& k) const {
  G4double theta, phi, tRes, pRes;

  tRes=pi/fVresTheta;
  pRes=twopi/fVresPhi;
  
  theta=k.getTheta();
  phi=k.getPhi();

  if(phi<0) phi = phi + twopi;
  if(theta>pi) theta=theta-pi;

  G4double Vg = fMap[polarizationState][int(theta/tRes)][int(phi/pRes)];

  if(Vg == 0){
      G4cout<<"\nFound v=0 for polarization "<<polarizationState
            <<" theta "<<theta<<" phi "<<phi<< " translating to map coords "
            <<"theta "<< int(theta/tRes) << " phi " << int(phi/pRes)<<G4endl;
  }

  if (verboseLevel>1) {
    G4cout << "G4LatticeLogical::MapKtoV theta,phi=" << theta << " " << phi
	   << " : ith,iph " << int(theta/tRes) << " " << int(phi/pRes)
	   << " : V " << Vg << G4endl;
  }

  return Vg;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//Given the phonon wave vector k, phonon physical volume Vol 
//and polarizationState(0=LON, 1=FT, 2=ST), 
//returns phonon propagation direction as dimensionless unit vector

G4ThreeVector G4LatticeLogical::MapKtoVDir(G4int polarizationState,
					   const G4ThreeVector& k) const {  
  G4double theta, phi, tRes, pRes;

  tRes=pi/(fDresTheta-1);//The summant "-1" is required:index=[0:array length-1]
  pRes=2*pi/(fDresPhi-1);

  theta=k.getTheta();
  phi=k.getPhi(); 

  if(theta>pi) theta=theta-pi;
  //phi=[0 to 2 pi] in accordance with DMC //if(phi>pi/2) phi=phi-pi/2;
  if(phi<0) phi = phi + 2*pi;

  G4int iTheta = int(theta/tRes+0.5);
  G4int iPhi = int(phi/pRes+0.5);

  if (verboseLevel>1) {
    G4cout << "G4LatticeLogical::MapKtoVDir theta,phi=" << theta << " " << phi
	   << " : ith,iph " << iTheta << " " << iPhi
	   << " : dir " << fN_map[polarizationState][iTheta][iPhi] << G4endl;
  }

  return fN_map[polarizationState][iTheta][iPhi];
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

// Convert electron momentum to valley velocity, wavevector, and HV vector

G4ThreeVector 
G4LatticeLogical::MapPtoV_el(G4int ivalley, const G4ThreeVector& p_e) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapPtoV_el " << ivalley << " " << p_e
	   << G4endl;

  const G4RotationMatrix& vToN = GetValley(ivalley);
  return (vToN.inverse()*GetMInvTensor()*vToN) * p_e/c_light;
}

G4ThreeVector 
G4LatticeLogical::MapPtoK_valley(G4int ivalley, G4ThreeVector p_e) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapPtoK " << ivalley << " " << p_e
	   << G4endl;

  p_e /= hbarc;					// Convert to wavevector
  return p_e.transform(GetValley(ivalley));	// Rotate into valley frame
}

G4ThreeVector 
G4LatticeLogical::MapPtoK_HV(G4int ivalley, G4ThreeVector p_e) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapPtoK_HV " << ivalley << " " << p_e
	   << G4endl;

  p_e.transform(GetValley(ivalley));		// Rotate into valley frame
  return GetSqrtInvTensor() * p_e/hbarc;	// Herring-Vogt transformation
}

G4ThreeVector 
G4LatticeLogical::MapK_HVtoK_valley(G4int ivalley, G4ThreeVector k_HV) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapK_HVtoK_valley " << ivalley << " " << k_HV
	   << G4endl;

  k_HV *= GetSqrtTensor();			// From Herring-Vogt to valley
  return k_HV;
}

G4ThreeVector 
G4LatticeLogical::MapK_HVtoP(G4int ivalley, G4ThreeVector k_HV) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapK_HVtoP " << ivalley << " " << k_HV
	   << G4endl;

  k_HV *= GetSqrtTensor();			// From Herring-Vogt to valley 
  k_HV.transform(GetValley(ivalley).inverse());	// Rotate out of valley frame
  k_HV *= hbarc;			// Convert wavevector to momentum
  return k_HV;
}

G4ThreeVector 
G4LatticeLogical::MapK_valleyToP(G4int ivalley, G4ThreeVector k) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapK_valleyToP " << ivalley << " " << k
	   << G4endl;

  k.transform(GetValley(ivalley).inverse());	// Rotate out of valley frame
  k *= hbarc;				// Convert wavevector to momentum
  return k;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

// Apply energy-momentum relationship for electron transport

G4double  
G4LatticeLogical::MapPtoEkin(G4int iv, G4ThreeVector p) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::MapPtoEkin " << iv << " " << p << G4endl;

  p *= GetValley(iv);				// Rotate to valley frame

  // Compute kinetic energy component by component, then sum
  return (0.5/c_squared) * (p.x()*p.x()*fMassInverse.xx() +
			    p.y()*p.y()*fMassInverse.yy() +
			    p.z()*p.z()*fMassInverse.zz());
}

// Compute effective "scalar" electron mass to match energy/momentum relation

G4double 
G4LatticeLogical::GetElectronEffectiveMass(G4int iv,
					   const G4ThreeVector& p) const {
  if (verboseLevel>1)
    G4cout << "G4LatticeLogical::GetElectronEffectiveMass " << iv
	   << " " << p << G4endl;

  return 0.5*p.mag2()/c_squared/MapPtoEkin(iv,p);	// Non-relativistic
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

// Store electron mass tensor using diagonal elements

void G4LatticeLogical::SetMassTensor(G4double mXX, G4double mYY, G4double mZZ) {
  if (verboseLevel>1) {
    G4cout << "G4LatticeLogical::SetMassTensor " << mXX << " " << mYY
	   << " " << mZZ << " *m_e" << G4endl;
  }

  // NOTE:  Use of G4RotationMatrix not appropriate here, as matrix is
  //        not normalized.  But CLHEP/Matrix not available in GEANT4.
  fMassTensor.set(G4Rep3x3(mXX*mElectron, 0., 0.,
			   0., mYY*mElectron, 0.,
			   0., 0., mZZ*mElectron));

  FillMassInfo();
}

void G4LatticeLogical::SetMassTensor(const G4RotationMatrix& etens) {
  if (verboseLevel>1) {
    G4cout << "G4LatticeLogical::SetMassTensor " << etens << G4endl;
  }

  // Check if mass tensor already has electron mass, or is just coefficients
  G4bool hasEmass = (etens.xx()/mElectron > 1e-3 ||
		     etens.yy()/mElectron > 1e-3 ||
		     etens.zz()/mElectron > 1e-3);
  G4double mscale = hasEmass ? 1. : mElectron;

  // NOTE:  Use of G4RotationMatrix not appropriate here, as matrix is
  //        not normalized.  But CLHEP/Matrix not available in GEANT4.
  fMassTensor.set(G4Rep3x3(etens.xx()*mscale, 0., 0.,
			   0., etens.yy()*mscale, 0.,
			   0., 0., etens.zz()*mscale));

  FillMassInfo();
}

// Compute derived quantities from user-input mass tensor

void G4LatticeLogical::FillMassInfo() {
  // Herring-Vogt scalar mass of electron, used for sound speed calcs  
  fElectronMass = 3. / ( 1./fMassTensor.xx() + 1./fMassTensor.yy()
			 + 1./fMassTensor.zz() );  

  // 1/m mass tensor used for k and v calculations in valley coordinates
  fMassInverse.set(G4Rep3x3(1./fMassTensor.xx(), 0., 0.,
			    0., 1./fMassTensor.yy(), 0.,
			    0., 0., 1./fMassTensor.zz()));

  // Mass ratio tensor used for scattering and field calculations
  fMassRatioSqrt.set(G4Rep3x3(sqrt(fMassTensor.xx()/fElectronMass), 0., 0.,
			      0., sqrt(fMassTensor.yy()/fElectronMass), 0.,
			      0., 0., sqrt(fMassTensor.zz()/fElectronMass)));

  fMInvRatioSqrt.set(G4Rep3x3(1./fMassRatioSqrt.xx(), 0., 0.,
			      0., 1./fMassRatioSqrt.yy(), 0.,
			      0., 0., 1./fMassRatioSqrt.zz()));
}

// Store drifting-electron valley using Euler angles

void G4LatticeLogical::AddValley(G4double phi, G4double theta, G4double psi) {
  if (verboseLevel>1) {
    G4cout << "G4LatticeLogical::AddValley " << psi << " " << theta
	   << " " << psi << " rad" << G4endl;
  }

  // Extend vector first, then fill last value, to reduce temporaries
  fValley.resize(fValley.size()+1);
  fValley.back().set(phi,theta,psi);
}

// Transform for drifting-electron valleys in momentum space

const G4RotationMatrix& G4LatticeLogical::GetValley(G4int iv) const {
  if (verboseLevel>1) G4cout << "G4LatticeLogical::GetValley " << iv << G4endl;

  if (iv >=0 && iv < (G4int)NumberOfValleys()) return fValley[iv];

  G4cerr << "G4LatticeLogical ERROR: No such valley " << iv << G4endl;
  return G4RotationMatrix::IDENTITY;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

// Dump structure in format compatible with reading back

void G4LatticeLogical::Dump(std::ostream& os) const {
  os << "# Phonon propagation parameters"
     << "\ndyn " << fBeta << " " << fGamma << " " << fLambda << " " << fMu
     << "\nscat " << fB << " decay " << fA
     << "\nLDOS " << fLDOS << " STDOS " << fSTDOS << " FTDOS " << fFTDOS
     << std::endl;

  os << "# Charge carrier propagation parameters"
     << "\nhmass " << fHoleMass/mElectron
     << "\nemass " << fMassTensor.xx()/mElectron
     << " " << fMassTensor.yy()/mElectron
     << " " << fMassTensor.zz()/mElectron << std::endl;

  os << "# Inverse mass tensor: " << fMassInverse.xx()*mElectron
     << " " << fMassInverse.yy()*mElectron
     << " " << fMassInverse.zz()*mElectron
     << " * 1/m(electron)" << std::endl
     << "# Herring-Vogt scalar mass: " << fElectronMass/mElectron << std::endl
     << "# sqrt(tensor/scalor): " << fMassRatioSqrt.xx()
     << " " << fMassRatioSqrt.yy()
     << " " << fMassRatioSqrt.zz()
     << std::endl;

  for (size_t i=0; i<NumberOfValleys(); i++) {
    os << "valley " << fValley[i].phi()/deg << " " << fValley[i].theta()/deg
       << " " << fValley[i].psi()/deg << " deg" << std::endl;
  }

  os << "# Intervalley scattering parameters"
     << "\nivField " << fIVField << "\t# V/m"
     << "\nivRate " << fIVRate/s << "\t# s"
     << "\nivPower" << fIVExponent << std::endl;

  os << "# Phonon wavevector/velocity maps" << std::endl;
  Dump_NMap(os, 0, "LVec.ssv");
  Dump_NMap(os, 1, "FTVec.ssv");
  Dump_NMap(os, 2, "STVec.ssv");

  DumpMap(os, 0, "L.ssv");
  DumpMap(os, 1, "FT.ssv");
  DumpMap(os, 2, "ST.ssv");


}

void G4LatticeLogical::DumpMap(std::ostream& os, G4int pol,
			       const G4String& name) const {
  os << "VG " << name << " " << (pol==0?"L":pol==1?"FT":pol==2?"ST":"??")
     << " " << fVresTheta << " " << fVresPhi << std::endl;

  for (G4int iTheta=0; iTheta<fVresTheta; iTheta++) {
    for (G4int iPhi=0; iPhi<fVresPhi; iPhi++) {
      os << fMap[pol][iTheta][iPhi] << std::endl;
    }
  }
}

void G4LatticeLogical::Dump_NMap(std::ostream& os, G4int pol,
				 const G4String& name) const {
  os << "VDir " << name << " " << (pol==0?"L":pol==1?"FT":pol==2?"ST":"??")
     << " " << fDresTheta << " " << fDresPhi << std::endl;

  for (G4int iTheta=0; iTheta<fDresTheta; iTheta++) {
    for (G4int iPhi=0; iPhi<fDresPhi; iPhi++) {
      os << fN_map[pol][iTheta][iPhi].x()
	 << " " << fN_map[pol][iTheta][iPhi].y()
	 << " " << fN_map[pol][iTheta][iPhi].z()
	 << std::endl;
    }
  }
}

