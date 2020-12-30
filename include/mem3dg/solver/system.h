// Membrane Dynamics in 3D using Discrete Differential Geometry (Mem3DG)
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2020:
//     Laboratory for Computational Cellular Mechanobiology
//     Cuncheng Zhu (cuzhu@eng.ucsd.edu)
//     Christopher T. Lee (ctlee@ucsd.edu)
//     Ravi Ramamoorthi (ravir@cs.ucsd.edu)
//     Padmini Rangamani (prangamani@eng.ucsd.edu)
//

#pragma once

#include <cassert>

#include <geometrycentral/surface/halfedge_mesh.h>
#include <geometrycentral/surface/heat_method_distance.h>
#include <geometrycentral/surface/intrinsic_geometry_interface.h>
#include <geometrycentral/surface/vertex_position_geometry.h>
#include <geometrycentral/utilities/eigen_interop_helpers.h>

#include <Eigen/Core>
#include <Eigen/SparseLU>

#include <pcg_random.hpp>
#include <random>

#include <math.h>

#include "mem3dg/solver/macros.h"
#include "mem3dg/solver/meshops.h"
#include "mem3dg/solver/util.h"
#include "mem3dg/solver/constants.h"

#include <vector>

namespace mem3dg {

namespace gc = ::geometrycentral;
namespace gcs = ::geometrycentral::surface;

struct Parameters {
  /// Bending modulus
  double Kb;
  /// Spontaneous curvature
  double H0;
  /// Sharpness of the spontaneous curvature hetergeneity
  double sharpness;
  /// radius of non-zero spontaneous curvature
  std::vector<double> r_H0;
  /// Global stretching modulus
  double Ksg;
  /// Vertex shifting constant
  double Kst;
  /// Local stretching modulus
  double Ksl;
  /// Edge spring constant
  double Kse;
  /// Volume regularization
  double Kv;
  /// Line tension
  double eta;
  /// binding energy per protein
  double epsilon;
  /// binding constant
  double Bc;
  /// Dissipation coefficient
  double gamma;
  /// Reduced volume
  double Vt;
  /// Boltzmann constant*Temperature
  double temp;
  /// Noise
  double sigma;
  /// index of node with applied external force
  std::vector<double> pt;
  /// Magnitude of external force
  double Kf;
  /// level of concentration of the external force
  double conc;
  /// target height
  double height;
  /// domain of integration
  double radius;
  /// augmented Lagrangian parameter for area
  double lambdaSG = 0;
  /// augmented Lagrangian parameter for volume
  double lambdaV = 0;
};

struct Energy {
  /// total Energy of the system
  double totalE;
  /// kinetic energy of the membrane
  double kE;
  /// potential energy of the membrane
  double potE;
  /// bending energy of the membrane
  double BE;
  /// stretching energy of the membrane
  double sE;
  /// work of pressure within membrane
  double pE;
  /// chemical energy of the membrane protein
  double cE;
  /// line tension energy of interface
  double lE;
  /// work of external force
  double exE;
};

class DLL_PUBLIC System {
public:
  /// Parameters
  Parameters P;
  /// Cached mesh of interest
  gcs::ManifoldSurfaceMesh &mesh;
  /// Cached mesh data
  gcs::RichSurfaceMeshData &richData;
  /// Embedding and other geometric details
  gcs::VertexPositionGeometry &vpg;
  /// reference embedding geometry
  gcs::VertexPositionGeometry &refVpg;
  /// Energy
  Energy E;

  /// Cached bending stress
  gcs::VertexData<gc::Vector3> bendingPressure;
  /// Cached tension-induced capillary pressure
  gcs::VertexData<gc::Vector3> capillaryPressure;
  /// Cached interfacial line tension
  gcs::VertexData<gc::Vector3> lineTensionPressure;
  /// Cached relative inside pressure
  gcs::VertexData<gc::Vector3> insidePressure;
  /// Cached externally-applied pressure
  gcs::VertexData<gc::Vector3> externalPressure;

