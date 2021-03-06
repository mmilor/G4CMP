/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id$
//
// Wrapper class for G4ElectroMagneticField objects which handles transforming
// between global and local coordinates for input and output queries.

#include "G4CMPLocalElectroMagField.hh"
#include <algorithm>

// Specify local-to-global transformation before field call
// Typically, this will be called from FieldManager::ConfigureForTrack()

void G4CMPLocalElectroMagField::SetTransforms(const G4AffineTransform& lToG) {
  fGlobalToLocal = fLocalToGlobal = lToG;
  fGlobalToLocal.Invert();
}


// This function transforms Point[0..2] to local coordinates on input,
// and Field[3..5] from local to global coordinate on output
void G4CMPLocalElectroMagField::GetFieldValue(const G4double Point[4],
					      G4double *BEfield) const {
  GetLocalPoint(Point);

  // Get local field vector(s) using local position
  std::fill(localF, localF+6, 0.);
  localField->GetFieldValue(localP, localF);

  // Transform local magnetic and electric fields to global
  CopyLocalToGlobalVector(0, BEfield);
  CopyLocalToGlobalVector(3, BEfield);
}


// Copy input point to vector and rotate to local coordinates

void G4CMPLocalElectroMagField::GetLocalPoint(const G4double Point[4]) const {
  vec.set(Point[0], Point[1], Point[2]);
  fGlobalToLocal.ApplyPointTransform(vec);

  // Fill local point buffer and initialize field
  localP[0] = vec.x();
  localP[1] = vec.y();
  localP[2] = vec.z();
  localP[3] = Point[3];		// Time coordinate
}

// Transform local vector (array) to global coordinate system

void G4CMPLocalElectroMagField::
CopyLocalToGlobalVector(G4int index, G4double* gbl) const {
  vec.set(localF[index+0], localF[index+1], localF[index+2]);
  fLocalToGlobal.ApplyAxisTransform(vec);

  gbl[index+0] = vec.x();
  gbl[index+1] = vec.y();
  gbl[index+2] = vec.z();
}
