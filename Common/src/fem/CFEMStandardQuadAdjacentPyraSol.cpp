/*!
 * \file CFEMStandardQuadAdjacentPyraSol.cpp
 * \brief Functions for the class CFEMStandardQuadAdjacentPyraSol.
 * \author E. van der Weide
 * \version 7.1.1 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2020, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../include/fem/CFEMStandardQuadAdjacentPyraSol.hpp"

/*----------------------------------------------------------------------------------*/
/*            Public member functions of CFEMStandardQuadAdjacentPyraSol.           */
/*----------------------------------------------------------------------------------*/

CFEMStandardQuadAdjacentPyraSol::CFEMStandardQuadAdjacentPyraSol(const unsigned short val_nPoly,
                                                                 const unsigned short val_orderExact,
                                                                 const unsigned short val_faceID_Elem,
                                                                 const unsigned short val_orientation,
                                                                 CGemmBase           *val_gemm_1,
                                                                 CGemmBase           *val_gemm_2)
  : CFEMStandardPyraBase(),
    CFEMStandardQuadBase(val_nPoly, val_orderExact) {

  /*--- Store the faceID of the element and the orientation. ---*/
  faceID_Elem = val_faceID_Elem;
  orientation = val_orientation;

  /*--- Convert the pointers for the gemm functionalities. ---*/
  gemmDOFs2Int = dynamic_cast<CGemmStandard *> (val_gemm_1);
  gemmInt2DOFs = dynamic_cast<CGemmStandard *> (val_gemm_2);
  if(!gemmDOFs2Int || !gemmInt2DOFs)
    SU2_MPI::Error(string("Dynamic cast failure. This should not happen"), CURRENT_FUNCTION);

  /*--- Convert the 2D parametric coordinates of the integration points of the
        quadrilateral face to the 3D parametric coordinates of the adjacent pyramid. ---*/
  vector<passivedouble> rInt, sInt, tInt;
  ConvertCoor2DQuadFaceTo3DPyra(rLineInt, val_faceID_Elem, val_orientation, rInt, sInt, tInt);

  /*--- Allocate the memory for the Legendre basis functions and its
        1st derivatives in the integration points. ---*/
  nDOFs = nDOFs1D*(nDOFs1D+1)*(2*nDOFs1D+1)/6;
  legBasisInt.resize(nIntegrationPad, nDOFs); legBasisInt.setConstant(0.0);

  derLegBasisInt.resize(3);
  derLegBasisInt[0].resize(nIntegrationPad, nDOFs); derLegBasisInt[0].setConstant(0.0);
  derLegBasisInt[1].resize(nIntegrationPad, nDOFs); derLegBasisInt[1].setConstant(0.0);
  derLegBasisInt[2].resize(nIntegrationPad, nDOFs); derLegBasisInt[2].setConstant(0.0);

  /*--- Compute the Legendre basis functions and its first
        derivatives in the integration points. ---*/
  VandermondePyramid(nPoly, rInt, sInt, tInt, legBasisInt);
  GradVandermondePyramid(nPoly, rInt, sInt, tInt, derLegBasisInt[0],
                         derLegBasisInt[1], derLegBasisInt[2]);

  /*--- Make sure that the padded values of legBasisInt are initialized properly
        to avoid problems. The gradients are set to zero, which is fine. ---*/
  for(unsigned short i=nIntegration; i<nIntegrationPad; ++i)
    for(unsigned short j=0; j<nDOFs; ++j) 
      legBasisInt(i,j) = legBasisInt(0,j);
}

void CFEMStandardQuadAdjacentPyraSol::GradSolIntPoints(ColMajorMatrix<su2double>          &matSolDOF,
                                                       vector<ColMajorMatrix<su2double> > &matGradSolInt) {

  /*--- Call the general functionality of gemmDOFs2Int with the appropriate
        arguments to compute the gradient of the solution in the integration
        points of the face. ---*/
  gemmDOFs2Int->DOFs2Int(derLegBasisInt[0], matSolDOF.cols(), matSolDOF, matGradSolInt[0], nullptr);
  gemmDOFs2Int->DOFs2Int(derLegBasisInt[1], matSolDOF.cols(), matSolDOF, matGradSolInt[1], nullptr);
  gemmDOFs2Int->DOFs2Int(derLegBasisInt[2], matSolDOF.cols(), matSolDOF, matGradSolInt[2], nullptr);
}

void CFEMStandardQuadAdjacentPyraSol::SolIntPoints(ColMajorMatrix<su2double> &matSolDOF,
                                                   ColMajorMatrix<su2double> &matSolInt) {

  /*--- Call the general functionality of gemmDOFs2Int with the appropriate
        arguments to compute the solution in the integration points
        of the face. ---*/
  gemmDOFs2Int->DOFs2Int(legBasisInt, matSolDOF.cols(), matSolDOF, matSolInt, nullptr);
}