  /// Cached local stretching forces (in-plane regularization)
  gcs::VertexData<gc::Vector3> regularizationForce;
  /// Cached damping forces
  gcs::VertexData<gc::Vector3> dampingForce;
  /// Cached stochastic forces
  gcs::VertexData<gc::Vector3> stochasticForce;

  /// Cached protein surface density
  gcs::VertexData<double> proteinDensity;
  /// Cached chemical potential
  gcs::VertexData<double> chemicalPotential;

  /// Whether or not use tufted laplacian matrix
  const bool isTuftedLaplacian;
  /// Mollify Factor in constructing tufted laplacian matrix
  const double mollifyFactor;
  /// Whether or not do vertex shift
  const bool isVertexShift;
  /// Whether or not consider protein binding
  const bool isProtein;
  /// Whether circular spon curv domain
  const bool isCircle;

  /// Target area per face
  gcs::FaceData<double> targetFaceAreas;
  /// Target total face area
  double targetSurfaceArea;
  /// Maximal volume
  double refVolume;
  /// Target length per edge
  gcs::EdgeData<double> targetEdgeLengths;
  /// Target edge cross length ratio
  gcs::EdgeData<double> targetLcr;
  /// Distance solver
  gcs::HeatMethodDistanceSolver heatSolver;

  /// Cached galerkin mass matrix
  Eigen::SparseMatrix<double> M;
  /// Inverted galerkin mass matrix
  Eigen::SparseMatrix<double> M_inv;
  /// Cotangent Laplacian
  Eigen::SparseMatrix<double> L;
  /// Cached geodesic distance
  gcs::VertexData<double> geodesicDistanceFromPtInd;

  /// L2 error norm
  double L2ErrorNorm;
  /// surface area
  double surfaceArea;
  /// Volume
  double volume;
  /// Interface Area;
  double interArea;
  /// Cached vertex positions from the previous step
  gcs::VertexData<gc::Vector3> pastPositions;
  /// Cached vertex velocity by finite differencing past and current position
  gcs::VertexData<gc::Vector3> vel;
  // Mean curvature of the mesh
  Eigen::Matrix<double, Eigen::Dynamic, 1> H;
  // Spontaneous curvature of the mesh
  Eigen::Matrix<double, Eigen::Dynamic, 1> H0;
  /// Random number engine
  pcg32 rng;
  std::normal_distribution<double> normal_dist;
  /// indices for vertices chosen for integration
  Eigen::Matrix<bool, Eigen::Dynamic, 1> mask;
  /// "the point" index
  size_t ptInd;

  /*
   * @brief Construct a new Force object
   *
   * @param mesh_         Mesh connectivity
   * @param vpg_          Embedding and geometry information
   * @param time_step_    Numerical timestep
   */

  System(gcs::ManifoldSurfaceMesh &mesh_, gcs::VertexPositionGeometry &vpg_,
         gcs::VertexPositionGeometry &refVpg_,
         gcs::RichSurfaceMeshData &richData_, Parameters &p, bool isProtein_,
         bool isVertexShift_, bool isTuftedLaplacian_,
         double mollifyFactor_ = 1e-6)
      : mesh(mesh_), vpg(vpg_), richData(richData_), refVpg(refVpg_), P(p),
        isTuftedLaplacian(isTuftedLaplacian_), isProtein(isProtein_),
        isCircle(p.r_H0[0] == p.r_H0[1]), mollifyFactor(mollifyFactor_),
        isVertexShift(isVertexShift_), bendingPressure(mesh_, {0, 0, 0}),
        insidePressure(mesh_, {0, 0, 0}), capillaryPressure(mesh_, {0, 0, 0}),
        lineTensionPressure(mesh_, {0, 0, 0}), chemicalPotential(mesh_, 0.0),
        externalPressure(mesh_, {0, 0, 0}),
        regularizationForce(mesh_, {0, 0, 0}), targetLcr(mesh_),
        stochasticForce(mesh_, {0, 0, 0}), dampingForce(mesh_, {0, 0, 0}),
        proteinDensity(mesh_, 0), vel(mesh_, {0, 0, 0}),
        E({0, 0, 0, 0, 0, 0, 0, 0, 0}), heatSolver(vpg) {

    // Initialize RNG
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    rng = pcg32(seed_source);

    // GC computed properties
    vpg.requireFaceNormals();
    vpg.requireVertexLumpedMassMatrix();
    vpg.requireCotanLaplacian();
    vpg.requireFaceAreas();
    vpg.requireVertexIndices();
    vpg.requireVertexGaussianCurvatures();
    vpg.requireFaceIndices();
    vpg.requireEdgeLengths();
    vpg.requireVertexNormals();
    vpg.requireVertexDualAreas();
    vpg.requireCornerAngles();
    vpg.requireCornerScaledAngles();
    // vpg.requireVertexTangentBasis();

    /// compute constant values during simulation
    // Find the closest point index to P.pt in refVpg
    closestPtIndToPt(mesh, refVpg, P.pt, ptInd);

    // Initialize the constant mask based on distance from the point specified
    // Or mask boundary element
    mask = (heatMethodDistance(refVpg, mesh.vertex(ptInd)).raw().array() <
            P.radius)
               .matrix();
    if (mesh.hasBoundary()) {
      boundaryMask(mesh, mask);
    }

    // Initialize the constant target face/surface areas
    targetFaceAreas = refVpg.faceAreas;
    targetSurfaceArea = targetFaceAreas.raw().sum();

    // Initialize the constant target edge length
    targetEdgeLengths = refVpg.edgeLengths.reinterpretTo(mesh);

    // Initialize the target constant cross length ration
    getCrossLengthRatio(mesh, refVpg, targetLcr);

    // Initialize the constant reference volume
    if (mesh.hasBoundary()) {
      refVolume = 0.0;
    } else {
      refVolume = std::pow(targetSurfaceArea / constants::PI / 4, 1.5) * (4 * constants::PI / 3);
    }

    // Regularize the vetex position geometry if needed
    if (isVertexShift) {
      vertexShift(mesh, vpg, mask);
      update_Vertex_positions();
    }

    /// compute nonconstant values during simulation
    update_Vertex_positions();
  }

  /**
   * @brief Destroy the Force object
   *
   * Explicitly unrequire values required by the constructor. In case, there
   * is another pointer to the HalfEdgeMesh and VertexPositionGeometry
   * elsewhere, calculation of dependent quantities should be respected.
   */
  ~System() {
    vpg.unrequireFaceNormals();
    vpg.unrequireVertexLumpedMassMatrix();
    vpg.unrequireCotanLaplacian();
    vpg.unrequireFaceAreas();
    vpg.unrequireVertexIndices();
    vpg.unrequireVertexGaussianCurvatures();
    vpg.unrequireFaceIndices();
    vpg.unrequireEdgeLengths();
    vpg.unrequireVertexNormals();
    vpg.unrequireVertexDualAreas();
    vpg.unrequireCornerAngles();
    vpg.unrequireCornerScaledAngles();
  }

  void getBendingPressure();

  void getChemicalPotential();

  void getCapillaryPressure();

  void getInsidePressure();

  void getRegularizationForce();

  void getLineTensionPressure();

  void getDPDForces();

  void getExternalPressure();

  void getAllForces();

  /**
   * @brief Get free energy and each components of the system
   */
  void getFreeEnergy();

  /**
   * @brief Get the L2 norm of the force (pressure), which is the residual of
   * the PDE
   */
  void
  getL2ErrorNorm(Eigen::Matrix<double, Eigen::Dynamic, 3> physicalPressure);

  /**
   * @brief Get velocity from the position of the last iteration
   *
   * @param timeStep
   */
  void getVelocityFromPastPosition(double dt);

  /**
   * @brief Update the vertex position and recompute cached values
   * (all quantities that characterizes the current energy state)
   *
   */
  void update_Vertex_positions() {
    vpg.refreshQuantities();

    auto vertexAngleNormal_e = gc::EigenMap<double, 3>(vpg.vertexNormals);
    auto positions = gc::EigenMap<double, 3>(vpg.inputVertexPositions);

    // initialize/update (inverse) mass and Laplacian matrix
    if (isTuftedLaplacian) {
      getTuftedLaplacianAndMass(M, L, mesh, vpg, mollifyFactor);
    } else {
      M = vpg.vertexLumpedMassMatrix;
      L = vpg.cotanLaplacian;
    }
    M_inv = (1 / (M.diagonal().array())).matrix().asDiagonal();

    // initialize/update distance from the point specified
    geodesicDistanceFromPtInd = heatSolver.computeDistance(mesh.vertex(ptInd));

    // initialize/update spontaneous curvature
    if (isProtein) {
      // proteinDensity.raw().setZero();
      // H0.setZero(mesh.nVertices(), 1);
      Eigen::Matrix<double, Eigen::Dynamic, 1> proteinDensitySq =
          (proteinDensity.raw().array() * proteinDensity.raw().array())
              .matrix();
      H0 = (P.H0 * proteinDensitySq.array() / (1 + proteinDensitySq.array()))
               .matrix();
    } else if (P.H0 != 0) {
      if (isCircle) {
        tanhDistribution(H0, geodesicDistanceFromPtInd.raw(), P.sharpness,
                         P.r_H0[0]);
      } else {
        tanhDistribution(vpg, H0, geodesicDistanceFromPtInd.raw(), P.sharpness,
                         P.r_H0);
      }
      H0 *= P.H0;
      if (((H0.array() - (H0.sum() / mesh.nVertices())).matrix().norm() <
           1e-12)) {
        assert(P.eta == 0);
      }
    } else {
      H0.setZero(mesh.nVertices(), 1);
      assert(P.eta == 0);
    }

    // initialize/update mean curvature
    Eigen::Matrix<double, Eigen::Dynamic, 1> H_integrated =
        rowwiseDotProduct(L * positions / 2.0, vertexAngleNormal_e);
    H = M_inv * H_integrated;

    /// initialize/udate excess pressure
    volume = 0;
    for (gcs::Face f : mesh.faces()) {
      volume += signedVolumeFromFace(
          f, vpg, refVpg.inputVertexPositions[mesh.vertex(ptInd)]);
    }

    // initialize/update total surface area
    surfaceArea = vpg.faceAreas.raw().sum();

    // initialize/update intersection area
    interArea = 0.0;
    for (gcs::Vertex v : mesh.vertices()) {
      if ((H0[v.getIndex()] > (0.1 * P.H0)) &&
          (H0[v.getIndex()] < (0.9 * P.H0)) && (H[v.getIndex()] != 0)) {
        interArea += vpg.vertexDualAreas[v];
      }
    }

    // initialize/update external force
    getExternalPressure();

    // initialize/update the vertex position of the last iteration
    pastPositions = vpg.inputVertexPositions;

    // zero all forces
    gc::EigenMap<double, 3>(bendingPressure).setZero();
    gc::EigenMap<double, 3>(insidePressure).setZero();
    gc::EigenMap<double, 3>(capillaryPressure).setZero();
    gc::EigenMap<double, 3>(lineTensionPressure).setZero();
    gc::EigenMap<double, 3>(externalPressure).setZero();
    gc::EigenMap<double, 3>(regularizationForce).setZero();
    gc::EigenMap<double, 3>(dampingForce).setZero();
    gc::EigenMap<double, 3>(stochasticForce).setZero();
    chemicalPotential.raw().setZero();
  }

  void pcg_test();
};
} // end namespace ddgsolver
